#include "ui/MainWindow.h"
#include "config/AppConfig.h"
#include "core/Types.h"
#include "i18n/I18nManager.h"
#include "system/WindowsSystemIntegration.h"
#include "ui/dialogs/AddDownloadDialog.h"
#include "ui/dialogs/BatchDownloadDialog.h"
#include "ui/dialogs/DownloadCompleteDialog.h"
#include "ui/dialogs/DownloadProgressDialog.h"
#include "ui/security/LicenseDialog.h"
#include "ui/settings/SettingsDialog.h"
#include "ui/theme/AppStyleSheet.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"
#include <QApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace idm::ui {

using config::AppConfig;
using theme::AppStyleSheet;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(i18n::trText(QStringLiteral("app.title"), QStringLiteral("IDM Vibe")));
    setWindowIcon(IconRegistry::icon(IconId::CategoryAll, 20));
    setMinimumSize(780, 480);
    const QRect avail = QGuiApplication::primaryScreen()
        ? QGuiApplication::primaryScreen()->availableGeometry() : QRect(0, 0, 1280, 760);
    resize(std::clamp(int(avail.width() * 0.85), 820, 1260), std::clamp(int(avail.height() * 0.84), 520, 750));

    (void)m_dbManager.initialize();
    m_licenseCoordinator.initialize();
    m_scheduler = new network::QueueScheduler(&m_dbManager, &m_licenseCoordinator.capabilityToken(), this);
    (void)m_ipcServer.startListening(AppConfig::instance().ipcPort());

    setupUi();
    setupStatusBar();
    wireSignals();
    wireTraySignals();
    m_trayController.showTrayIcon();
    applySavedPreferences();

    m_toolbar->updateLicenseBadge(m_licenseCoordinator.currentTier(), m_licenseCoordinator.statusBadgeText());
    m_scheduler->loadPersistedTasks();
    for (network::DownloadTask* t : m_scheduler->tasks()) { bindTaskPopups(t); }
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_forceQuit && AppConfig::instance().closeToTray()) {
        event->ignore(); hide();
        m_trayController.showNotification(QStringLiteral("IDM Vibe"),
            i18n::I18nManager::instance().isRightToLeft()
                ? QStringLiteral("يعمل البرنامج في الخلفية بجانب الساعة لالتقاط التحميلات.")
                : QStringLiteral("IDM Vibe is minimized to the system tray and monitoring downloads."));
        return;
    }
    m_scheduler->stopAllTasks();
    system::WindowsSystemIntegration::setPreventSystemSleep(false);
    event->accept();
}
void MainWindow::handleSecondaryInstancePayload(const QString& payload) {
    showNormal(); raise(); activateWindow();
    if (payload.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) || payload.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        openAddDownloadDialog(payload.trimmed());
    }
}
void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    m_toolbar = new widgets::TopToolbar(central);
    rootLayout->addWidget(m_toolbar);
    auto* mainHorizSplitter = new QSplitter(Qt::Horizontal, central);
    mainHorizSplitter->setChildrenCollapsible(false);
    m_sidebar = new widgets::SidebarTree(mainHorizSplitter);
    mainHorizSplitter->addWidget(m_sidebar);
    auto* rightVertSplitter = new QSplitter(Qt::Vertical, mainHorizSplitter);
    rightVertSplitter->setChildrenCollapsible(false);

    m_tableModel = new models::DownloadTableModel(m_scheduler, this);
    m_tableDelegate = new delegates::DownloadItemDelegate(this);
    m_tableView = new QTableView(rightVertSplitter);
    m_tableView->setModel(m_tableModel);
    m_tableView->setItemDelegate(m_tableDelegate);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->setMouseTracking(true);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(ThemeTokens::kRowHeightPx);

    QHeaderView* hh = m_tableView->horizontalHeader();
    hh->setFixedHeight(ThemeTokens::kHeaderHeightPx);
    hh->setHighlightSections(false);
    hh->setStretchLastSection(false);
    hh->setMinimumSectionSize(75);
    const int widths[] = {320, 100, 105, 185, 130, 110, 85, 80, 105};
    for (int col = 0; col < 9; ++col) { m_tableView->setColumnWidth(col, widths[col]); }
    hh->setSectionResizeMode(models::DownloadTableModel::ColFileName, QHeaderView::Stretch);
    rightVertSplitter->addWidget(m_tableView);
    m_inspectorDock = new widgets::InspectorDock(rightVertSplitter);
    rightVertSplitter->addWidget(m_inspectorDock);
    rightVertSplitter->setStretchFactor(0, 3);
    rightVertSplitter->setStretchFactor(1, 2);
    rightVertSplitter->setSizes({460, 240});
    mainHorizSplitter->addWidget(rightVertSplitter);
    mainHorizSplitter->setStretchFactor(0, 0);
    mainHorizSplitter->setStretchFactor(1, 1);
    mainHorizSplitter->setSizes({230, 1050});
    rootLayout->addWidget(mainHorizSplitter, 1);
    setCentralWidget(central);
}
void MainWindow::setupStatusBar() {
    auto* sb = statusBar();
    sb->setSizeGripEnabled(true);
    m_lblIpcStatus = new QLabel(this);
    m_lblIpcStatus->setFont(ThemeTokens::monoFont(false));
    m_lblIpcStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_lblIpcStatus->setMinimumWidth(0);
    m_lblIpcStatus->setText(m_ipcServer.isListening()
        ? QStringLiteral("CHROME IPC 127.0.0.1:%1 [LISTENING]  |  VIBETUBE ENGINE [READY]  |  HWID: %2")
              .arg(m_ipcServer.serverPort()).arg(m_licenseCoordinator.currentHwid().compositeFingerprint())
        : QStringLiteral("CHROME IPC [UNAVAILABLE]  |  SQLite WAL [READY]"));
    sb->addWidget(m_lblIpcStatus, 1);
    m_lblActiveTasks = new QLabel(QStringLiteral("ACTIVE TASKS: 0"), this);
    m_lblActiveTasks->setFont(ThemeTokens::monoFont(true));
    sb->addPermanentWidget(m_lblActiveTasks);
}
void MainWindow::wireSignals() {
    connect(m_toolbar, &widgets::TopToolbar::addUrlClicked, this, [this]() { openAddDownloadDialog(); });
    connect(m_toolbar, &widgets::TopToolbar::batchDownloadClicked, this, &MainWindow::openBatchDownloadDialog);
    connect(m_toolbar, &widgets::TopToolbar::resumeSelectedClicked, this, &MainWindow::resumeSelectedTask);
    connect(m_toolbar, &widgets::TopToolbar::pauseSelectedClicked, this, &MainWindow::pauseSelectedTask);
    connect(m_toolbar, &widgets::TopToolbar::stopAllClicked, m_scheduler, &network::QueueScheduler::stopAllTasks);
    connect(m_toolbar, &widgets::TopToolbar::deleteSelectedClicked, this, [this]() { deleteSelectedTask(false); });
    connect(m_toolbar, &widgets::TopToolbar::openFolderClicked, this, &MainWindow::openSelectedTaskFolder);
    connect(m_toolbar, &widgets::TopToolbar::verifyHashClicked, this, &MainWindow::verifySelectedTaskHash);
    connect(m_toolbar, &widgets::TopToolbar::themeToggleClicked, this, &MainWindow::toggleDayDarkTheme);
    connect(m_toolbar, &widgets::TopToolbar::settingsClicked, this, &MainWindow::openSettingsDialog);
    connect(m_toolbar, &widgets::TopToolbar::licenseBadgeClicked, this, &MainWindow::openLicenseDialog);
    connect(m_toolbar, &widgets::TopToolbar::globalSpeedLimitChanged, this, [this](qint64 kib) {
        m_scheduler->globalSpeedLimiter()->setLimitKiBPerSec(kib);
        AppConfig::instance().setGlobalSpeedLimitKiB(kib);
    });
    connect(m_toolbar, &widgets::TopToolbar::searchQueryChanged, m_tableModel, &models::DownloadTableModel::setSearchText);
    connect(&m_licenseCoordinator, &idm::security::LicenseCoordinator::licenseStateChanged, this, [this]() {
        m_toolbar->updateLicenseBadge(m_licenseCoordinator.currentTier(), m_licenseCoordinator.statusBadgeText());
    });

    connect(m_sidebar, &widgets::SidebarTree::categorySelected, m_tableModel, &models::DownloadTableModel::setFilterCategory);
    connect(m_sidebar, &widgets::SidebarTree::queueSelected, m_tableModel, &models::DownloadTableModel::setFilterQueue);
    connect(m_sidebar, &widgets::SidebarTree::startQueueRequested, m_scheduler, &network::QueueScheduler::startQueue);
    connect(m_sidebar, &widgets::SidebarTree::stopQueueRequested, m_scheduler, &network::QueueScheduler::stopQueue);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_tableView, &QTableView::customContextMenuRequested, this, &MainWindow::onTableContextMenu);
    connect(m_tableView, &QTableView::doubleClicked, this, [this](const QModelIndex&) {
        if (network::DownloadTask* t = selectedTask()) { (new dialogs::DownloadProgressDialog(t, this))->show(); }
    });
    connect(m_scheduler, &network::QueueScheduler::taskUpdated, this, [this](int, network::DownloadTask* t) {
        if (t == selectedTask()) { onSelectionChanged(); }
    });
    connect(m_scheduler, &network::QueueScheduler::aggregateSpeedUpdated, this, [this](qint64 speedBps, int activeCount) {
        m_toolbar->updateAggregateSpeed(speedBps, activeCount);
        m_inspectorDock->pushGlobalSpeedSample(speedBps);
        m_lblActiveTasks->setText(QStringLiteral("ACTIVE TASKS: %1").arg(activeCount));
        m_trayController.updateSpeedTooltip(core::FormatUtils::formatSpeed(speedBps), activeCount);
        if (AppConfig::instance().preventSystemSleep()) {
            system::WindowsSystemIntegration::setPreventSystemSleep(activeCount > 0);
        }
    });
    connect(&m_clipboardWatcher, &ipc::ClipboardWatcher::downloadUrlDetected, this, [this](const QString& url) { openAddDownloadDialog(url); });
    connect(&m_ipcServer, &ipc::BrowserIpcServer::browserDownloadIntercepted, this,
        [this](const QString& url, const QString& suggestedName, const network::RequestProfile& prof) {
            openAddDownloadDialog(url, prof, suggestedName);
        });
}

