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
        ar ? QStringLiteral("نوافذ التحميل المنبثقة الحية (IDM-Style Popups)")
           : QStringLiteral("Live Download Popup Windows (IDM-Style)"), this);
    auto* popLayout = new QVBoxLayout(popupsGroup);
    popLayout->setSpacing(8);

    m_chkShowStartConfirm = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة تأكيد بدء التحميل واختيار الفئة قبل التنزيل")
           : QStringLiteral("Show Start Download confirmation dialog before starting"), this);
    m_chkShowStartConfirm->setChecked(cfg.showStartConfirmDialog());
    popLayout->addWidget(m_chkShowStartConfirm);

    m_chkShowLiveProgress = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة تقدم التحميل الحية وشريط الاتصالات المجزأ (Download Progress Dialog)")
           : QStringLiteral("Show live multi-segment Download Progress window during download"), this);
    m_chkShowLiveProgress->setChecked(cfg.showLiveProgressDialog());
    popLayout->addWidget(m_chkShowLiveProgress);

    m_chkShowCompleteDialog = new QCheckBox(
        ar ? QStringLiteral("إظهار نافذة اكتمال التحميل المنبثقة عند انتهاء الملف (Download Complete Dialog)")
           : QStringLiteral("Show Download Complete popup dialog when a file finishes"), this);
    m_chkShowCompleteDialog->setChecked(cfg.showCompletionDialog());
    popLayout->addWidget(m_chkShowCompleteDialog);
    root->addWidget(popupsGroup);

    auto* avGroup = new QGroupBox(
        ar ? QStringLiteral("فحص الفيروسات التلقائي وختم حماية ويندوز (Antivirus & Mark-of-the-Web)")
           : QStringLiteral("Antivirus Scanning & Windows SmartScreen Protection"), this);
    auto* avForm = new QFormLayout(avGroup);
    avForm->setSpacing(8);

    m_chkEnableAvScan = new QCheckBox(
        ar ? QStringLiteral("فحص جميع الملفات المكتملة تلقائياً ضد الفيروسات فور انتهاء التجميع")
           : QStringLiteral("Automatically scan completed files for viruses after assembly"), this);
    m_chkEnableAvScan->setChecked(cfg.enableAntivirusScan());
    avForm->addRow(m_chkEnableAvScan);

    m_avEngineCombo = new QComboBox(this);
    m_avEngineCombo->addItem(
        ar ? QStringLiteral("فاحص Windows Defender المدمج (MpCmdRun.exe -Scan -ScanType 3)")
           : QStringLiteral("Built-in Windows Defender CLI (MpCmdRun.exe -Scan -ScanType 3)"),
        true);
    m_avEngineCombo->addItem(
        ar ? QStringLiteral("برنامج مكافحة فيروسات خارجي مخصص (Custom Antivirus CLI)")
           : QStringLiteral("Custom External Antivirus Scanner Executable"),
        false);
    m_avEngineCombo->setCurrentIndex(cfg.useWindowsDefender() ? 0 : 1);
    avForm->addRow(ar ? QStringLiteral("محرك الفحص:") : QStringLiteral("Scanner Engine:"), m_avEngineCombo);

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
    avForm->addRow(ar ? QStringLiteral("مسار الفاحص المخصص:") : QStringLiteral("Custom Scanner Path:"), customAvRow);

    m_chkWriteMotw = new QCheckBox(
        ar ? QStringLiteral("كتابة ختم المصدر (Zone.Identifier ADS) لتفعيل حماية Windows SmartScreen")
           : QStringLiteral("Write Mark-of-the-Web (Zone.Identifier ADS) for Windows SmartScreen safety"), this);
    m_chkWriteMotw->setChecked(cfg.writeMarkOfTheWeb());
    avForm->addRow(m_chkWriteMotw);
    root->addWidget(avGroup);

    auto* soundGroup = new QGroupBox(
        ar ? QStringLiteral("التنبيهات الصوتية عند اكتمال التحميل")
           : QStringLiteral("Audio Notifications on Download Events"), this);
    auto* soundForm = new QFormLayout(soundGroup);
    soundForm->setSpacing(8);

    m_chkSoundAlerts = new QCheckBox(
        ar ? QStringLiteral("تشغيل تنبيه صوتي عند اكتمال تحميل الملف")
           : QStringLiteral("Play audio notification when download completes"), this);
    m_chkSoundAlerts->setChecked(cfg.enableSoundAlerts());
    soundForm->addRow(m_chkSoundAlerts);

    auto* waveRow = new QHBoxLayout();
    m_wavePathEdit = new QLineEdit(cfg.completionWavePath(), this);
    m_wavePathEdit->setPlaceholderText(ar ? QStringLiteral("ملف WAV مخصص (اتركه فارغاً للنغمة الافتراضية)")
                                          : QStringLiteral("Custom .wav file (leave blank for system sound)"));
    auto* btnBrowseWave = new QPushButton(ar ? QStringLiteral("اختيار ملف...") : QStringLiteral("Browse WAV..."), this);
    connect(btnBrowseWave, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Select WAV Audio"),
                                                          QString(), QStringLiteral("Wave Audio (*.wav)"));
        if (!file.isEmpty()) m_wavePathEdit->setText(file);
    });
    waveRow->addWidget(m_wavePathEdit, 1);
    waveRow->addWidget(btnBrowseWave);
    soundForm->addRow(ar ? QStringLiteral("ملف الصوت (.wav):") : QStringLiteral("Custom WAV Sound:"), waveRow);
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
