#pragma once

#include "core/SpeedLimiter.h"
#include "core/Types.h"
#include "network/RequestProfile.h"
#include "storage/DiskWriter.h"
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <atomic>

namespace idm::network {

struct SplitRange {
    bool valid = false;
    qint64 newStartOffset = 0;
    qint64 newEndOffset = 0;
};

class SegmentWorker final : public QObject {
    Q_OBJECT

public:
    SegmentWorker(
        int segmentId,
        qint64 startOffset,
        qint64 currentOffset,
        qint64 endOffset,
        storage::DiskWriter* diskWriter,
        core::SpeedLimiter* globalLimiter,
        core::SpeedLimiter* taskLimiter,
        QObject* parent = nullptr
    );
    ~SegmentWorker() override;

    void start(
        const QUrl& url,
        const RequestProfile& profile,
        QNetworkAccessManager* sharedNam
    );
    void pause();
    void reassignRange(
        qint64 startOffset,
        qint64 endOffset,
        const QUrl& url,
        const RequestProfile& profile,
        QNetworkAccessManager* sharedNam
    );

    [[nodiscard]] SplitRange tryStealHalf(qint64 minRemainingBytes = 1048576);

    [[nodiscard]] core::SegmentInfo snapshot() const;
    [[nodiscard]] int segmentId() const noexcept { return m_segmentId; }
    [[nodiscard]] core::SegmentState state() const noexcept { return m_state; }
    [[nodiscard]] qint64 remainingBytes() const noexcept;
    [[nodiscard]] bool isRangeFullyDownloaded() const noexcept;

signals:
    void bytesWritten(int segmentId, qint64 deltaBytes);
    void segmentFinished(int segmentId);
    void segmentFailed(int segmentId, const QString& errorMessage);
    void stateUpdated(int segmentId);

private:
    void drainAvailableBytes();
    void handleReplyFinished();
    void updateSpeedMetric(qint64 newlyWritten);
    void detachReply();

    int m_segmentId;
    std::atomic<qint64> m_startOffset;
    std::atomic<qint64> m_currentOffset;
    std::atomic<qint64> m_endOffset;
    std::atomic<qint64> m_speedBytesPerSec{0};

    core::SegmentState m_state{core::SegmentState::Idle};
    QString m_statusText{QStringLiteral("Idle")};

    storage::DiskWriter* m_diskWriter{nullptr};
    core::SpeedLimiter* m_globalLimiter{nullptr};
    core::SpeedLimiter* m_taskLimiter{nullptr};

    QUrl m_url;
    RequestProfile m_profile;
    QPointer<QNetworkAccessManager> m_sharedNam;
    QPointer<QNetworkReply> m_reply;
    QTimer m_throttleTimer;
    QElapsedTimer m_rateTimer;
    qint64 m_bytesInWindow{0};
    int m_retryCount{0};
    bool m_completedByTruncation{false};
};

} // namespace idm::network
