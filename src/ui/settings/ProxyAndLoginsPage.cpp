#include "ui/settings/ProxyAndLoginsPage.h"
#include "config/AppConfig.h"
#include "config/SiteLoginVault.h"
#include "i18n/I18nManager.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace idm::ui::settings {

ProxyAndLoginsPage::ProxyAndLoginsPage(QWidget* parent)
    : QWidget(parent) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    auto& cfg = config::AppConfig::instance();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);

    auto* proxyGroup = new QGroupBox(
        ar ? QStringLiteral("الوكيل (Proxy / Socks)")
           : QStringLiteral("Proxy / Socks"), this);
    auto* form = new QFormLayout(proxyGroup);
    form->setSpacing(8);

    m_proxyModeCombo = new QComboBox(this);
    m_proxyModeCombo->addItem(ar ? QStringLiteral("بدون وكيل (اتصال مباشر)")
                                 : QStringLiteral("No proxy (direct connection)"),
                              static_cast<int>(config::ProxyMode::DirectConnection));
    m_proxyModeCombo->addItem(ar ? QStringLiteral("استخدام إعدادات النظام")
                                 : QStringLiteral("Use system settings"),
                              static_cast<int>(config::ProxyMode::SystemWinHttp));
    m_proxyModeCombo->addItem(ar ? QStringLiteral("إعداد يدوي للوكيل")
                                 : QStringLiteral("Manual proxy configuration"),
                              static_cast<int>(config::ProxyMode::ManualProxy));
    m_proxyModeCombo->setCurrentIndex(static_cast<int>(cfg.proxyMode()));
    connect(m_proxyModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateProxyFieldsState(); });
    form->addRow(ar ? QStringLiteral("الإعداد:") : QStringLiteral("Mode:"), m_proxyModeCombo);

    auto* hostRow = new QHBoxLayout();
    m_proxyProtoCombo = new QComboBox(this);
    m_proxyProtoCombo->addItem(QStringLiteral("HTTP"), static_cast<int>(config::ProxyProtocol::Http));
    m_proxyProtoCombo->addItem(QStringLiteral("HTTPS"), static_cast<int>(config::ProxyProtocol::Https));
    m_proxyProtoCombo->addItem(QStringLiteral("SOCKS5"), static_cast<int>(config::ProxyProtocol::Socks5));
    m_proxyProtoCombo->setCurrentIndex(static_cast<int>(cfg.proxyProtocol()));

    m_proxyHostEdit = new QLineEdit(cfg.proxyHost(), this);
    m_proxyHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
    m_proxyPortSpin = new QSpinBox(this);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(cfg.proxyPort());

    hostRow->addWidget(m_proxyProtoCombo);
    hostRow->addWidget(m_proxyHostEdit, 1);
    hostRow->addWidget(new QLabel(ar ? QStringLiteral("المنفذ:") : QStringLiteral("Port:"), this));
    hostRow->addWidget(m_proxyPortSpin);
    form->addRow(ar ? QStringLiteral("الخادم:") : QStringLiteral("Server:"), hostRow);

    auto* authRow = new QHBoxLayout();
    m_proxyUserEdit = new QLineEdit(cfg.proxyUsername(), this);
    m_proxyUserEdit->setPlaceholderText(ar ? QStringLiteral("اسم المستخدم") : QStringLiteral("Username"));
    m_proxyPassEdit = new QLineEdit(cfg.proxyPassword(), this);
    m_proxyPassEdit->setEchoMode(QLineEdit::Password);
    m_proxyPassEdit->setPlaceholderText(ar ? QStringLiteral("كلمة المرور") : QStringLiteral("Password"));
    authRow->addWidget(m_proxyUserEdit, 1);
    authRow->addWidget(m_proxyPassEdit, 1);
    form->addRow(ar ? QStringLiteral("المصادقة:") : QStringLiteral("Authentication:"), authRow);

    m_proxyBypassEdit = new QLineEdit(cfg.proxyBypassList().join(QStringLiteral(", ")), this);
    form->addRow(ar ? QStringLiteral("عدم استخدام الوكيل مع:") : QStringLiteral("Exceptions:"), m_proxyBypassEdit);
    root->addWidget(proxyGroup);

    auto* vaultGroup = new QGroupBox(
        ar ? QStringLiteral("تسجيل الدخول للمواقع")
           : QStringLiteral("Site Logins"), this);
    auto* vaultLayout = new QVBoxLayout(vaultGroup);

    auto* addSiteRow = new QHBoxLayout();
    m_siteDomainEdit = new QLineEdit(this);
    m_siteDomainEdit->setPlaceholderText(ar ? QStringLiteral("الموقع (مثال: example.com)")
                                            : QStringLiteral("Server / Domain (e.g. example.com)"));
    m_siteUserEdit = new QLineEdit(this);
    m_siteUserEdit->setPlaceholderText(ar ? QStringLiteral("اسم المستخدم") : QStringLiteral("Username"));
    m_sitePassEdit = new QLineEdit(this);
    m_sitePassEdit->setEchoMode(QLineEdit::Password);
    m_sitePassEdit->setPlaceholderText(ar ? QStringLiteral("كلمة المرور") : QStringLiteral("Password"));

    auto* btnAddSite = new QPushButton(ar ? QStringLiteral("إضافة") : QStringLiteral("Add"), this);
    auto* btnRemSite = new QPushButton(ar ? QStringLiteral("حذف") : QStringLiteral("Delete"), this);
    connect(btnAddSite, &QPushButton::clicked, this, &ProxyAndLoginsPage::onAddSiteLoginClicked);
    connect(btnRemSite, &QPushButton::clicked, this, &ProxyAndLoginsPage::onRemoveSiteLoginClicked);

    addSiteRow->addWidget(m_siteDomainEdit, 1);
    addSiteRow->addWidget(m_siteUserEdit, 1);
    addSiteRow->addWidget(m_sitePassEdit, 1);
    addSiteRow->addWidget(btnAddSite);
    addSiteRow->addWidget(btnRemSite);
    vaultLayout->addLayout(addSiteRow);

    m_vaultTable = new QTableWidget(0, 3, this);
    m_vaultTable->setHorizontalHeaderLabels({
        ar ? QStringLiteral("الموقع") : QStringLiteral("Server"),
        ar ? QStringLiteral("اسم المستخدم") : QStringLiteral("User"),
        ar ? QStringLiteral("الحالة") : QStringLiteral("Status")
    });
    m_vaultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_vaultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_vaultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_vaultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_vaultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_vaultTable->setMaximumHeight(130);
    vaultLayout->addWidget(m_vaultTable);

    root->addWidget(vaultGroup);
    root->addStretch(1);

    updateProxyFieldsState();
    refreshVaultTable();
}

