#include "storage/DatabaseManager.h"

#include <QDir>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

namespace idm::storage {

DatabaseManager::DatabaseManager()
    : m_connectionName(QStringLiteral("idm_vibe_sql_") + QUuid::createUuid().toString(QUuid::Id128)) {}

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::initialize(const QString& customDbPath) {
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        return true;
    }

    QString dbPath = customDbPath;
    if (dbPath.isEmpty()) {
        QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (appData.isEmpty()) {
            appData = QDir::homePath() + QStringLiteral("/.idm-vibe");
        }
        QDir().mkpath(appData);
        dbPath = QDir(appData).filePath(QStringLiteral("idm_vibe_state.db"));
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL;"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL;"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON;"));

    m_initialized = createTablesLocked();
    return m_initialized;
}

void DatabaseManager::close() {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return;
    }
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_initialized = false;
}

bool DatabaseManager::createTablesLocked() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);

    const bool okTasks = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "  task_id TEXT PRIMARY KEY, url TEXT NOT NULL, file_name TEXT NOT NULL,"
        "  save_dir TEXT NOT NULL, queue_name TEXT NOT NULL, category INTEGER NOT NULL,"
        "  state INTEGER NOT NULL, total_bytes INTEGER NOT NULL, downloaded_bytes INTEGER NOT NULL,"
        "  max_segments INTEGER NOT NULL, resume_supported INTEGER NOT NULL,"
        "  expected_hash TEXT, computed_sha256 TEXT, computed_md5 TEXT,"
        "  error_message TEXT, request_profile_json TEXT, created_at TEXT NOT NULL"
        ");"
    ));

    const bool okSegments = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS segments ("
        "  task_id TEXT NOT NULL, segment_id INTEGER NOT NULL,"
        "  start_offset INTEGER NOT NULL, current_offset INTEGER NOT NULL,"
        "  end_offset INTEGER NOT NULL, state INTEGER NOT NULL,"
        "  PRIMARY KEY (task_id, segment_id),"
        "  FOREIGN KEY (task_id) REFERENCES tasks(task_id) ON DELETE CASCADE"
        ");"
    ));

    return okTasks && okSegments;
}

bool DatabaseManager::saveTask(
    const core::TaskMetrics& metrics,
    const network::RequestProfile& profile
) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO tasks ("
        "  task_id, url, file_name, save_dir, queue_name, category, state,"
        "  total_bytes, downloaded_bytes, max_segments, resume_supported,"
        "  expected_hash, computed_sha256, computed_md5, error_message,"
        "  request_profile_json, created_at"
        ") VALUES ("
        "  :task_id, :url, :file_name, :save_dir, :queue_name, :category, :state,"
        "  :total_bytes, :downloaded_bytes, :max_segments, :resume_supported,"
        "  :expected_hash, :computed_sha256, :computed_md5, :error_message,"
        "  :request_profile_json, :created_at"
        ");"
    ));

    query.bindValue(QStringLiteral(":task_id"), metrics.taskId);
    query.bindValue(QStringLiteral(":url"), metrics.url);
    query.bindValue(QStringLiteral(":file_name"), metrics.fileName);
    query.bindValue(QStringLiteral(":save_dir"), metrics.saveDirectory);
    query.bindValue(QStringLiteral(":queue_name"), metrics.queueName);
    query.bindValue(QStringLiteral(":category"), static_cast<int>(metrics.category));
    query.bindValue(QStringLiteral(":state"), static_cast<int>(metrics.state));
    query.bindValue(QStringLiteral(":total_bytes"), metrics.totalBytes);
    query.bindValue(QStringLiteral(":downloaded_bytes"), metrics.downloadedBytes);
    query.bindValue(QStringLiteral(":max_segments"), metrics.maxSegments);
    query.bindValue(QStringLiteral(":resume_supported"), metrics.resumeSupported ? 1 : 0);
    query.bindValue(QStringLiteral(":expected_hash"), metrics.expectedHash);
    query.bindValue(QStringLiteral(":computed_sha256"), metrics.computedSha256);
    query.bindValue(QStringLiteral(":computed_md5"), metrics.computedMd5);
    query.bindValue(QStringLiteral(":error_message"), metrics.errorMessage);
    query.bindValue(QStringLiteral(":request_profile_json"), profile.toJsonString());
    query.bindValue(QStringLiteral(":created_at"), metrics.createdAt.toString(Qt::ISODate));

    if (!query.exec()) {
        return false;
    }

    locker.unlock();
    return saveSegments(metrics.taskId, metrics.segments);
}

