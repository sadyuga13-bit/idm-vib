#pragma once

#include "core/Types.h"
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>

namespace idm::ui::widgets {

class SidebarTree final : public QWidget {
    Q_OBJECT

public:
    explicit SidebarTree(QWidget* parent = nullptr);
    ~SidebarTree() override = default;

    void refreshDiskTelemetry(const QString& path = QString());

signals:
    void categorySelected(idm::core::FileCategory category);
    void queueSelected(const QString& queueName);
    void startQueueRequested(const QString& queueName);
    void stopQueueRequested(const QString& queueName);

private:
    void populateTree();
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

    QTreeWidget* m_tree{nullptr};
    QLabel* m_lblDrivePath{nullptr};
    QLabel* m_lblDiskNumbers{nullptr};
    QProgressBar* m_diskBar{nullptr};
    QPushButton* m_btnStartQueue{nullptr};
    QPushButton* m_btnStopQueue{nullptr};
    QString m_activeQueueName{QStringLiteral("Main Queue")};
};

} // namespace idm::ui::widgets
