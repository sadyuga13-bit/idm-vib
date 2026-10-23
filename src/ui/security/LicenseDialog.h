#pragma once

#include "security/LicenseCoordinator.h"
#include "ui/security/SeatManagerWidget.h"
#include "ui/security/SerialKeyInputWidget.h"
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <array>

namespace idm::ui::security {

class LicenseDialog final : public QDialog {
    Q_OBJECT

public:
    explicit LicenseDialog(
        idm::security::LicenseCoordinator* coordinator,
        QWidget* parent = nullptr
    );
    ~LicenseDialog() override = default;

private:
    QWidget* buildSerialActivationTab();
    QWidget* buildAccountSeatsTab();
    QWidget* buildOfflineActivationTab();

    void onActivationStepChanged(idm::security::ActivationStep step, const QString& detail);
    void refreshHeaderFromCoordinator();
    void onExportHwidReq();
    void onImportOfflineLic();

    idm::security::LicenseCoordinator* m_coordinator{nullptr};

    QLabel* m_lblHeaderStatus{nullptr};
    QLabel* m_lblHeaderHwid{nullptr};
    QTabWidget* m_tabs{nullptr};

    // Tab 1: Serial Activation
    QLineEdit* m_serialEmailEdit{nullptr};
    SerialKeyInputWidget* m_serialWidget{nullptr};
    std::array<QLabel*, 4> m_stepLabels{};
    QLabel* m_lblPipelineDetail{nullptr};
    QPushButton* m_btnActivateOnline{nullptr};

    // Tab 2: Account & Seats
    QLineEdit* m_accEmailEdit{nullptr};
    QLineEdit* m_accPassEdit{nullptr};
    QLineEdit* m_accTotpEdit{nullptr};
    SeatManagerWidget* m_seatManager{nullptr};
};

} // namespace idm::ui::security
