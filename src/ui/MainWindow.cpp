#include "ui/MainWindow.h"
#include "core/AppSettings.h"
#include "ui/dialogs/AddDownloadDialog.h"
#include "ui/dialogs/BatchDownloadDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/security/LicenseDialog.h"
#include "ui/theme/AppStyleSheet.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QDesktopServices>
#include <QHeaderView>
#include <QMenu>
#include <QSplitter>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

namespace idm::ui {

using core::AppSettings;
using theme::AppStyleSheet;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("IDM Vibe - Native C++20 Multi-Segment & YouTube Download Engine"));
    setWindowIcon(IconRegistry::icon(IconId::CategoryAll, 20));
    resize(1280, 760);

    (void)m_dbManager.initialize();
    m_licenseCoordinator.initialize();
    m_scheduler = new network::QueueScheduler(
        &m_dbManager, &m_licenseCoordinator.capabilityToken(), this);
    (void)m_ipcServer.startListening(AppSettings::instance().ipcPort());

    setupUi();
    setupStatusBar();
    wireSignals();
    applySavedPreferences();

    m_toolbar->updateLicenseBadge(
        m_licenseCoordinator.currentTier(), m_licenseCoordinator.statusBadgeText());
    m_scheduler->loadPersistedTasks();
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
    hh->setStretchLastSection(true);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColFileName, 260);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColSize, 95);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColDownloaded, 95);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColProgress, 165);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColStatus, 110);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColSpeed, 105);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColEta, 80);
    m_tableView->setColumnWidth(models::DownloadTableModel::ColSegments, 75);
    rightVertSplitter->addWidget(m_tableView);

    m_inspectorDock = new widgets::InspectorDock(rightVertSplitter);
    rightVertSplitter->addWidget(m_inspectorDock);
    rightVertSplitter->setStretchFactor(0, 3);
    rightVertSplitter->setStretchFactor(1, 2);

    mainHorizSplitter->addWidget(rightVertSplitter);
    mainHorizSplitter->setStretchFactor(0, 0);
    mainHorizSplitter->setStretchFactor(1, 1);

    rootLayout->addWidget(mainHorizSplitter, 1);
    setCentralWidget(central);
}

void MainWindow::setupStatusBar() {
    auto* sb = statusBar();
    sb->setSizeGripEnabled(false);

    m_lblIpcStatus = new QLabel(this);
    m_lblIpcStatus->setFont(ThemeTokens::monoFont(false));
    m_lblIpcStatus->setText(m_ipcServer.isListening()
        ? QStringLiteral("CHROME IPC 127.0.0.1:%1 [LISTENING]  |  YOUTUBE INNERTUBE [READY]  |  HWID: %2")
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
        AppSettings::instance().setGlobalSpeedLimitKiB(kib);
    });
    connect(m_toolbar, &widgets::TopToolbar::searchQueryChanged,
            m_tableModel, &models::DownloadTableModel::setSearchText);

    connect(&m_licenseCoordinator, &idm::security::LicenseCoordinator::licenseStateChanged, this, [this]() {
        m_toolbar->updateLicenseBadge(
            m_licenseCoordinator.currentTier(), m_licenseCoordinator.statusBadgeText());
    });

    connect(m_sidebar, &widgets::SidebarTree::categorySelected,
            m_tableModel, &models::DownloadTableModel::setFilterCategory);
    connect(m_sidebar, &widgets::SidebarTree::queueSelected,
            m_tableModel, &models::DownloadTableModel::setFilterQueue);
    connect(m_sidebar, &widgets::SidebarTree::startQueueRequested,
            m_scheduler, &network::QueueScheduler::startQueue);
    connect(m_sidebar, &widgets::SidebarTree::stopQueueRequested,
            m_scheduler, &network::QueueScheduler::stopQueue);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::onTableContextMenu);

    connect(m_scheduler, &network::QueueScheduler::taskUpdated, this, [this](int, network::DownloadTask* t) {
        if (t == selectedTask()) { onSelectionChanged(); }
    });
    connect(m_scheduler, &network::QueueScheduler::aggregateSpeedUpdated, this,
        [this](qint64 speedBps, int activeCount) {
            m_toolbar->updateAggregateSpeed(speedBps, activeCount);
            m_inspectorDock->pushGlobalSpeedSample(speedBps);
            m_lblActiveTasks->setText(QStringLiteral("ACTIVE TASKS: %1").arg(activeCount));
        });

    connect(&m_clipboardWatcher, &ipc::ClipboardWatcher::downloadUrlDetected, this, [this](const QString& url) {
        raise();
        activateWindow();
        openAddDownloadDialog(url);
    });
    connect(&m_ipcServer, &ipc::BrowserIpcServer::browserDownloadIntercepted, this,
        [this](const QString& url, const QString& suggestedName, const network::RequestProfile& prof) {
            raise();
            activateWindow();
            openAddDownloadDialog(url, prof, suggestedName);
        });
}

