#include "network/SegmentWorker.h"
#include <QMetaObject>
#include <QNetworkRequest>
#include <algorithm>

namespace idm::network {

SegmentWorker::SegmentWorker(
    int segmentId, qint64 startOffset, qint64 currentOffset, qint64 endOffset,
    storage::DiskWriter* diskWriter, core::SpeedLimiter* globalLimiter,
    core::SpeedLimiter* taskLimiter, QObject* parent
)
    : QObject(parent)
    , m_segmentId(segmentId)
    , m_startOffset(startOffset)
    , m_currentOffset(currentOffset)
    , m_endOffset(endOffset)
    , m_diskWriter(diskWriter)
    , m_globalLimiter(globalLimiter)
    , m_taskLimiter(taskLimiter) {
    m_throttleTimer.setInterval(25);
    connect(&m_throttleTimer, &QTimer::timeout, this, &SegmentWorker::drainAvailableBytes);
}

SegmentWorker::~SegmentWorker() {
    pause();
}

void SegmentWorker::start(
    const QUrl& url,
    const RequestProfile& profile,
    QNetworkAccessManager* sharedNam
) {
    detachReply();
    m_completedByTruncation = false;
    m_url = url;
    m_profile = profile;
    m_sharedNam = sharedNam;

    const qint64 cur = m_currentOffset.load(std::memory_order_acquire);
    const qint64 end = m_endOffset.load(std::memory_order_acquire);

    if (end >= 0 && cur > end) {
        m_state = core::SegmentState::Completed; m_statusText = QStringLiteral("Finished");
        m_speedBytesPerSec.store(0, std::memory_order_relaxed);
        emit stateUpdated(m_segmentId); emit segmentFinished(m_segmentId);
        return;
    }
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    profile.applyToRequest(req);
    const bool isYoutubeStream = url.host().contains(QStringLiteral("googlevideo.com"), Qt::CaseInsensitive);
    if (end >= 0) {
        const qint64 reqEnd = isYoutubeStream ? std::min<qint64>(end, cur + 8388607LL) : end;
        req.setRawHeader("Range", QByteArray("bytes=") + QByteArray::number(cur) + "-" + QByteArray::number(reqEnd));
    } else if (cur > 0) {
        req.setRawHeader("Range", QByteArray("bytes=") + QByteArray::number(cur) + "-");
    }
    m_state = core::SegmentState::Connecting;
    m_statusText = (m_retryCount > 0) ? QStringLiteral("Reconnecting (%1/15)").arg(m_retryCount) : QStringLiteral("Connecting");
    m_speedBytesPerSec.store(0, std::memory_order_relaxed);
    m_bytesInWindow = 0; m_rateTimer.start();

    QNetworkReply* reply = sharedNam->get(req);
    reply->setReadBufferSize(512 * 1024);
    m_reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, &SegmentWorker::drainAvailableBytes);
    connect(reply, &QNetworkReply::finished, this, &SegmentWorker::handleReplyFinished);
    m_throttleTimer.start();
    emit stateUpdated(m_segmentId);
}
void SegmentWorker::pause() {
    m_throttleTimer.stop(); detachReply();
    m_speedBytesPerSec.store(0, std::memory_order_relaxed);
    if (m_state != core::SegmentState::Completed) {
        m_state = core::SegmentState::Paused; m_statusText = QStringLiteral("Paused");
        emit stateUpdated(m_segmentId);
    }
}

void SegmentWorker::reassignRange(
    qint64 startOffset, qint64 endOffset,
    const QUrl& url, const RequestProfile& profile, QNetworkAccessManager* sharedNam
) {
    detachReply();
    const qint64 prevStart = m_startOffset.load(std::memory_order_acquire);
    const qint64 prevCur = m_currentOffset.load(std::memory_order_acquire);
    if (prevCur > prevStart) {
        m_priorCompletedBytes += (prevCur - prevStart);
    }
    m_retryCount = 0;
    m_startOffset.store(startOffset, std::memory_order_release);
    m_currentOffset.store(startOffset, std::memory_order_release);
    m_endOffset.store(endOffset, std::memory_order_release);
    start(url, profile, sharedNam);
}

SplitRange SegmentWorker::tryStealHalf(qint64 minRemainingBytes) {
    SplitRange split;
    if (m_state != core::SegmentState::Downloading && m_state != core::SegmentState::Connecting) {
        return split;
    }
    qint64 expectedEnd = m_endOffset.load(std::memory_order_acquire);
    const qint64 cur = m_currentOffset.load(std::memory_order_acquire);
    if (expectedEnd < 0) return split;

    const qint64 rem = (expectedEnd - cur) + 1;
    if (rem < std::max<qint64>(minRemainingBytes, 1048576)) return split;

    const qint64 midpoint = cur + (rem / 2) - 1;
    if (midpoint <= cur || midpoint >= expectedEnd) return split;

    if (m_endOffset.compare_exchange_strong(expectedEnd, midpoint, std::memory_order_acq_rel)) {
        split.valid = true;
        split.newStartOffset = midpoint + 1;
        split.newEndOffset = expectedEnd;
    }
    return split;
}

