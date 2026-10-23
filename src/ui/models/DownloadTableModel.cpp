#include "ui/models/DownloadTableModel.h"
#include "ui/theme/ThemeTokens.h"

namespace idm::ui::models {

DownloadTableModel::DownloadTableModel(
    network::QueueScheduler* scheduler,
    QObject* parent
)
    : QAbstractTableModel(parent)
    , m_scheduler(scheduler) {
    rebuildVisibleList();

    connect(m_scheduler, &network::QueueScheduler::taskAdded, this,
        [this](int, network::DownloadTask*) { rebuildVisibleList(); });

    connect(m_scheduler, &network::QueueScheduler::taskRemoved, this,
        [this](int, const QString&) { rebuildVisibleList(); });

    connect(m_scheduler, &network::QueueScheduler::taskUpdated, this,
        [this](int, network::DownloadTask* task) {
            const int row = static_cast<int>(m_visibleTasks.indexOf(task));
            if (row >= 0) {
                emit dataChanged(
                    index(row, 0),
                    index(row, ColumnCount - 1),
                    {Qt::DisplayRole, RoleTaskState, RoleProgressPercent, RoleFileName}
                );
            } else if (matchesFilters(task)) {
                rebuildVisibleList();
            }
        });
}

int DownloadTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_visibleTasks.size());
}

int DownloadTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant DownloadTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleTasks.size()) {
        return {};
    }

    const network::DownloadTask* task = m_visibleTasks.at(index.row());
    const core::TaskMetrics& m = task->metrics();

    if (role == RoleTaskId)          return m.taskId;
    if (role == RoleTaskState)       return static_cast<int>(m.state);
    if (role == RoleCategory)        return static_cast<int>(m.category);
    if (role == RoleProgressPercent) return m.progressPercent();
    if (role == RoleFileName)        return m.fileName;
    if (role == RoleSavePath)        return m.fullSavePath();
    if (role == RoleErrorMessage)    return m.errorMessage;

    if (role == Qt::FontRole) {
        if (index.column() == ColFileName || index.column() == ColStatus || index.column() == ColQueue) {
            return theme::ThemeTokens::uiFont(false);
        }
        return theme::ThemeTokens::monoFont(false);
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case ColSize:
            case ColDownloaded:
            case ColSpeed:
            case ColEta:
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            case ColSegments:
                return static_cast<int>(Qt::AlignCenter);
            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (role == Qt::ToolTipRole) {
        if (!m.errorMessage.isEmpty()) {
            return QStringLiteral("%1\nURL: %2\nError: %3").arg(m.fullSavePath(), m.url, m.errorMessage);
        }
        return QStringLiteral("%1\nURL: %2").arg(m.fullSavePath(), m.url);
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColFileName:
                return m.fileName;
            case ColSize:
                return m.totalBytes > 0 ? core::FormatUtils::formatBytes(m.totalBytes)
                                        : QStringLiteral("--");
            case ColDownloaded:
                return core::FormatUtils::formatBytes(m.downloadedBytes);
            case ColProgress:
                return QStringLiteral("%1%").arg(m.progressPercent(), 0, 'f', 1);
            case ColStatus:
                return core::FormatUtils::stateToString(m.state);
            case ColSpeed:
                return m.state == core::TaskState::Active
                           ? core::FormatUtils::formatSpeed(m.speedBytesPerSec)
                           : QStringLiteral("--");
            case ColEta:
                return m.state == core::TaskState::Active
                           ? core::FormatUtils::formatEta(m.etaSeconds)
                           : QStringLiteral("--:--");
            case ColSegments:
                return QStringLiteral("%1/%2")
                    .arg(m.segments.size())
                    .arg(m.maxSegments);
            case ColQueue:
                return m.queueName;
            default:
                return {};
        }
    }

    return {};
}

QVariant DownloadTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
        case ColFileName:   return QStringLiteral("File Name");
        case ColSize:       return QStringLiteral("Total Size");
        case ColDownloaded: return QStringLiteral("Downloaded");
        case ColProgress:   return QStringLiteral("Progress");
        case ColStatus:     return QStringLiteral("Status");
        case ColSpeed:      return QStringLiteral("Transfer Rate");
        case ColEta:        return QStringLiteral("ETA");
        case ColSegments:   return QStringLiteral("Segments");
        case ColQueue:      return QStringLiteral("Queue");
        default:            return {};
    }
}

void DownloadTableModel::setFilterCategory(core::FileCategory category) {
    m_filterCategory = category;
    m_filterQueue.clear();
    rebuildVisibleList();
}

void DownloadTableModel::setFilterQueue(const QString& queueName) {
    m_filterQueue = queueName;
    rebuildVisibleList();
}

void DownloadTableModel::setSearchText(const QString& query) {
    m_searchQuery = query.trimmed();
    rebuildVisibleList();
}

network::DownloadTask* DownloadTableModel::taskAtRow(int row) const {
    if (row < 0 || row >= m_visibleTasks.size()) {
        return nullptr;
    }
    return m_visibleTasks.at(row);
}

int DownloadTableModel::rowForTaskId(const QString& taskId) const {
    for (int i = 0; i < m_visibleTasks.size(); ++i) {
        if (m_visibleTasks.at(i)->taskId() == taskId) {
            return i;
        }
    }
    return -1;
}

void DownloadTableModel::rebuildVisibleList() {
    beginResetModel();
    m_visibleTasks.clear();
    if (m_scheduler != nullptr) {
        for (network::DownloadTask* task : m_scheduler->tasks()) {
            if (matchesFilters(task)) {
                m_visibleTasks.append(task);
            }
        }
    }
    endResetModel();
}

bool DownloadTableModel::matchesFilters(const network::DownloadTask* task) const {
    if (task == nullptr) {
        return false;
    }
    const core::TaskMetrics& m = task->metrics();
    if (!m_filterQueue.isEmpty()) {
        if (m.queueName != m_filterQueue) {
            return false;
        }
    } else if (m_filterCategory != core::FileCategory::All) {
        if (m.category != m_filterCategory) {
            return false;
        }
    }

    if (!m_searchQuery.isEmpty()) {
        if (!m.fileName.contains(m_searchQuery, Qt::CaseInsensitive) &&
            !m.url.contains(m_searchQuery, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

} // namespace idm::ui::models
