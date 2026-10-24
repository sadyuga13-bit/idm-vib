#include "ui/settings/GeneralSettingsPage.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "system/WindowsSystemIntegration.h"
#include "ui/theme/AppStyleSheet.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace idm::ui::settings {

using config::AppConfig;

GeneralSettingsPage::GeneralSettingsPage(QWidget* parent)
    : QWidget(parent) {
    const AppConfig& cfg = AppConfig::instance();
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto* langBox = new QGroupBox(ar ? QStringLiteral("اللغة والمظهر")
                                     : QStringLiteral("Language & Appearance"), this);
    auto* langForm = new QFormLayout(langBox);
    langForm->setSpacing(8);

    m_langCombo = new QComboBox(langBox);
    const QList<QPair<QString, QString>> langs = {
        {QStringLiteral("auto"), ar ? QStringLiteral("تلقائي (لغة النظام)") : QStringLiteral("System Default")},
        {QStringLiteral("ar"), QStringLiteral("العربية")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("es"), QStringLiteral("Español")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QStringLiteral("ja"), QStringLiteral("日本語")},
        {QStringLiteral("tr"), QStringLiteral("Türkçe")},
        {QStringLiteral("pt_BR"), QStringLiteral("Português (Brasil)")}
    };
    const QString curLang = cfg.languageCode();
    for (int i = 0; i < langs.size(); ++i) {
        m_langCombo->addItem(langs.at(i).second, langs.at(i).first);
        if (langs.at(i).first == curLang) {
            m_langCombo->setCurrentIndex(i);
        }
    }
    langForm->addRow(ar ? QStringLiteral("اللغة:") : QStringLiteral("Language:"), m_langCombo);

    m_themeCombo = new QComboBox(langBox);
    m_themeCombo->addItems({
        ar ? QStringLiteral("الوضع الداكن") : QStringLiteral("Dark"),
        ar ? QStringLiteral("الوضع الفاتح") : QStringLiteral("Light")
    });
    m_themeCombo->setCurrentIndex(cfg.isDayMode() ? 1 : 0);
    langForm->addRow(ar ? QStringLiteral("المظهر:") : QStringLiteral("Theme:"), m_themeCombo);
    root->addWidget(langBox);

    auto* sysBox = new QGroupBox(ar ? QStringLiteral("بدء التشغيل والنظام")
                                    : QStringLiteral("Startup & System"), this);
    auto* sysLayout = new QVBoxLayout(sysBox);
    m_chkStartWindows = new QCheckBox(
        ar ? QStringLiteral("تشغيل البرنامج عند بدء تشغيل ويندوز")
           : QStringLiteral("Launch IDM Vibe at Windows startup"), sysBox);
    m_chkStartWindows->setChecked(cfg.startWithWindows());
    m_chkCloseToTray = new QCheckBox(
        ar ? QStringLiteral("تصغير البرنامج إلى شريط المهام عند الإغلاق")
           : QStringLiteral("Minimize to system tray on close"), sysBox);
    m_chkCloseToTray->setChecked(cfg.closeToTray());
    m_chkPreventSleep = new QCheckBox(
        ar ? QStringLiteral("منع الجهاز من الدخول في وضع السكون أثناء التحميل")
           : QStringLiteral("Prevent system sleep during active downloads"), sysBox);
    m_chkPreventSleep->setChecked(cfg.preventSystemSleep());
    sysLayout->addWidget(m_chkStartWindows);
    sysLayout->addWidget(m_chkCloseToTray);
    sysLayout->addWidget(m_chkPreventSleep);
    root->addWidget(sysBox);

    auto* clipBox = new QGroupBox(ar ? QStringLiteral("الحافظة ومفاتيح الاختصار")
                                     : QStringLiteral("Clipboard & Keys"), this);
    auto* clipLayout = new QVBoxLayout(clipBox);
    m_chkClipboard = new QCheckBox(
        ar ? QStringLiteral("مراقبة الروابط المنسوخة في الحافظة تلقائياً")
           : QStringLiteral("Monitor clipboard for download links"), clipBox);
    m_chkClipboard->setChecked(cfg.monitorClipboard());
    m_chkAltSuppress = new QCheckBox(
        ar ? QStringLiteral("الضغط المطول على Alt لمنع التقاط الرابط")
           : QStringLiteral("Hold Alt key to skip automatic capture"), clipBox);
    m_chkAltSuppress->setChecked(cfg.suppressWithAltKey());
    m_chkInsertForce = new QCheckBox(
        ar ? QStringLiteral("الضغط المطول على Insert لإجبار التقاط الرابط")
           : QStringLiteral("Hold Insert key to force download capture"), clipBox);
    m_chkInsertForce->setChecked(cfg.forceWithInsertKey());
    clipLayout->addWidget(m_chkClipboard);
    clipLayout->addWidget(m_chkAltSuppress);
    clipLayout->addWidget(m_chkInsertForce);
    root->addWidget(clipBox);
    root->addStretch(1);
}

void GeneralSettingsPage::applyToConfig() {
    AppConfig& cfg = AppConfig::instance();
    const QString langCode = m_langCombo->currentData().toString();
    cfg.setLanguageCode(langCode);
    i18n::I18nManager::instance().applyLanguage(langCode);

    const bool dayMode = (m_themeCombo->currentIndex() == 1);
    cfg.setDayMode(dayMode);
    theme::AppStyleSheet::setDayMode(dayMode);

    cfg.setStartWithWindows(m_chkStartWindows->isChecked());
    system::WindowsSystemIntegration::setAutoStartWithWindows(m_chkStartWindows->isChecked());
    cfg.setCloseToTray(m_chkCloseToTray->isChecked());
    cfg.setPreventSystemSleep(m_chkPreventSleep->isChecked());
    cfg.setMonitorClipboard(m_chkClipboard->isChecked());
    cfg.setSuppressWithAltKey(m_chkAltSuppress->isChecked());
    cfg.setForceWithInsertKey(m_chkInsertForce->isChecked());
}

} // namespace idm::ui::settings
