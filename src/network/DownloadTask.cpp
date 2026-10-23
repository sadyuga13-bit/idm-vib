#include "network/DownloadTask.h"
#include "storage/CategoryRouter.h"

#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <algorithm>

namespace idm::network {

DownloadTask::DownloadTask(const core::TaskMetrics& initialMetrics, const RequestProfile& profile,
    core::SpeedLimiter* globalLimiter, const security::RuntimeCapabilityToken* capabilityToken, QObject* parent)
    : QObject(parent), m_metrics(initialMetrics), m_profile(profile)
    , m_globalLimiter(globalLimiter), m_capabilityToken(capabilityToken) {
    m_profile.applyProxyToManager(&m_nam);
    m_metricsTimer.setInterval(250);

    connect(&m_ytExtractor, &YoutubeExtractor::extractionCompleted, this, &DownloadTask::onYoutubeExtracted);
    connect(&m_prober, &UrlProber::probeCompleted, this, &DownloadTask::onProbeCompleted);
    connect(&m_metricsTimer, &QTimer::timeout, this, &DownloadTask::onTickMetrics);
    connect(&m_hashVerifier, &core::HashVerifier::verificationFinished, this,
        [this](const QString&, const QString& sha256, const QString& md5, bool matched, const QString& err) {
            if (!err.isEmpty() || !matched) {
                m_metrics.errorMessage = err.isEmpty()
                    ? QStringLiteral("Checksum mismatch against expected hash.") : err;
                appendLog(QStringLiteral("[VerifyError] %1").arg(m_metrics.errorMessage));
                setState(core::TaskState::Failed);
                return;
            }
            m_metrics.computedSha256 = sha256;
            m_metrics.computedMd5 = md5;
            appendLog(QStringLiteral("[Integrity] SHA-256: %1 | MD5: %2").arg(sha256, md5));
            setState(core::TaskState::Completed);
        });
}

DownloadTask::~DownloadTask() { pause(); }

void DownloadTask::start() {
    if (m_metrics.state == core::TaskState::Active || m_metrics.state == core::TaskState::Probing ||
        m_metrics.state == core::TaskState::Verifying || m_metrics.state == core::TaskState::Completed) {
        return;
    }
    m_metrics.errorMessage.clear();
    m_profile.applyProxyToManager(&m_nam);

    const QUrl targetUrl(m_metrics.url);
    if (YoutubeExtractor::isYoutubeUrl(targetUrl)) {
        setState(core::TaskState::Probing);
        appendLog(QStringLiteral("[YouTube] Resolving Innertube direct stream for %1").arg(m_metrics.url));
        m_ytExtractor.extractVideoInfo(targetUrl);
        return;
    }
    if (m_metrics.totalBytes > 0 && m_metrics.resumeSupported) {
        initializeSegmentsAndStart(!m_metrics.segments.isEmpty());
        return;
    }
    setState(core::TaskState::Probing);
    appendLog(QStringLiteral("[Prober] Probing endpoint %1").arg(m_metrics.url));
    m_prober.probeUrl(targetUrl, m_profile);
}

void DownloadTask::pause() {
    m_ytExtractor.abort();
    m_prober.abort();
    m_metricsTimer.stop();
    for (SegmentWorker* worker : m_workers) { worker->pause(); }
    m_diskWriter.flush();
    m_metrics.speedBytesPerSec = 0;
    m_metrics.etaSeconds = -1;
    if (m_metrics.state == core::TaskState::Active || m_metrics.state == core::TaskState::Probing) {
        onTickMetrics();
        appendLog(QStringLiteral("[Task] Paused at %1").arg(core::FormatUtils::formatBytes(m_metrics.downloadedBytes)));
        setState(core::TaskState::Paused);
    }
}

void DownloadTask::cancel(bool deleteFiles) {
    m_ytExtractor.abort();
    m_prober.abort();
    m_hashVerifier.cancel();
    m_metricsTimer.stop();
    for (SegmentWorker* worker : m_workers) { worker->pause(); }
    qDeleteAll(m_workers);
    m_workers.clear();
    m_diskWriter.close();
    if (deleteFiles) {
        m_diskWriter.removePartFile();
        QFile::remove(m_metrics.fullSavePath());
    }
    m_metrics.speedBytesPerSec = 0;
    m_metrics.etaSeconds = -1;
    setState(core::TaskState::Canceled);
}

void DownloadTask::runHashVerification() {
    if (!QFile::exists(m_metrics.fullSavePath())) { return; }
    setState(core::TaskState::Verifying);
    appendLog(QStringLiteral("[Verifier] Computing SHA-256 & MD5 for %1").arg(m_metrics.fileName));
    m_hashVerifier.verifyFileAsync(m_metrics.taskId, m_metrics.fullSavePath(), m_metrics.expectedHash);
}

void DownloadTask::setTaskSpeedLimitKiB(qint64 kibPerSec) { m_taskLimiter.setLimitKiBPerSec(kibPerSec); }
qint64 DownloadTask::taskSpeedLimitKiB() const noexcept { return m_taskLimiter.limitKiBPerSec(); }

void DownloadTask::onYoutubeExtracted(const YoutubeVideoInfo& info) {
    if (!info.success || info.formats.isEmpty()) {
        m_metrics.errorMessage = info.errorMessage;
        appendLog(QStringLiteral("[YouTubeError] %1").arg(info.errorMessage));
        setState(core::TaskState::Failed);
        return;
    }
    const YoutubeFormat& fmt = info.formats.first();
    m_metrics.url = fmt.directUrl;
    if (!fmt.userAgent.isEmpty()) {
        m_profile.userAgent = fmt.userAgent;
    }
    m_metrics.fileName = YoutubeExtractor::sanitizeVideoFileName(info.title, fmt.qualityTag, fmt.fileExtension);
    m_metrics.category = storage::CategoryRouter::classifyFileName(m_metrics.fileName);
    if (fmt.contentLength > 0) {
        m_metrics.totalBytes = fmt.contentLength;
        m_metrics.resumeSupported = true;
        initializeSegmentsAndStart(false);
    } else {
        m_prober.probeUrl(QUrl(m_metrics.url), m_profile);
    }
}

void DownloadTask::onProbeCompleted(const ProbeResult& result) {
    if (!result.success) {
        m_metrics.errorMessage = result.errorMessage;
        appendLog(QStringLiteral("[ProbeError] %1").arg(result.errorMessage));
        setState(core::TaskState::Failed);
        return;
    }
    if (result.finalUrl.isValid()) { m_metrics.url = result.finalUrl.toString(); }
    if (m_metrics.fileName.isEmpty() && !result.suggestedFileName.isEmpty()) {
        m_metrics.fileName = result.suggestedFileName;
        m_metrics.category = storage::CategoryRouter::classifyFileName(m_metrics.fileName);
    }
    m_metrics.totalBytes = std::max<qint64>(0, result.contentLength);
    m_metrics.resumeSupported = result.supportsRanges && (m_metrics.totalBytes > 0);
    appendLog(QStringLiteral("[Prober] HTTP %1 | Size: %2 | Range: %3")
        .arg(result.httpStatusCode).arg(core::FormatUtils::formatBytes(m_metrics.totalBytes))
        .arg(m_metrics.resumeSupported ? QStringLiteral("Yes") : QStringLiteral("No")));
    initializeSegmentsAndStart(false);
}

void DownloadTask::initializeSegmentsAndStart(bool isResuming) {
    QString diskErr;
    const bool preservePart = isResuming && m_metrics.resumeSupported;
    if (!m_diskWriter.openAndPreallocate(m_metrics.fullSavePath(), m_metrics.totalBytes, preservePart, &diskErr)) {
        m_metrics.errorMessage = diskErr;
        appendLog(QStringLiteral("[DiskError] %1").arg(diskErr));
        setState(core::TaskState::Failed);
        return;
    }
    if (!preservePart || m_metrics.segments.isEmpty()) {
        m_metrics.segments.clear();
        m_metrics.downloadedBytes = 0;
        const int tokenPermitted = (m_capabilityToken != nullptr)
            ? m_capabilityToken->resolvePermittedSegments(m_metrics.maxSegments)
            : std::clamp(m_metrics.maxSegments, 1, 32);
        const int segCount = (!m_metrics.resumeSupported || m_metrics.totalBytes < 512 * 1024) ? 1 : tokenPermitted;
        if (m_metrics.totalBytes > 0) {
            const qint64 chunk = m_metrics.totalBytes / segCount;
            for (int i = 0; i < segCount; ++i) {
                m_metrics.segments.append(core::SegmentInfo{i + 1, i * chunk, i * chunk,
                    (i == segCount - 1) ? (m_metrics.totalBytes - 1) : ((i + 1) * chunk - 1),
                    0, core::SegmentState::Idle, {}});
            }
        } else {
            m_metrics.segments.append(core::SegmentInfo{1, 0, 0, -1, 0, core::SegmentState::Idle, {}});
        }
    }
    spawnWorkersFromMetrics();
}

void DownloadTask::spawnWorkersFromMetrics() {
    qDeleteAll(m_workers);
    m_workers.clear();
    m_bytesInSpeedWindow = 0;
    m_speedWindowTimer.start();
    setState(core::TaskState::Active);

    const QUrl targetUrl(m_metrics.url);
    for (const core::SegmentInfo& seg : m_metrics.segments) {
        auto* worker = new SegmentWorker(
            seg.id, seg.startOffset, seg.currentOffset, seg.endOffset,
            &m_diskWriter, m_globalLimiter, &m_taskLimiter, this);
        connect(worker, &SegmentWorker::bytesWritten, this, &DownloadTask::onSegmentBytesWritten);
        connect(worker, &SegmentWorker::segmentFinished, this, &DownloadTask::onSegmentFinished);
        connect(worker, &SegmentWorker::segmentFailed, this, &DownloadTask::onSegmentFailed);
        m_workers.append(worker);
        worker->start(targetUrl, m_profile, &m_nam);
    }
    m_metricsTimer.start();
    appendLog(QStringLiteral("[Engine] Dispatched %1 concurrent segment(s).").arg(m_workers.size()));
}

void DownloadTask::onSegmentBytesWritten(int, qint64 deltaBytes) {
    m_metrics.downloadedBytes += deltaBytes;
    m_bytesInSpeedWindow += deltaBytes;
}

void DownloadTask::onSegmentFinished(int segmentId) {
    SegmentWorker* idleWorker = nullptr;
    SegmentWorker* largestWorker = nullptr;
    qint64 maxRem = 0;
    for (SegmentWorker* w : m_workers) {
        if (w->segmentId() == segmentId) { idleWorker = w; }
        else if (w->state() == core::SegmentState::Downloading && w->remainingBytes() > maxRem) {
            maxRem = w->remainingBytes();
            largestWorker = w;
        }
    }
    const qint64 splitMinThreshold = (m_capabilityToken != nullptr)
        ? m_capabilityToken->resolveInFlightSplitThreshold() : 1048576LL;
    if (m_metrics.resumeSupported && idleWorker != nullptr && largestWorker != nullptr && maxRem >= splitMinThreshold) {
        const SplitRange split = largestWorker->tryStealHalf(splitMinThreshold);
        if (split.valid) {
            appendLog(QStringLiteral("[InFlightSplit] Segment #%1 stole [%2 - %3] from Segment #%4")
                .arg(idleWorker->segmentId()).arg(split.newStartOffset).arg(split.newEndOffset).arg(largestWorker->segmentId()));
            idleWorker->reassignRange(split.newStartOffset, split.newEndOffset, QUrl(m_metrics.url), m_profile, &m_nam);
            onTickMetrics();
            return;
        }
    }
    const bool allDone = std::all_of(m_workers.cbegin(), m_workers.cend(), [](const SegmentWorker* w) {
        return w->state() == core::SegmentState::Completed;
    });
    onTickMetrics();
    if (allDone) { finalizeDownload(); }
}

void DownloadTask::onSegmentFailed(int segmentId, const QString& errorMessage) {
    m_metricsTimer.stop();
    for (SegmentWorker* w : m_workers) {
        if (w->segmentId() != segmentId) { w->pause(); }
    }
    m_diskWriter.flush();
    m_metrics.speedBytesPerSec = 0;
    m_metrics.etaSeconds = -1;
    m_metrics.errorMessage = QStringLiteral("Segment #%1: %2").arg(segmentId).arg(errorMessage);
    appendLog(QStringLiteral("[Error] %1").arg(m_metrics.errorMessage));
    onTickMetrics();
    setState(core::TaskState::Failed);
}

void DownloadTask::onTickMetrics() {
    m_metrics.segments.clear();
    for (const SegmentWorker* w : m_workers) {
        m_metrics.segments.append(w->snapshot());
    }
    const qint64 elapsedMs = m_speedWindowTimer.elapsed();
    if (elapsedMs >= 400) {
        m_metrics.speedBytesPerSec = (m_bytesInSpeedWindow * 1000) / std::max<qint64>(1, elapsedMs);
        m_bytesInSpeedWindow = 0;
        m_speedWindowTimer.restart();
    }
    m_metrics.etaSeconds = (m_metrics.speedBytesPerSec > 0 && m_metrics.totalBytes > m_metrics.downloadedBytes)
        ? ((m_metrics.totalBytes - m_metrics.downloadedBytes) / m_metrics.speedBytesPerSec) : -1;
    emit metricsUpdated(m_metrics.taskId);
}

void DownloadTask::finalizeDownload() {
    m_metricsTimer.stop();
    m_metrics.speedBytesPerSec = 0;
    m_metrics.etaSeconds = 0;
    if (m_metrics.totalBytes <= 0) { m_metrics.totalBytes = m_metrics.downloadedBytes; }
    QString finalizeErr;
    if (!m_diskWriter.finalizeFile(&finalizeErr)) {
        m_metrics.errorMessage = finalizeErr;
        appendLog(QStringLiteral("[FinalizeError] %1").arg(finalizeErr));
        setState(core::TaskState::Failed);
        return;
    }
    appendLog(QStringLiteral("[DiskWriter] Finalized file %1").arg(m_metrics.fullSavePath()));
    runHashVerification();
}
void DownloadTask::setState(core::TaskState newState) {
    m_metrics.state = newState;
    emit stateChanged(m_metrics.taskId, newState);
    emit metricsUpdated(m_metrics.taskId);
}
void DownloadTask::appendLog(const QString& message) {
    const QString line = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")), message);
    m_logs.append(line);
    if (m_logs.size() > 500) { m_logs.removeFirst(); }
    emit logAdded(m_metrics.taskId, line);
}
} // namespace idm::network
