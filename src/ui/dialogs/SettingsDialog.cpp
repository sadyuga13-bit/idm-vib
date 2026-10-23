#include "ui/dialogs/SettingsDialog.h"
#include "core/AppSettings.h"
#include "ui/theme/AppStyleSheet.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace idm::ui::dialogs {

using core::AppSettings;
using theme::AppStyleSheet;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("Application Settings, YouTube Extractor & Browser Bridge"));
    setMinimumSize(620, 440);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto* tabs = new QTabWidget(this);
    tabs->setIconSize(QSize(15, 15));
    tabs->addTab(buildGeneralTab(), IconRegistry::icon(IconId::ThemeSun, 15), QStringLiteral("Appearance & General"));
    tabs->addTab(buildEngineTab(), IconRegistry::icon(IconId::CpuThreads, 15), QStringLiteral("Download Engine"));
    tabs->addTab(buildYoutubeTab(), IconRegistry::icon(IconId::YoutubePlay, 15), QStringLiteral("YouTube & Media"));
    tabs->addTab(buildBrowserNetworkTab(), IconRegistry::icon(IconId::BrowserExt, 15), QStringLiteral("Chrome Extension & Network"));
    rootLayout->addWidget(tabs, 1);

    auto* footer = new QHBoxLayout();
    footer->addStretch(1);
    auto* btnCancel = new QPushButton(QStringLiteral("Cancel"), this);
    auto* btnSave = new QPushButton(QStringLiteral("Save & Apply Settings"), this);
    btnSave->setObjectName(QStringLiteral("primaryActionBtn"));
    btnSave->setIcon(QIcon(IconRegistry::pixmapColored(IconId::ShieldVerify, 14, Qt::white)));
    footer->addWidget(btnCancel);
    footer->addWidget(btnSave);
    rootLayout->addLayout(footer);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::onSaveSettings);
}

