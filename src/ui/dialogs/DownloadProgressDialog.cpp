#include "ui/dialogs/DownloadProgressDialog.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "system/WindowsSystemIntegration.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace idm::ui::dialogs {

using i18n::trText;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

DownloadProgressDialog::DownloadProgressDialog(network::DownloadTask* task, QWidget* parent)
    : QDialog(parent)
    , m_task(task) {
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMinimumSize(620, 460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* tabs = new QTabWidget(this);
    tabs->setIconSize(QSize(15, 15));
    tabs->addTab(buildStatusTab(), IconRegistry::icon(IconId::CpuThreads, 15),
                 trText(QStringLiteral("prog.tab.status"), QStringLiteral("Download Status")));
    tabs->addTab(buildSpeedLimiterTab(), IconRegistry::icon(IconId::SpeedGauge, 15),
                 trText(QStringLiteral("prog.tab.speed"), QStringLiteral("Speed Limiter")));
    tabs->addTab(buildCompletionOptionsTab(), IconRegistry::icon(IconId::SchedulerClock, 15),
                 trText(QStringLiteral("prog.tab.completion"), QStringLiteral("Options on Completion")));
    root->addWidget(tabs, 1);

    auto* footer = new QHBoxLayout();
    m_btnPinTop = new QPushButton(QStringLiteral("Always on Top"), this);
    m_btnPinTop->setCheckable(true);
    footer->addWidget(m_btnPinTop);
    footer->addStretch(1);

    m_btnPauseResume = new QPushButton(QStringLiteral("Pause"), this);
    m_btnPauseResume->setIcon(IconRegistry::icon(IconId::Pause, 14));
    m_btnCancelTask = new QPushButton(QStringLiteral("Cancel Download"), this);
    m_btnCancelTask->setIcon(IconRegistry::icon(IconId::StopSquare, 14));
    auto* btnHide = new QPushButton(QStringLiteral("Hide Window"), this);
    footer->addWidget(m_btnPauseResume);
    footer->addWidget(m_btnCancelTask);
    footer->addWidget(btnHide);
    root->addLayout(footer);

    connect(m_btnPinTop, &QPushButton::toggled, this, &DownloadProgressDialog::onToggleAlwaysOnTop);
    connect(m_btnPauseResume, &QPushButton::clicked, this, &DownloadProgressDialog::onPauseResumeClicked);
    connect(m_btnCancelTask, &QPushButton::clicked, this, [this]() {
        if (m_task != nullptr) { m_task->cancel(false); }
        close();
    });
    connect(btnHide, &QPushButton::clicked, this, &QDialog::close);

    if (m_task != nullptr) {
        connect(m_task, &network::DownloadTask::metricsUpdated, this, [this](const QString&) { refreshTelemetry(); });
        connect(m_task, &network::DownloadTask::stateChanged, this, [this](const QString&, core::TaskState st) {
            refreshTelemetry();
            if (st == core::TaskState::Completed && m_chkCloseOnComplete->isChecked()) {
                close();
            }
        });
        refreshTelemetry();
    }
}

DownloadProgressDialog::~DownloadProgressDialog() {
    system::WindowsSystemIntegration::clearTaskbarProgress(this);
}

QWidget* DownloadProgressDialog::buildStatusTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* infoBox = new QGroupBox(QStringLiteral("Live Transfer Telemetry"), page);
    auto* form = new QFormLayout(infoBox);
    form->setSpacing(5);

    auto makeMono = [infoBox]() {
        auto* l = new QLabel(QStringLiteral("--"), infoBox);
        l->setFont(ThemeTokens::monoFont(false));
        return l;
    };
    m_lblUrl = makeMono();
    m_lblStatus = makeMono();
    m_lblFileSize = makeMono();
    m_lblDownloaded = makeMono();
    m_lblTransferRate = makeMono();
    m_lblTimeLeft = makeMono();
    m_lblResumeCapability = makeMono();

    form->addRow(QStringLiteral("Endpoint:"), m_lblUrl);
    form->addRow(QStringLiteral("Status:"), m_lblStatus);
    form->addRow(QStringLiteral("File Size:"), m_lblFileSize);
    form->addRow(QStringLiteral("Downloaded:"), m_lblDownloaded);
    form->addRow(QStringLiteral("Transfer Rate:"), m_lblTransferRate);
    form->addRow(QStringLiteral("Time Left:"), m_lblTimeLeft);
    form->addRow(QStringLiteral("Resume Support:"), m_lblResumeCapability);
    layout->addWidget(infoBox);

    m_mainProgress = new QProgressBar(page);
    m_mainProgress->setRange(0, 1000);
    m_mainProgress->setFixedHeight(16);
    layout->addWidget(m_mainProgress);

    auto* segHeaderRow = new QHBoxLayout();
    segHeaderRow->addWidget(new QLabel(QStringLiteral("Byte Positions & Live Connections (1-32):"), page));
    segHeaderRow->addStretch(1);
    m_liveConnectionsCombo = new QComboBox(page);
    for (int c : {1, 2, 4, 8, 12, 16, 24, 32}) {
        m_liveConnectionsCombo->addItem(QStringLiteral("%1 Connections").arg(c), c);
    }
    m_liveConnectionsCombo->setCurrentIndex(3);
    connect(m_liveConnectionsCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        if (m_task != nullptr && idx >= 0) {
            m_task->setDynamicSegmentCount(m_liveConnectionsCombo->itemData(idx).toInt());
        }
    });
    segHeaderRow->addWidget(m_liveConnectionsCombo);
    layout->addLayout(segHeaderRow);

    m_segmentBar = new widgets::SegmentBarWidget(page);
    layout->addWidget(m_segmentBar);

    m_segModel = new models::SegmentTableModel(this);
    m_segTable = new QTableView(page);
    m_segTable->setModel(m_segModel);
    m_segTable->setAlternatingRowColors(true);
    m_segTable->verticalHeader()->setVisible(false);
    m_segTable->verticalHeader()->setDefaultSectionSize(22);
    m_segTable->horizontalHeader()->setFixedHeight(24);
    m_segTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_segTable, 1);
    return page;
}

