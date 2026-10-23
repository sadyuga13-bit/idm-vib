#pragma once

#include "security/IntegrityGuard.h"
#include "security/SecureAuthClient.h"
#include "security/SecurityTypes.h"
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

namespace idm::security {

class LicenseCoordinator final : public QObject {
    Q_OBJECT

public:
    explicit LicenseCoordinator(QObject* parent = nullptr);
    ~LicenseCoordinator() override = default;

    void initialize();

    void activateWithSerialOnline(const QString& email, const QString& serialKey);
    bool exportOfflineHwidRequest(
        const QString& email,
        const QString& serialKey,
        const QString& targetFilePath,
        QString* errorOut = nullptr
    ) const;
    bool importOfflineLicenseFile(const QString& licFilePath, QString* errorOut = nullptr);
    void deactivateLocalLicense();

    void signInAccount(
        const QString& email,
        const QString& password,
        const QString& totpCode
    );
    void revokeDeviceSeat(const QString& seatId);

    [[nodiscard]] const RuntimeCapabilityToken& capabilityToken() const noexcept {
        return m_capabilityToken;
    }
    [[nodiscard]] const HwidComponents& currentHwid() const noexcept { return m_currentHwid; }
    [[nodiscard]] const LicenseCertificate& activeCertificate() const noexcept { return m_activeCert; }
    [[nodiscard]] LicenseTier currentTier() const noexcept { return m_activeCert.tier; }
    [[nodiscard]] int remainingTrialDays() const noexcept { return m_remainingTrialDays; }
    [[nodiscard]] int hwidMatchedQuorumCount() const noexcept { return m_matchedHwidCount; }
    [[nodiscard]] QString statusBadgeText() const;

signals:
    void activationStepChanged(idm::security::ActivationStep step, const QString& detailMessage);
    void licenseStateChanged();
    void seatsUpdated(
        bool success,
        const QList<idm::security::SeatInfo>& seats,
        int maxSeats,
        const QString& message
    );

private:
    bool verifyAndApplyCertificate(
        const LicenseCertificate& candidate,
        bool persistToVault,
        QString* errorOut = nullptr
    );
    void rebuildCapabilityToken(bool validLicenseSignature);
    void onHeartbeatIntegrityCheck();

    HwidComponents m_currentHwid;
    LicenseCertificate m_activeCert;
    RuntimeCapabilityToken m_capabilityToken;
    IntegrityGuard m_integrityGuard;
    SecureAuthClient m_authClient;
    QTimer m_integrityTimer;
    int m_remainingTrialDays{14};
    int m_matchedHwidCount{4};
};

} // namespace idm::security
