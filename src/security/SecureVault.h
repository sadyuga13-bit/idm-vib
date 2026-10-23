#pragma once

#include "security/SecurityTypes.h"
#include <QByteArray>
#include <QString>

namespace idm::security {

class SecureVault final {
public:
    SecureVault() = default;
    ~SecureVault() = default;

    [[nodiscard]] static bool sealCertificateToDisk(
        const LicenseCertificate& cert,
        const HwidComponents& currentHwid,
        QString* errorOut = nullptr
    );

    [[nodiscard]] static bool unsealCertificateFromDisk(
        const HwidComponents& currentHwid,
        LicenseCertificate* certOut,
        QString* errorOut = nullptr
    );

    static void purgeVaultFile();

    [[nodiscard]] static int queryRemainingTrialDays();

private:
    [[nodiscard]] static QString vaultFilePath();
    [[nodiscard]] static QString dpapiKeyBlobPath();
    [[nodiscard]] static QString trialSealFilePath();

    [[nodiscard]] static QByteArray deriveMachineVaultKey(const HwidComponents& hwid);
    [[nodiscard]] static QByteArray dpapiProtect(const QByteArray& plainBytes);
    [[nodiscard]] static QByteArray dpapiUnprotect(const QByteArray& protectedBlob);
};

} // namespace idm::security
