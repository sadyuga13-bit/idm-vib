#include "ui/settings/SettingsDialog.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "ui/settings/CompletionAndAvPage.h"
#include "ui/settings/ConnectionSettingsPage.h"
#include "ui/settings/FileTypesSettingsPage.h"
#include "ui/settings/GeneralSettingsPage.h"
#include "ui/settings/ProxyAndLoginsPage.h"
#include "ui/settings/SavePathsSettingsPage.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

namespace idm::ui::settings {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    auto& trMgr = i18n::I18nManager::instance();
    setWindowTitle(trMgr.trKey(QStringLiteral("settings.title")));

    const QRect avail = QGuiApplication::primaryScreen()
        ? QGuiApplication::primaryScreen()->availableGeometry()
        : QRect(0, 0, 1280, 760);
    setMinimumSize(std::min(680, avail.width() - 40), std::min(460, avail.height() - 40));
    resize(std::min(820, int(avail.width() * 0.78)), std::min(580, int(avail.height() * 0.80)));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(12);

    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(205);
    m_navList->setIconSize(QSize(18, 18));
    m_navList->setSpacing(3);

    const auto addNavItem = [this, &trMgr](const QString& key, theme::IconId icon) {
        auto* item = new QListWidgetItem(
            theme::IconRegistry::icon(icon, 18),
            trMgr.trKey(key),
            m_navList);
        item->setSizeHint(QSize(190, 36));
    };

    addNavItem(QStringLiteral("settings.page.general"), theme::IconId::SettingsGear);
    addNavItem(QStringLiteral("settings.page.filetypes"), theme::IconId::CategoryApp);
    addNavItem(QStringLiteral("settings.page.paths"), theme::IconId::FolderOpen);
    addNavItem(QStringLiteral("settings.page.connection"), theme::IconId::CpuThreads);
    addNavItem(QStringLiteral("settings.page.proxy"), theme::IconId::LockClosed);
    addNavItem(QStringLiteral("settings.page.completion"), theme::IconId::ShieldVerify);

    m_stack = new QStackedWidget(this);
    m_generalPage = new GeneralSettingsPage(this);
    m_fileTypesPage = new FileTypesSettingsPage(this);
    m_savePathsPage = new SavePathsSettingsPage(this);
    m_connectionPage = new ConnectionSettingsPage(this);
    m_proxyPage = new ProxyAndLoginsPage(this);
    m_completionPage = new CompletionAndAvPage(this);

    auto wrapScroll = [this](QWidget* page) {
        page->setMinimumWidth(0);
        auto* scroll = new QScrollArea(m_stack);
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return scroll;
    };

    m_stack->addWidget(wrapScroll(m_generalPage));
    m_stack->addWidget(wrapScroll(m_fileTypesPage));
    m_stack->addWidget(wrapScroll(m_savePathsPage));
    m_stack->addWidget(wrapScroll(m_connectionPage));
    m_stack->addWidget(wrapScroll(m_proxyPage));
    m_stack->addWidget(wrapScroll(m_completionPage));

    connect(m_navList, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    m_navList->setCurrentRow(0);

    contentLayout->addWidget(m_navList);
    contentLayout->addWidget(m_stack, 1);
    root->addLayout(contentLayout, 1);

    auto* footer = new QHBoxLayout();
    footer->addStretch(1);

    auto* btnCancel = new QPushButton(trMgr.trKey(QStringLiteral("settings.btn.cancel")), this);
    btnCancel->setMinimumHeight(32);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(btnCancel);

    auto* btnSave = new QPushButton(trMgr.trKey(QStringLiteral("settings.btn.save")), this);
    btnSave->setObjectName(QStringLiteral("PrimaryButton"));
    btnSave->setMinimumHeight(32);
    btnSave->setDefault(true);
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::onSaveAndApply);
    footer->addWidget(btnSave);

    root->addLayout(footer);
}

void SettingsDialog::onSaveAndApply() {
    m_generalPage->applyToConfig();
    m_fileTypesPage->applyToConfig();
    m_savePathsPage->applyToConfig();
    m_connectionPage->applyToConfig();
    m_proxyPage->applyToConfig();
    m_completionPage->applyToConfig();

    config::AppConfig::instance().sync();
    emit settingsApplied();
    accept();
}

} // namespace idm::ui::settings