QWidget* SettingsDialog::buildGeneralTab() {
    const AppSettings& cfg = AppSettings::instance();
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* themeGroup = new QGroupBox(QStringLiteral("Interface Theme & Clipboard Automation"), page);
    auto* form = new QFormLayout(themeGroup);
    form->setSpacing(10);

    m_themeCombo = new QComboBox(themeGroup);
    m_themeCombo->addItems({
        QStringLiteral("Dark Engineering Mode (Obsidian #0B0C0E)"),
        QStringLiteral("Daylight High-Contrast Mode (Pure Light #FFFFFF)")
    });
    m_themeCombo->setCurrentIndex(cfg.isDayMode() ? 1 : 0);
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [](int idx) {
        AppStyleSheet::setDayMode(idx == 1);
    });
    form->addRow(QStringLiteral("Color Mode:"), m_themeCombo);

    m_clipboardCheck = new QCheckBox(
        QStringLiteral("Automatically capture HTTP/HTTPS & YouTube links copied to clipboard"), themeGroup);
    m_clipboardCheck->setChecked(cfg.clipboardMonitoringEnabled());
    form->addRow(QStringLiteral("Clipboard Monitor:"), m_clipboardCheck);

    auto* dirRow = new QHBoxLayout();
    m_defaultDirEdit = new QLineEdit(cfg.defaultSaveDirectory(), themeGroup);
    auto* btnBrowse = new QPushButton(QStringLiteral("Browse"), themeGroup);
    btnBrowse->setIcon(IconRegistry::icon(IconId::FolderOpen, 14));
    dirRow->addWidget(m_defaultDirEdit, 1);
    dirRow->addWidget(btnBrowse);
    form->addRow(QStringLiteral("Default Save Path:"), dirRow);

    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        const QString picked = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Select Default Save Directory"), m_defaultDirEdit->text());
        if (!picked.isEmpty()) {
            m_defaultDirEdit->setText(picked);
        }
    });

    layout->addWidget(themeGroup);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildEngineTab() {
    const AppSettings& cfg = AppSettings::instance();
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* engGroup = new QGroupBox(QStringLiteral("Multi-Segment & Concurrency Parameters"), page);
    auto* form = new QFormLayout(engGroup);
    form->setSpacing(10);

    m_defaultSegmentsSpin = new QSpinBox(engGroup);
    m_defaultSegmentsSpin->setRange(1, 32);
    m_defaultSegmentsSpin->setValue(cfg.defaultSegments());
    m_defaultSegmentsSpin->setSuffix(QStringLiteral(" parallel segments"));
    form->addRow(QStringLiteral("Default Connections:"), m_defaultSegmentsSpin);

    m_maxConcurrentSpin = new QSpinBox(engGroup);
    m_maxConcurrentSpin->setRange(1, 16);
    m_maxConcurrentSpin->setValue(cfg.maxConcurrentTasks());
    m_maxConcurrentSpin->setSuffix(QStringLiteral(" active tasks"));
    form->addRow(QStringLiteral("Queue Concurrency:"), m_maxConcurrentSpin);

    m_speedLimitSpin = new QSpinBox(engGroup);
    m_speedLimitSpin->setRange(0, 1000000);
    m_speedLimitSpin->setSingleStep(256);
    m_speedLimitSpin->setSpecialValueText(QStringLiteral("Unlimited"));
    m_speedLimitSpin->setSuffix(QStringLiteral(" KB/s"));
    m_speedLimitSpin->setValue(static_cast<int>(cfg.globalSpeedLimitKiB()));
    form->addRow(QStringLiteral("Global Rate Limit:"), m_speedLimitSpin);

    layout->addWidget(engGroup);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildYoutubeTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* ytGroup = new QGroupBox(QStringLiteral("YouTube Innertube & yt-dlp Media Engine"), page);
    auto* form = new QFormLayout(ytGroup);
    form->setSpacing(10);

    m_ytQualityCombo = new QComboBox(ytGroup);
    m_ytQualityCombo->addItems({
        QStringLiteral("Progressive MP4 (Combined Video + Audio)"),
        QStringLiteral("1080p Full HD MP4"),
        QStringLiteral("720p HD MP4"),
        QStringLiteral("Audio Only (M4A AAC)")
    });
    m_ytQualityCombo->setCurrentIndex(0);
    form->addRow(QStringLiteral("Preferred Stream:"), m_ytQualityCombo);

    const QString binPath = network::YoutubeExtractor::locateYtDlpBinary();
    m_lblYtDlpStatus = new QLabel(
        binPath.isEmpty()
            ? QStringLiteral("Built-in C++ Innertube ANDROID client active (yt-dlp.exe optional).")
            : QStringLiteral("Active: %1").arg(binPath),
        ytGroup);
    m_lblYtDlpStatus->setFont(ThemeTokens::monoFont(false));
    form->addRow(QStringLiteral("Engine Status:"), m_lblYtDlpStatus);

    m_btnInstallYtDlp = new QPushButton(QStringLiteral("Download / Update yt-dlp.exe into App Folder"), ytGroup);
    m_btnInstallYtDlp->setIcon(IconRegistry::icon(IconId::CategoryAll, 14));
    form->addRow(QStringLiteral("Secondary Engine:"), m_btnInstallYtDlp);

    connect(m_btnInstallYtDlp, &QPushButton::clicked, this, &SettingsDialog::onInstallYtDlp);
    connect(&m_ytExtractor, &network::YoutubeExtractor::ytDlpDownloadProgress, this,
        [this](qint64 recv, qint64 tot) {
            if (tot > 0) {
                m_lblYtDlpStatus->setText(QStringLiteral("Downloading yt-dlp.exe... %1%")
                    .arg((recv * 100) / tot));
            }
        });
    connect(&m_ytExtractor, &network::YoutubeExtractor::ytDlpDownloadFinished, this,
        [this](bool ok, const QString& msg) {
            m_btnInstallYtDlp->setEnabled(true);
            m_lblYtDlpStatus->setText(msg);
            m_lblYtDlpStatus->setStyleSheet(ok ? QStringLiteral("color: #10B981;") : QStringLiteral("color: #EF4444;"));
        });

    layout->addWidget(ytGroup);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildBrowserNetworkTab() {
    const AppSettings& cfg = AppSettings::instance();
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* extGroup = new QGroupBox(QStringLiteral("Google Chrome / Edge Extension Integration"), page);
    auto* extForm = new QFormLayout(extGroup);
    extForm->setSpacing(8);

    m_ipcPortSpin = new QSpinBox(extGroup);
    m_ipcPortSpin->setRange(1024, 65535);
    m_ipcPortSpin->setValue(cfg.ipcPort());
    extForm->addRow(QStringLiteral("Local IPC Port:"), m_ipcPortSpin);

    auto* btnOpenExt = new QPushButton(QStringLiteral("Open Chrome Extension Folder (Load Unpacked)"), extGroup);
    btnOpenExt->setObjectName(QStringLiteral("primaryActionBtn"));
    btnOpenExt->setIcon(QIcon(IconRegistry::pixmapColored(IconId::BrowserExt, 14, Qt::white)));
    connect(btnOpenExt, &QPushButton::clicked, this, &SettingsDialog::onOpenChromeExtFolder);
    extForm->addRow(QStringLiteral("Browser Extension:"), btnOpenExt);

    auto* lblHint = new QLabel(
        QStringLiteral("In Chrome: open chrome://extensions -> Enable Developer Mode -> Load unpacked -> select the 'chrome-extension' folder."),
        extGroup);
    lblHint->setWordWrap(true);
    extForm->addRow(QStringLiteral("Install Guide:"), lblHint);
    layout->addWidget(extGroup);

    auto* netGroup = new QGroupBox(QStringLiteral("Global User-Agent & HTTP Proxy"), page);
    auto* netForm = new QFormLayout(netGroup);
    netForm->setSpacing(8);

    m_userAgentEdit = new QLineEdit(cfg.customUserAgent(), netGroup);
    netForm->addRow(QStringLiteral("Default User-Agent:"), m_userAgentEdit);

    auto* proxyRow = new QHBoxLayout();
    m_proxyCheck = new QCheckBox(QStringLiteral("Enable HTTP Proxy"), netGroup);
    m_proxyCheck->setChecked(cfg.proxyEnabled());
    m_proxyHostEdit = new QLineEdit(cfg.proxyHost(), netGroup);
    m_proxyPortSpin = new QSpinBox(netGroup);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(cfg.proxyPort());
    proxyRow->addWidget(m_proxyCheck);
    proxyRow->addWidget(m_proxyHostEdit, 1);
    proxyRow->addWidget(m_proxyPortSpin);
    netForm->addRow(QStringLiteral("Proxy Server:"), proxyRow);

    layout->addWidget(netGroup);
    layout->addStretch(1);
    return page;
}

void SettingsDialog::onSaveSettings() {
    AppSettings& cfg = AppSettings::instance();
    AppStyleSheet::setDayMode(m_themeCombo->currentIndex() == 1);
    cfg.setClipboardMonitoringEnabled(m_clipboardCheck->isChecked());
    cfg.setDefaultSaveDirectory(m_defaultDirEdit->text());
    cfg.setDefaultSegments(m_defaultSegmentsSpin->value());
    cfg.setMaxConcurrentTasks(m_maxConcurrentSpin->value());
    cfg.setGlobalSpeedLimitKiB(m_speedLimitSpin->value());
    cfg.setIpcPort(static_cast<quint16>(m_ipcPortSpin->value()));
    cfg.setCustomUserAgent(m_userAgentEdit->text());
    cfg.setProxyEnabled(m_proxyCheck->isChecked());
    cfg.setProxyHost(m_proxyHostEdit->text());
    cfg.setProxyPort(static_cast<quint16>(m_proxyPortSpin->value()));
    cfg.sync();
    emit settingsApplied();
    accept();
}

void SettingsDialog::onInstallYtDlp() {
    m_btnInstallYtDlp->setEnabled(false);
    m_lblYtDlpStatus->setText(QStringLiteral("Connecting to GitHub releases..."));
    const QString target = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("yt-dlp.exe"));
    m_ytExtractor.downloadYtDlpBinary(target);
}

void SettingsDialog::onOpenChromeExtFolder() {
    QString extDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("chrome-extension"));
    if (!QDir(extDir).exists()) {
        extDir = QCoreApplication::applicationDirPath();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(extDir));
}

} // namespace idm::ui::dialogs