void MainWindow::wireTraySignals() {
    connect(&m_trayController, &system::TrayController::showMainWindowRequested, this, [this]() { showNormal(); raise(); activateWindow(); });
    connect(&m_trayController, &system::TrayController::addUrlRequested, this, [this]() { showNormal(); raise(); activateWindow(); openAddDownloadDialog(); });
    connect(&m_trayController, &system::TrayController::resumeAllRequested, this, [this]() { m_scheduler->startQueue(QStringLiteral("Main Queue")); });
    connect(&m_trayController, &system::TrayController::stopAllRequested, m_scheduler, &network::QueueScheduler::stopAllTasks);
    connect(&m_trayController, &system::TrayController::speedLimitPresetSelected, this, [this](qint64 kib) {
        AppConfig::instance().setGlobalSpeedLimitKiB(kib);
        m_scheduler->globalSpeedLimiter()->setLimitKiBPerSec(kib);
        m_toolbar->setSpeedLimitValue(kib);
    });
    connect(&m_trayController, &system::TrayController::exitApplicationRequested, this, [this]() {
        m_forceQuit = true; close(); QApplication::quit();
    });
}

void MainWindow::bindTaskPopups(network::DownloadTask* task) {
    if (!task) return;
    connect(task, &network::DownloadTask::stateChanged, this, [this, task](const QString&, core::TaskState st) {
        if (st == core::TaskState::Completed && AppConfig::instance().showCompletionDialog()) {
            (new dialogs::DownloadCompleteDialog(task->metrics(), this))->show();
        }
    });
}

