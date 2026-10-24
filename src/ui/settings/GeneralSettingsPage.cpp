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

    auto* langBox = new QGroupBox(ar ? QStringLiteral("اللغة والاتجاه والمظهر البصري")
                                     : QStringLiteral("Language (i18n + RTL/LTR) & Visual Theme"), this);
    auto* langForm = new QFormLayout(langBox);
    langForm->setSpacing(8);

    m_langCombo = new QComboBox(langBox);
    const QList<QPair<QString, QString>> langs = {
        {QStringLiteral("auto"), ar ? QStringLiteral("اكتشاف تلقائي للغة النظام") : QStringLiteral("Auto-Detect System Language")},
        {QStringLiteral("ar"), QStringLiteral("العربية (Arabic - RTL)")},
        {QStringLiteral("en"), QStringLiteral("English (LTR)")},
        {QStringLiteral("es"), QStringLiteral("Español (Spanish)")},
        {QStringLiteral("fr"), QStringLiteral("Français (French)")},
        {QStringLiteral("de"), QStringLiteral("Deutsch (German)")},
        {QStringLiteral("ru"), QStringLiteral("Русский (Russian)")},
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文 (Chinese Simplified)")},
        {QStringLiteral("ja"), QStringLiteral("日本語 (Japanese)")},
        {QStringLiteral("tr"), QStringLiteral("Türkçe (Turkish)")},
        {QStringLiteral("pt_BR"), QStringLiteral("Português (Brasil)")}
    };
    const QString curLang = cfg.languageCode();
    for (int i = 0; i < langs.size(); ++i) {
        m_langCombo->addItem(langs.at(i).second, langs.at(i).first);
        if (langs.at(i).first == curLang) {
            m_langCombo->setCurrentIndex(i);
        }
    }
    langForm->addRow(ar ? QStringLiteral("لغة الواجهة:") : QStringLiteral("Interface Language:"), m_langCombo);

    m_themeCombo = new QComboBox(langBox);
    m_themeCombo->addItems({
        ar ? QStringLiteral("الوضع الليلي الهندسي الداكن (#0B0C0E)") : QStringLiteral("Dark Engineering Theme (#0B0C0E)"),
        ar ? QStringLiteral("الوضع النهاري عالي التباين (#FFFFFF)") : QStringLiteral("Daylight High-Contrast Theme (#FFFFFF)")
    });
    m_themeCombo->setCurrentIndex(cfg.isDayMode() ? 1 : 0);
    langForm->addRow(ar ? QStringLiteral("نمط الألوان:") : QStringLiteral("Color Mode:"), m_themeCombo);
    root->addWidget(langBox);

    auto* sysBox = new QGroupBox(ar ? QStringLiteral("التشغيل مع ويندوز وشريط المهام والطاقة")
                                    : QStringLiteral("System Startup, Tray & Power Behavior"), this);
    auto* sysLayout = new QVBoxLayout(sysBox);
    m_chkStartWindows = new QCheckBox(
        ar ? QStringLiteral("التشغيل التلقائي مع بدء ويندوز (مُصغّر بجانب الساعة --tray)")
           : QStringLiteral("Launch automatically at Windows startup (minimized to system tray --tray)"), sysBox);
    m_chkStartWindows->setChecked(cfg.startWithWindows());
    m_chkCloseToTray = new QCheckBox(
        ar ? QStringLiteral("اختزال البرنامج بجانب الساعة عند الضغط على زر الإغلاق [X]")
           : QStringLiteral("Minimize to system tray near clock when clicking Close [X] button"), sysBox);
    m_chkCloseToTray->setChecked(cfg.closeToTray());
    m_chkPreventSleep = new QCheckBox(
        ar ? QStringLiteral("منع النظام من الدخول في وضع السكون أثناء وجود تحميلات نشطة")
           : QStringLiteral("Prevent Windows from entering sleep mode while downloads are active"), sysBox);
    m_chkPreventSleep->setChecked(cfg.preventSystemSleep());
    sysLayout->addWidget(m_chkStartWindows);
    sysLayout->addWidget(m_chkCloseToTray);
    sysLayout->addWidget(m_chkPreventSleep);
    root->addWidget(sysBox);

    auto* clipBox = new QGroupBox(ar ? QStringLiteral("مراقبة الحافظة ومفاتيح التحكم بالالتقاط")
                                     : QStringLiteral("Clipboard Monitoring & Capture Modifier Keys"), this);
    auto* clipLayout = new QVBoxLayout(clipBox);
    m_chkClipboard = new QCheckBox(
        ar ? QStringLiteral("مراقبة الحافظة تلقائياً لروابط التحميل وفيديوهات يوتيوب")
           : QStringLiteral("Monitor clipboard for HTTP/HTTPS & YouTube video URLs"), clipBox);
    m_chkClipboard->setChecked(cfg.monitorClipboard());
    m_chkAltSuppress = new QCheckBox(
        ar ? QStringLiteral("تعطيل الالتقاط التلقائي مؤقتاً عند الاستمرار بالضغط على مفتاح [Alt]")
           : QStringLiteral("Hold [Alt] key while clicking/copying to suppress automatic capture"), clipBox);
    m_chkAltSuppress->setChecked(cfg.suppressWithAltKey());
    m_chkInsertForce = new QCheckBox(
        ar ? QStringLiteral("إجبار البرنامج على التقاط أي رابط عند الضغط على مفتاح [Insert]")
           : QStringLiteral("Hold [Insert] key to force capture of any link regardless of extension"), clipBox);
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
