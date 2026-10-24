#include "ui/settings/FileTypesSettingsPage.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "storage/CategoryRouter.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

namespace idm::ui::settings {

using config::AppConfig;
using config::DuplicatePolicy;
using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

FileTypesSettingsPage::FileTypesSettingsPage(QWidget* parent)
    : QWidget(parent) {
    const AppConfig& cfg = AppConfig::instance();
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* extBox = new QGroupBox(ar ? QStringLiteral("أنواع الملفات التي يتم تحميلها تلقائياً")
                                    : QStringLiteral("Automatically start downloading these file types"), this);
    auto* extLayout = new QVBoxLayout(extBox);
    m_extensionsEdit = new QPlainTextEdit(cfg.capturedExtensions().join(QLatin1Char(' ')).toUpper(), extBox);
    m_extensionsEdit->setFont(ThemeTokens::monoFont(false));
    m_extensionsEdit->setMaximumHeight(85);
    extLayout->addWidget(m_extensionsEdit);

    auto* extBtnRow = new QHBoxLayout();
    auto* btnResetExt = new QPushButton(ar ? QStringLiteral("الافتراضي")
                                           : QStringLiteral("Default"), extBox);
    auto* btnOpenChromeExt = new QPushButton(ar ? QStringLiteral("إضافة المتصفح")
                                                : QStringLiteral("Browser Extension"), extBox);
    btnOpenChromeExt->setIcon(IconRegistry::icon(IconId::BrowserExt, 14));
    extBtnRow->addWidget(btnResetExt);
    extBtnRow->addStretch(1);
    extBtnRow->addWidget(btnOpenChromeExt);
    extLayout->addLayout(extBtnRow);
    root->addWidget(extBox);

    auto* domBox = new QGroupBox(ar ? QStringLiteral("عدم بدء التحميل تلقائياً من هذه المواقع")
                                    : QStringLiteral("Do not capture downloads from these websites"), this);
    auto* domLayout = new QVBoxLayout(domBox);
    m_excludedDomainsEdit = new QPlainTextEdit(cfg.excludedDomains().join(QStringLiteral(", ")), domBox);
    m_excludedDomainsEdit->setFont(ThemeTokens::monoFont(false));
    m_excludedDomainsEdit->setMaximumHeight(68);
    domLayout->addWidget(m_excludedDomainsEdit);
    root->addWidget(domBox);

    auto* dupBox = new QGroupBox(ar ? QStringLiteral("الملفات المكررة")
                                    : QStringLiteral("Duplicate Files"), this);
    auto* dupForm = new QFormLayout(dupBox);
    m_duplicatePolicyCombo = new QComboBox(dupBox);
    m_duplicatePolicyCombo->addItems({
        ar ? QStringLiteral("إعادة تسمية تلقائية برقم متسلسل")
           : QStringLiteral("Auto-rename with number"),
        ar ? QStringLiteral("استئناف التحميل إذا تطابق الحجم")
           : QStringLiteral("Resume if size matches"),
        ar ? QStringLiteral("استبدال الملف الموجود")
           : QStringLiteral("Overwrite existing file"),
        ar ? QStringLiteral("السؤال دائماً")
           : QStringLiteral("Always ask")
    });
    m_duplicatePolicyCombo->setCurrentIndex(static_cast<int>(cfg.duplicatePolicy()));
    dupForm->addRow(ar ? QStringLiteral("عند وجود الملف مسبقاً:") : QStringLiteral("If file exists:"), m_duplicatePolicyCombo);
    root->addWidget(dupBox);
    root->addStretch(1);

    connect(btnResetExt, &QPushButton::clicked, this, [this]() {
        m_extensionsEdit->setPlainText(
            storage::CategoryRouter::defaultCapturedExtensions().join(QLatin1Char(' ')).toUpper());
    });
    connect(btnOpenChromeExt, &QPushButton::clicked, this, []() {
        QString dir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("chrome-extension"));
        if (!QDir(dir).exists()) { dir = QCoreApplication::applicationDirPath(); }
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
}

void FileTypesSettingsPage::applyToConfig() {
    AppConfig& cfg = AppConfig::instance();
    static const QRegularExpression kSplitRx(QStringLiteral(R"([\s,;]+)"));
    const QStringList rawExts = m_extensionsEdit->toPlainText().toLower().split(kSplitRx, Qt::SkipEmptyParts);
    cfg.setCapturedExtensions(rawExts);

    const QStringList rawDoms = m_excludedDomainsEdit->toPlainText().toLower().split(kSplitRx, Qt::SkipEmptyParts);
    cfg.setExcludedDomains(rawDoms);
    cfg.setDuplicatePolicy(static_cast<DuplicatePolicy>(m_duplicatePolicyCombo->currentIndex()));
}

} // namespace idm::ui::settings
