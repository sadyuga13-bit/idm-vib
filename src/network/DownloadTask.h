#pragma once

#include "core/HashVerifier.h"
#include "core/SpeedLimiter.h"
#include "core/Types.h"
#include "network/RequestProfile.h"
#include "network/SegmentWorker.h"
#include "network/UrlProber.h"
#include "network/YoutubeExtractor.h"
#include "security/SecurityTypes.h"
#include "storage/DiskWriter.h"
#include <QElapsedTimer>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QTimer>

namespace idm::network {

class DownloadTask final : public QObject {
    Q_OBJECT

public:
    DownloadTask(
        const core::TaskMetrics& initialMetrics,
        const RequestProfile& profile,
        core::SpeedLimiter* globalLimiter,
        const security::RuntimeCapabilityToken* capabilityToken = nullptr,
        QObject* parent = nullptr
    );
    ~DownloadTask() override;

    void start();
    void pause();
    void cancel(bool deleteFiles = false);
    void runHashVerification();

    void setTaskSpeedLimitKiB(qint64 kibPerSec);
    [[nodiscard]] qint64 taskSpeedLimitKiB() const noexcept;

    [[nodiscard]] QString taskId() const { return m_metrics.taskId; }
    [[nodiscard]] core::TaskState state() const noexcept { return m_metrics.state; }
    [[nodiscard]] const core::TaskMetrics& metrics() const noexcept { return m_metrics; }
    [[nodiscard]] const RequestProfile& requestProfile() const noexcept { return m_profile; }
    [[nodiscard]] const QStringList& logs() const noexcept { return m_logs; }

signals:
    void metricsUpdated(const QString& taskId);
    void stateChanged(const QString& taskId, idm::core::TaskState state);
    void logAdded(const QString& taskId, const QString& formattedLine);

private:
    void onYoutubeExtracted(const YoutubeVideoInfo& info);
    void onProbeCompleted(const ProbeResult& result);
    void initializeSegmentsAndStart(bool isResuming);
    void spawnWorkersFromMetrics();
    void onSegmentBytesWritten(int segmentId, qint64 deltaBytes);
    void onSegmentFinished(int segmentId);
    void onSegmentFailed(int segmentId, const QString& errorMessage);
    void onTickMetrics();
    void finalizeDownload();
    void setState(core::TaskState newState);
    void appendLog(const QString& message);

    core::TaskMetrics m_metrics;
    RequestProfile m_profile;
    core::SpeedLimiter* m_globalLimiter{nullptr};
    const security::RuntimeCapabilityToken* m_capabilityToken{nullptr};
    core::SpeedLimiter m_taskLimiter;

    QNetworkAccessManager m_nam;
    UrlProber m_prober;
    YoutubeExtractor m_ytExtractor;
    storage::DiskWriter m_diskWriter;
    core::HashVerifier m_hashVerifier;

    QList<SegmentWorker*> m_workers;
    QTimer m_metricsTimer;
    QElapsedTimer m_speedWindowTimer;
    qint64 m_bytesInSpeedWindow{0};
    QStringList m_logs;
};

} // namespace idm::network