core::SegmentInfo SegmentWorker::snapshot() const {
    core::SegmentInfo info;
    info.id = m_segmentId;
    info.startOffset = m_startOffset.load(std::memory_order_relaxed);
    info.currentOffset = m_currentOffset.load(std::memory_order_relaxed);
    info.endOffset = m_endOffset.load(std::memory_order_relaxed);
    info.speedBytesPerSec = m_speedBytesPerSec.load(std::memory_order_relaxed);
    info.state = m_state;
    info.statusText = m_statusText;
    return info;
}

qint64 SegmentWorker::remainingBytes() const noexcept {
    const qint64 end = m_endOffset.load(std::memory_order_relaxed);
    const qint64 cur = m_currentOffset.load(std::memory_order_relaxed);
    return (end < 0 || end < cur) ? 0 : (end - cur) + 1;
}

qint64 SegmentWorker::totalDownloadedBytes() const noexcept {
    const qint64 start = m_startOffset.load(std::memory_order_relaxed);
    const qint64 cur = m_currentOffset.load(std::memory_order_relaxed);
    return m_priorCompletedBytes + std::max<qint64>(0, cur - start);
}

bool SegmentWorker::isRangeFullyDownloaded() const noexcept {
    const qint64 end = m_endOffset.load(std::memory_order_acquire);
    const qint64 cur = m_currentOffset.load(std::memory_order_acquire);
    return (end < 0) ? (m_state == core::SegmentState::Completed) : (cur > end);
}

void SegmentWorker::drainAvailableBytes() {
    if (m_reply == nullptr) return;
    const int httpStatus = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus >= 400 || m_reply->error() != QNetworkReply::NoError) return;

    const qint64 avail = m_reply->bytesAvailable();
    if (avail <= 0) { updateSpeedMetric(0); return; }

    const qint64 cur = m_currentOffset.load(std::memory_order_acquire);
    if (cur > 0 && httpStatus == 200) return;

    if (m_state == core::SegmentState::Connecting) {
        m_state = core::SegmentState::Downloading;
        m_statusText = QStringLiteral("Receiving data");
        m_retryCount = 0;
    }

    qint64 allowed = std::min<qint64>(avail, 256 * 1024);
    if (m_globalLimiter != nullptr && m_globalLimiter->isEnabled()) allowed = m_globalLimiter->requestQuota(allowed);
    if (allowed > 0 && m_taskLimiter != nullptr && m_taskLimiter->isEnabled()) allowed = m_taskLimiter->requestQuota(allowed);
    if (allowed <= 0) { updateSpeedMetric(0); return; }

    const qint64 end = m_endOffset.load(std::memory_order_acquire);
    if (end >= 0) {
        const qint64 maxNeeded = (end - cur) + 1;
        if (maxNeeded <= 0) {
            m_completedByTruncation = true; m_throttleTimer.stop(); detachReply();
            m_state = core::SegmentState::Completed; m_statusText = QStringLiteral("Finished");
            m_speedBytesPerSec.store(0, std::memory_order_relaxed);
            emit stateUpdated(m_segmentId);
            QMetaObject::invokeMethod(this, [this]() { emit segmentFinished(m_segmentId); }, Qt::QueuedConnection);
            return;
        }
        allowed = std::min(allowed, maxNeeded);
    }

    const QByteArray chunk = m_reply->read(allowed);
    if (chunk.isEmpty()) return;

    QString diskError;
    if (!m_diskWriter->writeAt(cur, chunk, &diskError)) {
        m_throttleTimer.stop(); detachReply();
        m_state = core::SegmentState::Error; m_statusText = diskError;
        m_speedBytesPerSec.store(0, std::memory_order_relaxed);
        emit stateUpdated(m_segmentId); emit segmentFailed(m_segmentId, diskError);
        return;
    }

    const qint64 newCur = cur + chunk.size();
    m_currentOffset.store(newCur, std::memory_order_release);
    updateSpeedMetric(chunk.size());
    emit bytesWritten(m_segmentId, chunk.size());

    const qint64 latestEnd = m_endOffset.load(std::memory_order_acquire);
    if (latestEnd >= 0 && newCur > latestEnd) {
        m_completedByTruncation = true; m_throttleTimer.stop(); detachReply();
        m_state = core::SegmentState::Completed; m_statusText = QStringLiteral("Finished");
        m_speedBytesPerSec.store(0, std::memory_order_relaxed);
        emit stateUpdated(m_segmentId);
        QMetaObject::invokeMethod(this, [this]() { emit segmentFinished(m_segmentId); }, Qt::QueuedConnection);
    }
}

