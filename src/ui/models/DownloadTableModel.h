#pragma once

#include "core/Types.h"
#include "network/DownloadTask.h"
#include "network/QueueScheduler.h"
#include <QAbstractTableModel>
#include <QList>
#include <QString>

namespace idm::ui::models {

class DownloadTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column : int {
        ColFileName = 0,
        ColSize,
        ColDownloaded,
        ColProgress,
        ColStatus,
        ColSpeed,
        ColEta,
        ColSegments,
        ColQueue,
        ColumnCount
    };

    enum CustomRole : int {
        RoleTaskId = Qt::UserRole + 1,
        RoleTaskState,
        RoleCategory,
        RoleProgressPercent,
        RoleFileName,
        RoleSavePath,
        RoleErrorMessage
    };

    explicit DownloadTableModel(
        network::QueueScheduler* scheduler,
        QObject* parent = nullptr
    );
    ~DownloadTableModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
    ) const override;

    void setFilterCategory(core::FileCategory category);
    void setFilterQueue(const QString& queueName);
    void setSearchText(const QString& query);

    [[nodiscard]] network::DownloadTask* taskAtRow(int row) const;
    [[nodiscard]] int rowForTaskId(const QString& taskId) const;

private:
    void rebuildVisibleList();
    [[nodiscard]] bool matchesFilters(const network::DownloadTask* task) const;

    network::QueueScheduler* m_scheduler{nullptr};
    QList<network::DownloadTask*> m_visibleTasks;

    core::FileCategory m_filterCategory{core::FileCategory::All};
    QString m_filterQueue;
    QString m_searchQuery;
};

} // namespace idm::ui::models
