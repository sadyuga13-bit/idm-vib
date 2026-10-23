#pragma once

#include "ipc/BrowserIpcServer.h"
#include "ipc/ClipboardWatcher.h"
#include "network/QueueScheduler.h"
#include "security/LicenseCoordinator.h"
#include "storage/DatabaseManager.h"
#include "ui/delegates/DownloadItemDelegate.h"
#include "ui/models/DownloadTableModel.h"
#include "ui/widgets/InspectorDock.h"
#include "ui/widgets/SidebarTree.h"
#include "ui/widgets/TopToolbar.h"
#include <QLabel>
#include <QMainWindow>
#include <QTableView>

namespace idm::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUi();
    void setupStatusBar();
    void wireSignals();
    void applySavedPreferences();

    void openAddDownloadDialog(
        const QString& prefillUrl = QString(),
        const network::RequestProfile& profile = network::RequestProfile{},
        const QString& suggestedFileName = QString()
    );
    void openBatchDownloadDialog();
    void openSettingsDialog();
    void openLicenseDialog();
    void toggleDayDarkTheme();

    void onSelectionChanged();
    void onTableContextMenu(const QPoint& pos);
    void resumeSelectedTask();
    void pauseSelectedTask();
    void deleteSelectedTask(bool deleteDiskFile = false);
    void openSelectedTaskFolder();
    void verifySelectedTaskHash();

    [[nodiscard]] network::DownloadTask* selectedTask() const;

    storage::DatabaseManager m_dbManager;
    idm::security::LicenseCoordinator m_licenseCoordinator;
    network::QueueScheduler* m_scheduler{nullptr};
    ipc::BrowserIpcServer m_ipcServer;
    ipc::ClipboardWatcher m_clipboardWatcher;

    widgets::TopToolbar* m_toolbar{nullptr};
    widgets::SidebarTree* m_sidebar{nullptr};
    QTableView* m_tableView{nullptr};
    models::DownloadTableModel* m_tableModel{nullptr};
    delegates::DownloadItemDelegate* m_tableDelegate{nullptr};
    widgets::InspectorDock* m_inspectorDock{nullptr};

    QLabel* m_lblIpcStatus{nullptr};
    QLabel* m_lblActiveTasks{nullptr};
};

} // namespace idm::ui