void SegmentWorker::handleReplyFinished() {
    m_throttleTimer.stop();
    QNetworkReply* activeReply = m_reply;
    if (activeReply == nullptr) return;

    while (!m_completedByTruncation && m_reply == activeReply && activeReply->bytesAvailable() > 0) {
        const qint64 prev = m_currentOffset.load(std::memory_order_acquire);
        drainAvailableBytes();
        if (m_completedByTruncation || m_reply != activeReply || m_currentOffset.load(std::memory_order_acquire) == prev) {
            break;
        }
    }
    if (m_completedByTruncation || m_reply != activeReply || m_reply == nullptr) return;

    QNetworkReply* reply = m_reply;
    m_reply.clear();
    m_speedBytesPerSec.store(0, std::memory_order_relaxed);
    if (reply->error() == QNetworkReply::OperationCanceledError) { reply->deleteLater(); return; }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const qint64 cur = m_currentOffset.load(std::memory_order_acquire);
    const qint64 end = m_endOffset.load(std::memory_order_acquire);
    const bool tailEof = (httpStatus == 416 && cur > m_startOffset.load(std::memory_order_relaxed) && end >= cur && (end - cur) < 262144);
    if (tailEof) { m_endOffset.store(cur - 1, std::memory_order_release); }

    const bool rangeSatisfied = tailEof || ((end >= 0) ? (cur > end) : (reply->error() == QNetworkReply::NoError && cur > 0));
    if (rangeSatisfied) {
        m_state = core::SegmentState::Completed; m_statusText = QStringLiteral("Finished");
        reply->deleteLater();
        emit stateUpdated(m_segmentId); emit segmentFinished(m_segmentId);
        return;
    }
    const bool isYoutubeChunkDone = (reply->error() == QNetworkReply::NoError && httpStatus < 400) && (end >= 0 && cur <= end) &&
        m_url.host().contains(QStringLiteral("googlevideo.com"), Qt::CaseInsensitive);
    const QString errStr = (reply->error() != QNetworkReply::NoError)
        ? QStringLiteral("HTTP %1: %2").arg(httpStatus).arg(reply->errorString())
        : QStringLiteral("Stream EOF at byte %1/%2 (HTTP %3)").arg(cur).arg(end).arg(httpStatus);
    reply->deleteLater();
    if (isYoutubeChunkDone && m_sharedNam != nullptr && m_state != core::SegmentState::Paused) {
        m_retryCount = 0;
        QTimer::singleShot(0, this, [this]() {
            if (m_state != core::SegmentState::Paused && m_sharedNam != nullptr) { start(m_url, m_profile, m_sharedNam); }
        });
        return;
    }
    emit logEvent(m_segmentId, QStringLiteral("[Warn] Segment #%1 (%2) - Retry %3/15").arg(m_segmentId).arg(errStr).arg(m_retryCount + 1));
    if (m_retryCount < 15 && m_sharedNam != nullptr && m_state != core::SegmentState::Paused) {
        ++m_retryCount;
        m_state = core::SegmentState::Connecting; m_statusText = QStringLiteral("Retrying (%1/15)...").arg(m_retryCount);
        emit stateUpdated(m_segmentId);
        QTimer::singleShot(1000, this, [this]() {
            if (m_state != core::SegmentState::Paused && m_sharedNam != nullptr) { start(m_url, m_profile, m_sharedNam); }
        });
        return;
    }
    m_state = core::SegmentState::Error; m_statusText = errStr;
    emit stateUpdated(m_segmentId); emit segmentFailed(m_segmentId, errStr);
}

void SegmentWorker::updateSpeedMetric(qint64 newlyWritten) {
    m_bytesInWindow += newlyWritten;
    const qint64 elapsedMs = m_rateTimer.elapsed();
    if (elapsedMs >= 450) {
        m_speedBytesPerSec.store((m_bytesInWindow * 1000) / std::max<qint64>(1, elapsedMs), std::memory_order_relaxed);
        m_bytesInWindow = 0; m_rateTimer.restart();
    }
}

void SegmentWorker::detachReply() {
    if (m_reply != nullptr) {
        m_reply->disconnect(this); m_reply->abort(); m_reply->deleteLater(); m_reply.clear();
    }
}

} // namespace idm::network
