#pragma once

#include "core/SpeedLimiter.h"
#include "core/Types.h"
#include "network/DownloadTask.h"
#include "network/RequestProfile.h"
#include "security/SecurityTypes.h"
#include "storage/DatabaseManager.h"
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTime>
#include <QTimer>

namespace idm::network {

struct QueueConfig {
    QString name;
    int maxConcurrent = 4;
    bool scheduleEnabled = false;
    QTime startTime{1, 0};
    QTime stopTime{7, 0};
    bool isRunning = true;
};

class QueueScheduler final : public QObject {
    Q_OBJECT

public:
    explicit QueueScheduler(
        storage::DatabaseManager* dbManager,
        const security::RuntimeCapabilityToken* capabilityToken = nullptr,
        QObject* parent = nullptr
    );
    ~QueueScheduler() override;

    void loadPersistedTasks();

    DownloadTask* addDownload(
        const core::TaskMetrics& initialMetrics,
        const RequestProfile& profile,
        bool startImmediately = true
    );

    void startTask(const QString& taskId);
    void pauseTask(const QString& taskId);
    void stopAllTasks();
    void removeTask(const QString& taskId, bool deleteDiskFiles = false);

    void startQueue(const QString& queueName);
    void stopQueue(const QString& queueName);
    void setQueueConfig(const QueueConfig& config);
    [[nodiscard]] QueueConfig queueConfig(const QString& queueName) const;
    [[nodiscard]] QList<QueueConfig> allQueues() const;

    [[nodiscard]] DownloadTask* findTask(const QString& taskId) const;
    [[nodiscard]] const QList<DownloadTask*>& tasks() const noexcept { return m_tasks; }
    [[nodiscard]] core::SpeedLimiter* globalSpeedLimiter() noexcept { return &m_globalLimiter; }
    [[nodiscard]] qint64 totalActiveSpeedBytesPerSec() const noexcept;

signals:
    void taskAdded(int index, idm::network::DownloadTask* task);
    void taskUpdated(int index, idm::network::DownloadTask* task);
    void taskRemoved(int index, const QString& taskId);
    void aggregateSpeedUpdated(qint64 bytesPerSec, int activeCount);

private:
    void connectTaskSignals(DownloadTask* task);
    void evaluateQueues();
    void checkScheduleWindow();
    void persistActiveTasks();

    storage::DatabaseManager* m_dbManager{nullptr};
    const security::RuntimeCapabilityToken* m_capabilityToken{nullptr};
    core::SpeedLimiter m_globalLimiter;
    QList<DownloadTask*> m_tasks;
    QMap<QString, QueueConfig> m_queues;
    QTimer m_scheduleTimer;
    QTimer m_persistTimer;
};

} // namespace idm::network
