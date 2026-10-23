#include "network/QueueScheduler.h"
#include "storage/CategoryRouter.h"

#include <QDateTime>
#include <QUrl>
#include <QUuid>
#include <algorithm>

namespace idm::network {

QueueScheduler::QueueScheduler(
    storage::DatabaseManager* dbManager,
    const security::RuntimeCapabilityToken* capabilityToken,
    QObject* parent
)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_capabilityToken(capabilityToken) {
    QueueConfig mainQueue;
    mainQueue.name = QStringLiteral("Main Queue");
    mainQueue.maxConcurrent = 4;
    mainQueue.scheduleEnabled = false;
    mainQueue.isRunning = true;
    m_queues.insert(mainQueue.name, mainQueue);

    QueueConfig nightQueue;
    nightQueue.name = QStringLiteral("Night Queue");
    nightQueue.maxConcurrent = 2;
    nightQueue.scheduleEnabled = true;
    nightQueue.startTime = QTime(1, 0);
    nightQueue.stopTime = QTime(7, 0);
    nightQueue.isRunning = false;
    m_queues.insert(nightQueue.name, nightQueue);

    m_scheduleTimer.setInterval(1000);
    connect(&m_scheduleTimer, &QTimer::timeout, this, &QueueScheduler::checkScheduleWindow);
    m_scheduleTimer.start();

    m_persistTimer.setInterval(2000);
    connect(&m_persistTimer, &QTimer::timeout, this, &QueueScheduler::persistActiveTasks);
    m_persistTimer.start();
}

QueueScheduler::~QueueScheduler() {
    stopAllTasks();
    persistActiveTasks();
}

void QueueScheduler::loadPersistedTasks() {
    if (m_dbManager == nullptr) {
        return;
    }
    const QList<storage::PersistedTaskRecord> records = m_dbManager->loadAllTasks();
    for (const storage::PersistedTaskRecord& rec : records) {
        auto* task = new DownloadTask(
            rec.metrics, rec.profile, &m_globalLimiter, m_capabilityToken, this);
        connectTaskSignals(task);
        const int idx = static_cast<int>(m_tasks.size());
        m_tasks.append(task);
        emit taskAdded(idx, task);
    }
}

DownloadTask* QueueScheduler::addDownload(
    const core::TaskMetrics& initialMetrics,
    const RequestProfile& profile,
    bool startImmediately
) {
    core::TaskMetrics metrics = initialMetrics;
    if (metrics.taskId.isEmpty()) {
        metrics.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (metrics.fileName.isEmpty()) {
        metrics.fileName = UrlProber::extractFileName(QUrl(metrics.url), QByteArray());
    }
    metrics.category = storage::CategoryRouter::classifyFileName(metrics.fileName);
    if (metrics.saveDirectory.isEmpty()) {
        metrics.saveDirectory = storage::CategoryRouter::defaultDirectoryForCategory(metrics.category);
    }
    if (metrics.queueName.isEmpty()) {
        metrics.queueName = QStringLiteral("Main Queue");
    }
    metrics.createdAt = QDateTime::currentDateTimeUtc();
    metrics.state = core::TaskState::Queued;

    auto* task = new DownloadTask(
        metrics, profile, &m_globalLimiter, m_capabilityToken, this);
    connectTaskSignals(task);

    const int idx = 0;
    m_tasks.prepend(task);
    if (m_dbManager != nullptr) {
        m_dbManager->saveTask(task->metrics(), task->requestProfile());
    }
    emit taskAdded(idx, task);

    if (startImmediately) {
        task->start();
    } else {
        evaluateQueues();
    }
    return task;
}

void QueueScheduler::startTask(const QString& taskId) {
    if (DownloadTask* task = findTask(taskId)) {
        task->start();
    }
}

void QueueScheduler::pauseTask(const QString& taskId) {
    if (DownloadTask* task = findTask(taskId)) {
        task->pause();
    }
}

void QueueScheduler::stopAllTasks() {
    for (DownloadTask* task : m_tasks) {
        if (task->state() == core::TaskState::Active ||
            task->state() == core::TaskState::Probing) {
            task->pause();
        }
    }
}

void QueueScheduler::removeTask(const QString& taskId, bool deleteDiskFiles) {
    for (int i = 0; i < m_tasks.size(); ++i) {
        DownloadTask* task = m_tasks.at(i);
        if (task->taskId() == taskId) {
            task->cancel(deleteDiskFiles);
            m_tasks.removeAt(i);
            if (m_dbManager != nullptr) {
                m_dbManager->deleteTask(taskId);
            }
            emit taskRemoved(i, taskId);
            task->deleteLater();
            evaluateQueues();
            return;
        }
    }
}

void QueueScheduler::startQueue(const QString& queueName) {
    if (m_queues.contains(queueName)) {
        m_queues[queueName].isRunning = true;
        evaluateQueues();
    }
}

void QueueScheduler::stopQueue(const QString& queueName) {
    if (m_queues.contains(queueName)) {
        m_queues[queueName].isRunning = false;
        for (DownloadTask* task : m_tasks) {
            if (task->metrics().queueName == queueName &&
                (task->state() == core::TaskState::Active || task->state() == core::TaskState::Probing)) {
                task->pause();
            }
        }
    }
}

void QueueScheduler::setQueueConfig(const QueueConfig& config) {
    m_queues.insert(config.name, config);
    evaluateQueues();
}

QueueConfig QueueScheduler::queueConfig(const QString& queueName) const {
    return m_queues.value(queueName, QueueConfig{queueName, 4, false, QTime(1, 0), QTime(7, 0), true});
}

QList<QueueConfig> QueueScheduler::allQueues() const {
    return m_queues.values();
}

DownloadTask* QueueScheduler::findTask(const QString& taskId) const {
    for (DownloadTask* task : m_tasks) {
        if (task->taskId() == taskId) {
            return task;
        }
    }
    return nullptr;
}

qint64 QueueScheduler::totalActiveSpeedBytesPerSec() const noexcept {
    qint64 sum = 0;
    for (const DownloadTask* task : m_tasks) {
        if (task->state() == core::TaskState::Active) {
            sum += task->metrics().speedBytesPerSec;
        }
    }
    return sum;
}

void QueueScheduler::connectTaskSignals(DownloadTask* task) {
    connect(task, &DownloadTask::metricsUpdated, this, [this, task](const QString&) {
        const int idx = static_cast<int>(m_tasks.indexOf(task));
        if (idx >= 0) {
            emit taskUpdated(idx, task);
        }
    });
    connect(task, &DownloadTask::stateChanged, this, [this, task](const QString&, core::TaskState) {
        if (m_dbManager != nullptr) {
            m_dbManager->saveTask(task->metrics(), task->requestProfile());
        }
        evaluateQueues();
    });
}

void QueueScheduler::evaluateQueues() {
    for (auto it = m_queues.cbegin(); it != m_queues.cend(); ++it) {
        const QueueConfig& cfg = it.value();
        if (!cfg.isRunning) {
            continue;
        }
        int activeInQueue = 0;
        for (const DownloadTask* task : m_tasks) {
            if (task->metrics().queueName == cfg.name &&
                (task->state() == core::TaskState::Active || task->state() == core::TaskState::Probing)) {
                ++activeInQueue;
            }
        }
        for (DownloadTask* task : m_tasks) {
            if (activeInQueue >= cfg.maxConcurrent) {
                break;
            }
            if (task->metrics().queueName == cfg.name && task->state() == core::TaskState::Queued) {
                task->start();
                ++activeInQueue;
            }
        }
    }
}

void QueueScheduler::checkScheduleWindow() {
    const QTime now = QTime::currentTime();
    for (auto it = m_queues.begin(); it != m_queues.end(); ++it) {
        QueueConfig& cfg = it.value();
        if (!cfg.scheduleEnabled) {
            continue;
        }
        const bool inWindow = (cfg.startTime <= cfg.stopTime)
            ? (now >= cfg.startTime && now < cfg.stopTime)
            : (now >= cfg.startTime || now < cfg.stopTime);
        if (inWindow && !cfg.isRunning) {
            startQueue(cfg.name);
        } else if (!inWindow && cfg.isRunning) {
            stopQueue(cfg.name);
        }
    }

    int activeCount = 0;
    qint64 totalSpeed = 0;
    for (const DownloadTask* task : m_tasks) {
        if (task->state() == core::TaskState::Active) {
            ++activeCount;
            totalSpeed += task->metrics().speedBytesPerSec;
        }
    }
    emit aggregateSpeedUpdated(totalSpeed, activeCount);
}

void QueueScheduler::persistActiveTasks() {
    if (m_dbManager == nullptr) {
        return;
    }
    for (const DownloadTask* task : m_tasks) {
        if (task->state() == core::TaskState::Active) {
            m_dbManager->saveTask(task->metrics(), task->requestProfile());
        }
    }
}

} // namespace idm::network
