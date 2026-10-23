#pragma once

#include "core/Types.h"
#include "network/RequestProfile.h"
#include <QList>
#include <QMutex>
#include <QSqlDatabase>
#include <QString>

namespace idm::storage {

struct PersistedTaskRecord {
    core::TaskMetrics metrics;
    network::RequestProfile profile;
};

class DatabaseManager final {
public:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    [[nodiscard]] bool initialize(const QString& customDbPath = QString());
    void close();

    bool saveTask(
        const core::TaskMetrics& metrics,
        const network::RequestProfile& profile
    );
    bool saveSegments(
        const QString& taskId,
        const QList<core::SegmentInfo>& segments
    );
    bool deleteTask(const QString& taskId);

    [[nodiscard]] QList<PersistedTaskRecord> loadAllTasks() const;

private:
    bool createTablesLocked();
    [[nodiscard]] QList<core::SegmentInfo> loadSegmentsLocked(const QString& taskId) const;

    mutable QMutex m_mutex;
    QString m_connectionName;
    bool m_initialized{false};
};

} // namespace idm::storage
