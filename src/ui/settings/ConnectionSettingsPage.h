#pragma once

#include "network/YoutubeExtractor.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

namespace idm::ui::settings {

class ConnectionSettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit ConnectionSettingsPage(QWidget* parent = nullptr);
    ~ConnectionSettingsPage() override = default;

    void applyToConfig();

private:
    void populateDomainCapsTable();
    void onAddDomainCapClicked();
    void onRemoveDomainCapClicked();
    void onUpdateYtDlpClicked();

    network::YoutubeExtractor m_ytExtractor{this};

    QSpinBox* m_spinDefaultSegments{nullptr};
    QSpinBox* m_spinMaxConcurrent{nullptr};
    QSpinBox* m_spinSpeedLimitKiB{nullptr};
    QSpinBox* m_spinTimeoutSec{nullptr};
    QSpinBox* m_spinRetryDelaySec{nullptr};
    QSpinBox* m_spinMaxRetries{nullptr};
    QSpinBox* m_spinIpcPort{nullptr};
    QComboBox* m_uaPresetCombo{nullptr};
    QLineEdit* m_uaEdit{nullptr};

    QLineEdit* m_domainCapHostEdit{nullptr};
    QSpinBox* m_domainCapSpin{nullptr};
    QTableWidget* m_domainCapsTable{nullptr};

    QLabel* m_lblYtDlpStatus{nullptr};
    QPushButton* m_btnUpdateYtDlp{nullptr};
};

} // namespace idm::ui::settings
