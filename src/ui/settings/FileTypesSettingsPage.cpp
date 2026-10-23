#include "ui/settings/FileTypesSettingsPage.h"
#include "config/AppConfig.h"
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
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* extBox = new QGroupBox(QStringLiteral("Automatically Captured File Extensions"), this);
    auto* extLayout = new QVBoxLayout(extBox);
    m_extensionsEdit = new QPlainTextEdit(cfg.capturedExtensions().join(QLatin1Char(' ')).toUpper(), extBox);
    m_extensionsEdit->setFont(ThemeTokens::monoFont(false));
    m_extensionsEdit->setMaximumHeight(85);
    extLayout->addWidget(m_extensionsEdit);

    auto* extBtnRow = new QHBoxLayout();
    auto* btnResetExt = new QPushButton(QStringLiteral("Reset Default Extensions"), extBox);
    auto* btnOpenChromeExt = new QPushButton(QStringLiteral("Open Chrome Extension Folder"), extBox);
    btnOpenChromeExt->setIcon(IconRegistry::icon(IconId::BrowserExt, 14));
    extBtnRow->addWidget(btnResetExt);
    extBtnRow->addStretch(1);
    extBtnRow->addWidget(btnOpenChromeExt);
    extLayout->addLayout(extBtnRow);
    root->addWidget(extBox);

    auto* domBox = new QGroupBox(QStringLiteral("Excluded Websites & Domains (Do Not Intercept Automatically)"), this);
    auto* domLayout = new QVBoxLayout(domBox);
    m_excludedDomainsEdit = new QPlainTextEdit(cfg.excludedDomains().join(QStringLiteral(", ")), domBox);
    m_excludedDomainsEdit->setFont(ThemeTokens::monoFont(false));
    m_excludedDomainsEdit->setMaximumHeight(68);
    domLayout->addWidget(m_excludedDomainsEdit);
    root->addWidget(domBox);

    auto* dupBox = new QGroupBox(QStringLiteral("Duplicate File Conflict Resolution Policy"), this);
    auto* dupForm = new QFormLayout(dupBox);
    m_duplicatePolicyCombo = new QComboBox(dupBox);
    m_duplicatePolicyCombo->addItems({
        QStringLiteral("Auto-Rename with Sequential Number: file (1).zip"),
        QStringLiteral("Resume Download if File Size & ETag Match"),
        QStringLiteral("Overwrite Existing File on Disk"),
        QStringLiteral("Always Prompt User on Conflict")
    });
    m_duplicatePolicyCombo->setCurrentIndex(static_cast<int>(cfg.duplicatePolicy()));
    dupForm->addRow(QStringLiteral("When File Exists:"), m_duplicatePolicyCombo);
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