void MainWindow::applySavedPreferences() {
    const AppSettings& cfg = AppSettings::instance();
    m_clipboardWatcher.setEnabled(cfg.clipboardMonitoringEnabled());
    m_scheduler->globalSpeedLimiter()->setLimitKiBPerSec(cfg.globalSpeedLimitKiB());
    m_toolbar->setSpeedLimitValue(cfg.globalSpeedLimitKiB());
    m_toolbar->refreshThemeIcons();
}
void MainWindow::toggleDayDarkTheme() {
    AppStyleSheet::setDayMode(!ThemeTokens::isDayMode());
    m_toolbar->refreshThemeIcons();
}
void MainWindow::openSettingsDialog() {
    dialogs::SettingsDialog dlg(this);
    connect(&dlg, &dialogs::SettingsDialog::settingsApplied, this, &MainWindow::applySavedPreferences);
    dlg.exec();
    m_toolbar->refreshThemeIcons();
}
void MainWindow::openAddDownloadDialog(const QString& prefillUrl, const network::RequestProfile& profile, const QString& suggestedFileName) {
    dialogs::AddDownloadDialog dlg(prefillUrl, profile, suggestedFileName, this);
    if (dlg.exec() == QDialog::Accepted) {
        network::DownloadTask* task = m_scheduler->addDownload(
            dlg.configuredMetrics(), dlg.configuredProfile(), dlg.startImmediately());
        m_sidebar->refreshDiskTelemetry(task->metrics().saveDirectory);
        const int row = m_tableModel->rowForTaskId(task->taskId());
        if (row >= 0) { m_tableView->selectRow(row); }
    }
}
void MainWindow::openBatchDownloadDialog() {
    dialogs::BatchDownloadDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        for (const QString& u : dlg.expandedUrls()) {
            core::TaskMetrics m;
            m.url = u;
            m.saveDirectory = dlg.saveDirectory();
            m.queueName = dlg.selectedQueue();
            m.maxSegments = dlg.maxSegments();
            m_scheduler->addDownload(m, network::RequestProfile{}, false);
        }
        m_scheduler->startQueue(dlg.selectedQueue());
    }
}
void MainWindow::openLicenseDialog() {
    security::LicenseDialog dlg(&m_licenseCoordinator, this);
    dlg.exec();
}

void MainWindow::onSelectionChanged() {
    network::DownloadTask* task = selectedTask();
    m_inspectorDock->inspectTask(task);
    if (task == nullptr) {
        m_toolbar->setTaskActionsEnabled(false, false, false);
        return;
    }
    const core::TaskState st = task->state();
    const bool canStart = (st == core::TaskState::Paused || st == core::TaskState::Queued ||
                           st == core::TaskState::Failed || st == core::TaskState::Canceled);
    const bool canPause = (st == core::TaskState::Active || st == core::TaskState::Probing);
    m_toolbar->setTaskActionsEnabled(canStart, canPause, true);
}

void MainWindow::onTableContextMenu(const QPoint& pos) {
    const QModelIndex idx = m_tableView->indexAt(pos);
    if (!idx.isValid()) { return; }
    m_tableView->selectRow(idx.row());
    if (selectedTask() == nullptr) { return; }

    QMenu menu(this);
    QAction* actResume = menu.addAction(IconRegistry::icon(IconId::StartPlay, 14), QStringLiteral("Resume Download"));
    QAction* actPause = menu.addAction(IconRegistry::icon(IconId::Pause, 14), QStringLiteral("Pause Download"));
    menu.addSeparator();
    QAction* actFolder = menu.addAction(IconRegistry::icon(IconId::FolderOpen, 14), QStringLiteral("Open Save Directory"));
    QAction* actVerify = menu.addAction(IconRegistry::icon(IconId::ShieldVerify, 14), QStringLiteral("Verify SHA-256 / MD5"));
    menu.addSeparator();
    QAction* actRemove = menu.addAction(IconRegistry::icon(IconId::TrashDelete, 14), QStringLiteral("Remove from List"));
    QAction* actDeleteDisk = menu.addAction(IconRegistry::icon(IconId::TrashDelete, 14), QStringLiteral("Delete Task & Disk File"));

    connect(actResume, &QAction::triggered, this, &MainWindow::resumeSelectedTask);
    connect(actPause, &QAction::triggered, this, &MainWindow::pauseSelectedTask);
    connect(actFolder, &QAction::triggered, this, &MainWindow::openSelectedTaskFolder);
    connect(actVerify, &QAction::triggered, this, &MainWindow::verifySelectedTaskHash);
    connect(actRemove, &QAction::triggered, this, [this]() { deleteSelectedTask(false); });
    connect(actDeleteDisk, &QAction::triggered, this, [this]() { deleteSelectedTask(true); });
    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::resumeSelectedTask() {
    if (network::DownloadTask* t = selectedTask()) { m_scheduler->startTask(t->taskId()); }
}
void MainWindow::pauseSelectedTask() {
    if (network::DownloadTask* t = selectedTask()) { m_scheduler->pauseTask(t->taskId()); }
}
void MainWindow::deleteSelectedTask(bool deleteDiskFile) {
    if (network::DownloadTask* t = selectedTask()) {
        m_scheduler->removeTask(t->taskId(), deleteDiskFile);
        onSelectionChanged();
    }
}
void MainWindow::openSelectedTaskFolder() {
    if (network::DownloadTask* t = selectedTask()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(t->metrics().saveDirectory));
    }
}
void MainWindow::verifySelectedTaskHash() {
    if (network::DownloadTask* t = selectedTask()) { t->runHashVerification(); }
}
network::DownloadTask* MainWindow::selectedTask() const {
    const QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    return sel.isEmpty() ? nullptr : m_tableModel->taskAtRow(sel.first().row());
}

} // namespace idm::ui
