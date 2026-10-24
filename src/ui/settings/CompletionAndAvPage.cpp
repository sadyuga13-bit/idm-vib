#include "ui/settings/CompletionAndAvPage.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace idm::ui::settings {

CompletionAndAvPage::CompletionAndAvPage(QWidget* parent)
    : QWidget(parent) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto& cfg = config::AppConfig::instance();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(12);

    auto* popupsGroup = new QGroupBox(
        ar ? QStringLiteral("مربعات حوار التنزيل")
           : QStringLiteral("Download Dialogs"), this);
    auto* popLayout = new QVBoxLayout(popupsGroup);
    popLayout->setSpacing(8);

    m_chkShowStartConfirm = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة بدء التحميل")
           : QStringLiteral("Show start download dialog"), this);
    m_chkShowStartConfirm->setChecked(cfg.showStartConfirmDialog());
    popLayout->addWidget(m_chkShowStartConfirm);

    m_chkShowLiveProgress = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة تقدم التحميل أثناء التنزيل")
           : QStringLiteral("Show download progress dialog"), this);
    m_chkShowLiveProgress->setChecked(cfg.showLiveProgressDialog());
    popLayout->addWidget(m_chkShowLiveProgress);

    m_chkShowCompleteDialog = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة اكتمال التحميل")
           : QStringLiteral("Show download complete dialog"), this);
    m_chkShowCompleteDialog->setChecked(cfg.showCompletionDialog());
    popLayout->addWidget(m_chkShowCompleteDialog);
    root->addWidget(popupsGroup);

    auto* avGroup = new QGroupBox(
        ar ? QStringLiteral("فحص الفيروسات")
           : QStringLiteral("Virus Checking"), this);
    auto* avForm = new QFormLayout(avGroup);
    avForm->setSpacing(8);

    m_chkEnableAvScan = new QCheckBox(
        ar ? QStringLiteral("فحص الملفات تلقائياً بعد اكتمال التحميل")
           : QStringLiteral("Automatically scan downloaded files"), this);
    m_chkEnableAvScan->setChecked(cfg.enableAntivirusScan());
    avForm->addRow(m_chkEnableAvScan);

    m_avEngineCombo = new QComboBox(this);
    m_avEngineCombo->addItem(
        ar ? QStringLiteral("Windows Defender")
           : QStringLiteral("Windows Defender"),
        true);
    m_avEngineCombo->addItem(
        ar ? QStringLiteral("برنامج حماية مخصص")
           : QStringLiteral("Custom antivirus program"),
        false);
    m_avEngineCombo->setCurrentIndex(cfg.useWindowsDefender() ? 0 : 1);
    avForm->addRow(ar ? QStringLiteral("البرنامج:") : QStringLiteral("Program:"), m_avEngineCombo);

    auto* customAvRow = new QHBoxLayout();
    m_customAvPathEdit = new QLineEdit(cfg.customAntivirusPath(), this);
    m_customAvPathEdit->setPlaceholderText(QStringLiteral("C:\\Program Files\\...\\avscan.exe"));
    auto* btnBrowseAv = new QPushButton(ar ? QStringLiteral("استعراض...") : QStringLiteral("Browse..."), this);
    connect(btnBrowseAv, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select Antivirus Executable"),
                                                          QString(), QStringLiteral("Executables (*.exe)"));
        if (!file.isEmpty()) m_customAvPathEdit->setText(file);
    });
    customAvRow->addWidget(m_customAvPathEdit, 1);
    customAvRow->addWidget(btnBrowseAv);
    avForm->addRow(ar ? QStringLiteral("المسار:") : QStringLiteral("Path:"), customAvRow);

    m_chkWriteMotw = new QCheckBox(
        ar ? QStringLiteral("التحقق من أمان الملفات عبر Windows SmartScreen")
           : QStringLiteral("Enable Windows SmartScreen verification"), this);
    m_chkWriteMotw->setChecked(cfg.writeMarkOfTheWeb());
    avForm->addRow(m_chkWriteMotw);
    root->addWidget(avGroup);

    auto* soundGroup = new QGroupBox(
        ar ? QStringLiteral("الأصوات")
           : QStringLiteral("Sounds"), this);
    auto* soundForm = new QFormLayout(soundGroup);
    soundForm->setSpacing(8);

    m_chkSoundAlerts = new QCheckBox(
        ar ? QStringLiteral("تشغيل صوت عند اكتمال التحميل")
           : QStringLiteral("Play sound when download completes"), this);
    m_chkSoundAlerts->setChecked(cfg.enableSoundAlerts());
    soundForm->addRow(m_chkSoundAlerts);

    auto* waveRow = new QHBoxLayout();
    m_wavePathEdit = new QLineEdit(cfg.completionWavePath(), this);
    m_wavePathEdit->setPlaceholderText(ar ? QStringLiteral("افتراضي")
                                          : QStringLiteral("Default"));
    auto* btnBrowseWave = new QPushButton(ar ? QStringLiteral("استعراض...") : QStringLiteral("Browse..."), this);
    connect(btnBrowseWave, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select WAV Audio"),
                                                          QString(), QStringLiteral("Wave Audio (*.wav)"));
        if (!file.isEmpty()) m_wavePathEdit->setText(file);
    });
    waveRow->addWidget(m_wavePathEdit, 1);
    waveRow->addWidget(btnBrowseWave);
    soundForm->addRow(ar ? QStringLiteral("ملف الصوت:") : QStringLiteral("Sound file:"), waveRow);
    root->addWidget(soundGroup);

    root->addStretch(1);
}

void CompletionAndAvPage::applyToConfig() {
    auto& cfg = config::AppConfig::instance();
    cfg.setShowStartConfirmDialog(m_chkShowStartConfirm->isChecked());
    cfg.setShowLiveProgressDialog(m_chkShowLiveProgress->isChecked());
    cfg.setShowCompletionDialog(m_chkShowCompleteDialog->isChecked());

    cfg.setEnableAntivirusScan(m_chkEnableAvScan->isChecked());
    cfg.setUseWindowsDefender(m_avEngineCombo->currentData().toBool());
    cfg.setCustomAntivirusPath(m_customAvPathEdit->text().trimmed());
    cfg.setWriteMarkOfTheWeb(m_chkWriteMotw->isChecked());

    cfg.setEnableSoundAlerts(m_chkSoundAlerts->isChecked());
    cfg.setCompletionWavePath(m_wavePathEdit->text().trimmed());
}

} // namespace idm::ui::settings
