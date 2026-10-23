#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

namespace idm::ui::settings {

class ProxyAndLoginsPage final : public QWidget {
    Q_OBJECT

public:
    explicit ProxyAndLoginsPage(QWidget* parent = nullptr);
    ~ProxyAndLoginsPage() override = default;

    void applyToConfig();

private:
    void updateProxyFieldsState();
    void refreshVaultTable();
    void onAddSiteLoginClicked();
    void onRemoveSiteLoginClicked();

    QComboBox* m_proxyModeCombo{nullptr};
    QComboBox* m_proxyProtoCombo{nullptr};
    QLineEdit* m_proxyHostEdit{nullptr};
    QSpinBox* m_proxyPortSpin{nullptr};
    QLineEdit* m_proxyUserEdit{nullptr};
    QLineEdit* m_proxyPassEdit{nullptr};
    QLineEdit* m_proxyBypassEdit{nullptr};

    QLineEdit* m_siteDomainEdit{nullptr};
    QLineEdit* m_siteUserEdit{nullptr};
    QLineEdit* m_sitePassEdit{nullptr};
    QTableWidget* m_vaultTable{nullptr};
};

} // namespace idm::ui::settings
