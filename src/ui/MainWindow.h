#pragma once

#include "ipc/BrowserIpcServer.h"
#include "ipc/ClipboardWatcher.h"
#include "network/QueueScheduler.h"
#include "security/LicenseCoordinator.h"
#include "storage/DatabaseManager.h"
#include "system/TrayController.h"
#include "ui/delegates/DownloadItemDelegate.h"
#include "ui/models/DownloadTableModel.h"
#include "ui/widgets/InspectorDock.h"
#include "ui/widgets/SidebarTree.h"
#include "ui/widgets/TopToolbar.h"
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QTableView>

namespace idm::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    void handleSecondaryInstancePayload(const QString& payload);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupStatusBar();
    void wireSignals();
    void wireTraySignals();
    void applySavedPreferences();
    void bindTaskPopups(network::DownloadTask* task);

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
    system::TrayController m_trayController;
    bool m_forceQuit{false};

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