bool DatabaseManager::saveSegments(
    const QString& taskId,
    const QList<core::SegmentInfo>& segments
) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    if (!db.transaction()) {
        return false;
    }

    QSqlQuery delQuery(db);
    delQuery.prepare(QStringLiteral("DELETE FROM segments WHERE task_id = :task_id;"));
    delQuery.bindValue(QStringLiteral(":task_id"), taskId);
    if (!delQuery.exec()) {
        db.rollback();
        return false;
    }

    QSqlQuery insQuery(db);
    insQuery.prepare(QStringLiteral(
        "INSERT INTO segments (task_id, segment_id, start_offset, current_offset, end_offset, state)"
        " VALUES (:task_id, :segment_id, :start_offset, :current_offset, :end_offset, :state);"
    ));

    for (const core::SegmentInfo& seg : segments) {
        insQuery.bindValue(QStringLiteral(":task_id"), taskId);
        insQuery.bindValue(QStringLiteral(":segment_id"), seg.id);
        insQuery.bindValue(QStringLiteral(":start_offset"), seg.startOffset);
        insQuery.bindValue(QStringLiteral(":current_offset"), seg.currentOffset);
        insQuery.bindValue(QStringLiteral(":end_offset"), seg.endOffset);
        insQuery.bindValue(QStringLiteral(":state"), static_cast<int>(seg.state));
        if (!insQuery.exec()) {
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

bool DatabaseManager::deleteTask(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM tasks WHERE task_id = :task_id;"));
    query.bindValue(QStringLiteral(":task_id"), taskId);
    return query.exec();
}

QList<PersistedTaskRecord> DatabaseManager::loadAllTasks() const {
    QMutexLocker locker(&m_mutex);
    QList<PersistedTaskRecord> records;
    if (!m_initialized) {
        return records;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(QStringLiteral("SELECT * FROM tasks ORDER BY created_at DESC;"), db);

    while (query.next()) {
        PersistedTaskRecord rec;
        rec.metrics.taskId = query.value(QStringLiteral("task_id")).toString();
        rec.metrics.url = query.value(QStringLiteral("url")).toString();
        rec.metrics.fileName = query.value(QStringLiteral("file_name")).toString();
        rec.metrics.saveDirectory = query.value(QStringLiteral("save_dir")).toString();
        rec.metrics.queueName = query.value(QStringLiteral("queue_name")).toString();
        rec.metrics.category = static_cast<core::FileCategory>(query.value(QStringLiteral("category")).toInt());

        auto savedState = static_cast<core::TaskState>(query.value(QStringLiteral("state")).toInt());
        if (savedState == core::TaskState::Active || savedState == core::TaskState::Probing ||
            savedState == core::TaskState::Verifying) {
            savedState = core::TaskState::Paused;
        }
        rec.metrics.state = savedState;
        rec.metrics.totalBytes = query.value(QStringLiteral("total_bytes")).toLongLong();
        rec.metrics.downloadedBytes = query.value(QStringLiteral("downloaded_bytes")).toLongLong();
        rec.metrics.maxSegments = query.value(QStringLiteral("max_segments")).toInt();
        rec.metrics.resumeSupported = query.value(QStringLiteral("resume_supported")).toInt() != 0;
        rec.metrics.expectedHash = query.value(QStringLiteral("expected_hash")).toString();
        rec.metrics.computedSha256 = query.value(QStringLiteral("computed_sha256")).toString();
        rec.metrics.computedMd5 = query.value(QStringLiteral("computed_md5")).toString();
        rec.metrics.errorMessage = query.value(QStringLiteral("error_message")).toString();
        rec.metrics.createdAt = QDateTime::fromString(query.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
        rec.profile = network::RequestProfile::fromJsonString(query.value(QStringLiteral("request_profile_json")).toString());
        rec.metrics.segments = loadSegmentsLocked(rec.metrics.taskId);
        records.append(rec);
    }
    return records;
}

QList<core::SegmentInfo> DatabaseManager::loadSegmentsLocked(const QString& taskId) const {
    QList<core::SegmentInfo> segments;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT segment_id, start_offset, current_offset, end_offset, state "
        "FROM segments WHERE task_id = :task_id ORDER BY segment_id ASC;"
    ));
    query.bindValue(QStringLiteral(":task_id"), taskId);
    if (!query.exec()) {
        return segments;
    }

    while (query.next()) {
        core::SegmentInfo seg;
        seg.id = query.value(0).toInt();
        seg.startOffset = query.value(1).toLongLong();
        seg.currentOffset = query.value(2).toLongLong();
        seg.endOffset = query.value(3).toLongLong();
        auto st = static_cast<core::SegmentState>(query.value(4).toInt());
        if (st == core::SegmentState::Connecting || st == core::SegmentState::Downloading) {
            st = core::SegmentState::Paused;
        }
        seg.state = st;
        seg.statusText = core::FormatUtils::segmentStateToString(st);
        segments.append(seg);
    }
    return segments;
}

} // namespace idm::storage