QWidget* DownloadProgressDialog::buildSpeedLimiterTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* box = new QGroupBox(QStringLiteral("Per-Task Token-Bucket Rate Limiter"), page);
    auto* form = new QFormLayout(box);

    m_chkTaskSpeedLimit = new QCheckBox(QStringLiteral("Enable speed limit for this download"), box);
    m_spinTaskSpeedKiB = new QSpinBox(box);
    m_spinTaskSpeedKiB->setRange(16, 1000000);
    m_spinTaskSpeedKiB->setValue(1024);
    m_spinTaskSpeedKiB->setSuffix(QStringLiteral(" KB/s"));

    form->addRow(m_chkTaskSpeedLimit);
    form->addRow(QStringLiteral("Maximum Speed:"), m_spinTaskSpeedKiB);
    layout->addWidget(box);
    layout->addStretch(1);

    auto applyLimit = [this]() {
        if (m_task != nullptr) {
            m_task->setTaskSpeedLimitKiB(m_chkTaskSpeedLimit->isChecked() ? m_spinTaskSpeedKiB->value() : 0);
        }
    };
    connect(m_chkTaskSpeedLimit, &QCheckBox::toggled, this, applyLimit);
    connect(m_spinTaskSpeedKiB, &QSpinBox::valueChanged, this, applyLimit);
    return page;
}

QWidget* DownloadProgressDialog::buildCompletionOptionsTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* box = new QGroupBox(QStringLiteral("Post-Download Actions"), page);
    auto* form = new QFormLayout(box);

    m_chkCloseOnComplete = new QCheckBox(QStringLiteral("Close this progress window when download completes"), box);
    m_chkCloseOnComplete->setChecked(true);
    m_chkShowCompleteDlg = new QCheckBox(QStringLiteral("Show completion summary & antivirus verification dialog"), box);
    m_chkShowCompleteDlg->setChecked(config::AppConfig::instance().showCompletionDialog());

    m_powerActionCombo = new QComboBox(box);
    m_powerActionCombo->addItems({
        QStringLiteral("Do Nothing"),
        QStringLiteral("Put Computer to Sleep"),
        QStringLiteral("Hibernate System"),
        QStringLiteral("Shut Down Computer (60s Countdown)")
    });

    form->addRow(m_chkCloseOnComplete);
    form->addRow(m_chkShowCompleteDlg);
    form->addRow(QStringLiteral("System Power Action:"), m_powerActionCombo);
    layout->addWidget(box);
    layout->addStretch(1);
    return page;
}

void DownloadProgressDialog::refreshTelemetry() {
    if (m_task == nullptr) { return; }
    const core::TaskMetrics& m = m_task->metrics();
    const double pct = m.progressPercent();
    setWindowTitle(QStringLiteral("[%1% | %2] %3")
        .arg(pct, 0, 'f', 1).arg(core::FormatUtils::formatSpeed(m.speedBytesPerSec), m.fileName));

    m_lblUrl->setText(m.url.left(80));
    m_lblStatus->setText(core::FormatUtils::stateToString(m.state));
    m_lblFileSize->setText(core::FormatUtils::formatBytes(m.totalBytes));
    m_lblDownloaded->setText(QStringLiteral("%1 (%2%)")
        .arg(core::FormatUtils::formatBytes(m.downloadedBytes)).arg(pct, 0, 'f', 1));
    m_lblTransferRate->setText(core::FormatUtils::formatSpeed(m.speedBytesPerSec));
    m_lblTimeLeft->setText(core::FormatUtils::formatEta(m.etaSeconds));
    m_lblResumeCapability->setText(m.resumeSupported ? QStringLiteral("Yes (HTTP 206 Byte-Ranges)") : QStringLiteral("No"));

    m_mainProgress->setValue(static_cast<int>(pct * 10.0));
    m_segmentBar->setSegments(m.totalBytes, m.segments);
    m_segModel->setSegments(m.segments);

    const bool active = (m.state == core::TaskState::Active || m.state == core::TaskState::Probing);
    m_btnPauseResume->setText(active ? QStringLiteral("Pause") : QStringLiteral("Resume"));
    m_btnPauseResume->setIcon(IconRegistry::icon(active ? IconId::Pause : IconId::StartPlay, 14));
    system::WindowsSystemIntegration::setTaskbarProgress(this, m.downloadedBytes, m.totalBytes, !active);
}

void DownloadProgressDialog::onToggleAlwaysOnTop(bool pinned) {
    setWindowFlag(Qt::WindowStaysOnTopHint, pinned);
    show();
}

void DownloadProgressDialog::onPauseResumeClicked() {
    if (m_task == nullptr) { return; }
    if (m_task->state() == core::TaskState::Active || m_task->state() == core::TaskState::Probing) {
        m_task->pause();
    } else {
        m_task->start();
    }
}

} // namespace idm::ui::dialogs