void ProxyAndLoginsPage::updateProxyFieldsState() {
    const bool manual = (m_proxyModeCombo->currentData().toInt() ==
                         static_cast<int>(config::ProxyMode::ManualProxy));
    m_proxyProtoCombo->setEnabled(manual);
    m_proxyHostEdit->setEnabled(manual);
    m_proxyPortSpin->setEnabled(manual);
    m_proxyUserEdit->setEnabled(manual);
    m_proxyPassEdit->setEnabled(manual);
    m_proxyBypassEdit->setEnabled(manual);
}

void ProxyAndLoginsPage::refreshVaultTable() {
    m_vaultTable->setRowCount(0);
    const auto entries = config::SiteLoginVault::loadAllEntries();
    for (const auto& entry : entries) {
        const int r = m_vaultTable->rowCount();
        m_vaultTable->insertRow(r);
        m_vaultTable->setItem(r, 0, new QTableWidgetItem(entry.domain));
        m_vaultTable->setItem(r, 1, new QTableWidgetItem(entry.username));
        m_vaultTable->setItem(r, 2, new QTableWidgetItem(QStringLiteral("DPAPI Protected")));
    }
}

void ProxyAndLoginsPage::onAddSiteLoginClicked() {
    const QString dom = m_siteDomainEdit->text().trimmed().toLower();
    const QString usr = m_siteUserEdit->text().trimmed();
    const QString pwd = m_sitePassEdit->text();
    if (dom.isEmpty() || usr.isEmpty()) return;

    config::SiteLoginVault::upsertEntry(dom, usr, pwd);
    m_siteDomainEdit->clear();
    m_siteUserEdit->clear();
    m_sitePassEdit->clear();
    refreshVaultTable();
}

void ProxyAndLoginsPage::onRemoveSiteLoginClicked() {
    const int r = m_vaultTable->currentRow();
    if (r < 0) return;
    const QString dom = m_vaultTable->item(r, 0)->text();
    config::SiteLoginVault::removeEntry(dom);
    refreshVaultTable();
}

void ProxyAndLoginsPage::applyToConfig() {
    auto& cfg = config::AppConfig::instance();
    cfg.setProxyMode(static_cast<config::ProxyMode>(m_proxyModeCombo->currentData().toInt()));
    cfg.setProxyProtocol(static_cast<config::ProxyProtocol>(m_proxyProtoCombo->currentData().toInt()));
    cfg.setProxyHost(m_proxyHostEdit->text().trimmed());
    cfg.setProxyPort(static_cast<quint16>(m_proxyPortSpin->value()));
    cfg.setProxyUsername(m_proxyUserEdit->text().trimmed());
    cfg.setProxyPassword(m_proxyPassEdit->text());

    QStringList bypass;
    for (const QString& item : m_proxyBypassEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bypass.append(item.trimmed());
    }
    cfg.setProxyBypassList(bypass);
}

} // namespace idm::ui::settings