void MainWindow::applySavedPreferences() {
    const AppConfig& cfg = AppConfig::instance();
    setWindowTitle(i18n::trText(QStringLiteral("app.title"), QStringLiteral("IDM Vibe")));
    m_clipboardWatcher.setEnabled(cfg.monitorClipboard());
    m_scheduler->globalSpeedLimiter()->setLimitKiBPerSec(cfg.globalSpeedLimitKiB());
    m_toolbar->setSpeedLimitValue(cfg.globalSpeedLimitKiB());
    m_toolbar->refreshThemeIcons();
    m_trayController.retranslateMenu();
}

void MainWindow::toggleDayDarkTheme() {
    const bool nextDay = !ThemeTokens::isDayMode();
    AppConfig::instance().setDayMode(nextDay);
    AppStyleSheet::setDayMode(nextDay);
    m_toolbar->refreshThemeIcons();
}
void MainWindow::openSettingsDialog() {
    settings::SettingsDialog dlg(this);
    connect(&dlg, &settings::SettingsDialog::settingsApplied, this, &MainWindow::applySavedPreferences);
    dlg.exec();
    m_toolbar->refreshThemeIcons();
}
void MainWindow::openAddDownloadDialog(const QString& prefillUrl, const network::RequestProfile& profile, const QString& suggestedFileName) {
    dialogs::AddDownloadDialog dlg(prefillUrl, profile, suggestedFileName, this);
    if (dlg.exec() == QDialog::Accepted) {
        network::DownloadTask* task = m_scheduler->addDownload(dlg.configuredMetrics(), dlg.configuredProfile(), dlg.startImmediately());
        bindTaskPopups(task);
        m_sidebar->refreshDiskTelemetry(task->metrics().saveDirectory);
        const int row = m_tableModel->rowForTaskId(task->taskId());
        if (row >= 0) { m_tableView->selectRow(row); }
        if (dlg.startImmediately() && AppConfig::instance().showLiveProgressDialog()) {
            (new dialogs::DownloadProgressDialog(task, this))->show();
        }
    }
}
void MainWindow::openBatchDownloadDialog() {
    dialogs::BatchDownloadDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        for (const QString& u : dlg.expandedUrls()) {
            core::TaskMetrics m;
            m.url = u; m.saveDirectory = dlg.saveDirectory(); m.queueName = dlg.selectedQueue(); m.maxSegments = dlg.maxSegments();
            bindTaskPopups(m_scheduler->addDownload(m, network::RequestProfile{}, false));
        }
        m_scheduler->startQueue(dlg.selectedQueue());
    }
}
void MainWindow::openLicenseDialog() { security::LicenseDialog dlg(&m_licenseCoordinator, this); dlg.exec(); }
void MainWindow::onSelectionChanged() {
    network::DownloadTask* task = selectedTask();
    m_inspectorDock->inspectTask(task);
    if (!task) { m_toolbar->setTaskActionsEnabled(false, false, false); return; }
    const core::TaskState st = task->state();
    const bool canStart = (st == core::TaskState::Paused || st == core::TaskState::Queued || st == core::TaskState::Failed || st == core::TaskState::Canceled);
    const bool canPause = (st == core::TaskState::Active || st == core::TaskState::Probing);
    m_toolbar->setTaskActionsEnabled(canStart, canPause, true);
}
void MainWindow::onTableContextMenu(const QPoint& pos) {
    const QModelIndex idx = m_tableView->indexAt(pos);
    if (!idx.isValid()) return;
    m_tableView->selectRow(idx.row());
    if (!selectedTask()) return;
    QMenu menu(this);
    connect(menu.addAction(IconRegistry::icon(IconId::StartPlay, 14), QStringLiteral("Resume Download")), &QAction::triggered, this, &MainWindow::resumeSelectedTask);
    connect(menu.addAction(IconRegistry::icon(IconId::Pause, 14), QStringLiteral("Pause Download")), &QAction::triggered, this, &MainWindow::pauseSelectedTask);
    menu.addSeparator();
    connect(menu.addAction(IconRegistry::icon(IconId::FolderOpen, 14), QStringLiteral("Open Save Directory")), &QAction::triggered, this, &MainWindow::openSelectedTaskFolder);
    connect(menu.addAction(IconRegistry::icon(IconId::ShieldVerify, 14), QStringLiteral("Verify SHA-256 / MD5")), &QAction::triggered, this, &MainWindow::verifySelectedTaskHash);
    menu.addSeparator();
    connect(menu.addAction(IconRegistry::icon(IconId::TrashDelete, 14), QStringLiteral("Remove from List")), &QAction::triggered, this, [this]() { deleteSelectedTask(false); });
    connect(menu.addAction(IconRegistry::icon(IconId::TrashDelete, 14), QStringLiteral("Delete Task & Disk File")), &QAction::triggered, this, [this]() { deleteSelectedTask(true); });
    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::resumeSelectedTask() { if (auto* t = selectedTask()) m_scheduler->startTask(t->taskId()); }
void MainWindow::pauseSelectedTask() { if (auto* t = selectedTask()) m_scheduler->pauseTask(t->taskId()); }
void MainWindow::deleteSelectedTask(bool deleteDiskFile) { if (auto* t = selectedTask()) { m_scheduler->removeTask(t->taskId(), deleteDiskFile); onSelectionChanged(); } }
void MainWindow::openSelectedTaskFolder() { if (auto* t = selectedTask()) QDesktopServices::openUrl(QUrl::fromLocalFile(t->metrics().saveDirectory)); }
void MainWindow::verifySelectedTaskHash() { if (auto* t = selectedTask()) t->runHashVerification(); }
network::DownloadTask* MainWindow::selectedTask() const {
    const QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    return sel.isEmpty() ? nullptr : m_tableModel->taskAtRow(sel.first().row());
}

} // namespace idm::ui
