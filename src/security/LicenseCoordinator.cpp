#include "security/LicenseCoordinator.h"
#include "security/CryptoEngine.h"
#include "security/HwidEngine.h"
#include "security/ObfuscatedConstants.h"
#include "security/SecureVault.h"

#include <QDateTime>
#include <QFile>
#include <algorithm>
#include <cstring>

namespace idm::security {

LicenseCoordinator::LicenseCoordinator(QObject* parent)
    : QObject(parent)
    , m_currentHwid(HwidEngine::collectCurrentHwid()) {
    connect(&m_authClient, &SecureAuthClient::activationResponseReceived, this,
        [this](bool ok, const LicenseCertificate& cert, const QString& err) {
            if (!ok) {
                emit activationStepChanged(ActivationStep::Failed, err);
                return;
            }
            emit activationStepChanged(ActivationStep::VerifyingSignature,
                QStringLiteral("Verifying Ed25519 signature & 3-of-4 HWID Quorum..."));
            QString verifyErr;
            if (!verifyAndApplyCertificate(cert, true, &verifyErr)) {
                emit activationStepChanged(ActivationStep::Failed, verifyErr);
                return;
            }
            emit activationStepChanged(ActivationStep::Completed,
                QStringLiteral("License verified & sealed in DPAPI + AES-256-GCM vault."));
        });

    connect(&m_authClient, &SecureAuthClient::accountSeatsReceived,
            this, &LicenseCoordinator::seatsUpdated);

    connect(&m_authClient, &SecureAuthClient::seatDeactivated, this,
        [this](bool ok, const QString& seatId, const QString& msg) {
            emit seatsUpdated(ok, {}, 3, QStringLiteral("%1 (%2)").arg(msg, seatId));
        });

    m_integrityTimer.setInterval(10000);
    connect(&m_integrityTimer, &QTimer::timeout, this, &LicenseCoordinator::onHeartbeatIntegrityCheck);
    m_integrityTimer.start();
}

void LicenseCoordinator::initialize() {
    m_currentHwid = HwidEngine::collectCurrentHwid();
    m_remainingTrialDays = SecureVault::queryRemainingTrialDays();

    LicenseCertificate storedCert;
    if (SecureVault::unsealCertificateFromDisk(m_currentHwid, &storedCert, nullptr)) {
        if (verifyAndApplyCertificate(storedCert, false, nullptr)) {
            return;
        }
    }

    m_activeCert = LicenseCertificate{};
    m_activeCert.tier = LicenseTier::Trial;
    m_activeCert.maxAllowedThreads = (m_remainingTrialDays > 0) ? 32 : 2;
    rebuildCapabilityToken(m_remainingTrialDays > 0);
    emit licenseStateChanged();
}

void LicenseCoordinator::activateWithSerialOnline(const QString& email, const QString& serialKey) {
    emit activationStepChanged(ActivationStep::ValidatingFormat,
        QStringLiteral("Validating 25-character serial key HKDF checksum..."));

    if (email.trimmed().isEmpty() || !email.contains(QLatin1Char('@'))) {
        emit activationStepChanged(ActivationStep::Failed,
            QStringLiteral("Valid account email address is required."));
        return;
    }
    if (!CryptoEngine::validateSerialKeyChecksum(serialKey)) {
        emit activationStepChanged(ActivationStep::Failed,
            QStringLiteral("Serial key checksum verification failed (Segment #5 invalid)."));
        return;
    }

    emit activationStepChanged(ActivationStep::BindingHwid,
        QStringLiteral("Binding 4-way HWID fingerprint (%1)...").arg(m_currentHwid.compositeFingerprint()));
    m_authClient.sendSerialActivationRequest(email, serialKey, m_currentHwid);
}

bool LicenseCoordinator::exportOfflineHwidRequest(
    const QString& email,
    const QString& serialKey,
    const QString& targetFilePath,
    QString* errorOut
) const {
    if (!CryptoEngine::validateSerialKeyChecksum(serialKey)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Invalid serial key checksum.");
        }
        return false;
    }
    const QByteArray payload = HwidEngine::exportOfflineRequestPayload(email, serialKey, m_currentHwid);
    QFile file(targetFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut != nullptr) {
            *errorOut = file.errorString();
        }
        return false;
    }
    file.write(payload);
    file.close();
    return true;
}

bool LicenseCoordinator::importOfflineLicenseFile(const QString& licFilePath, QString* errorOut) {
    emit activationStepChanged(ActivationStep::ValidatingFormat,
        QStringLiteral("Reading signed offline certificate (.idm_lic)..."));

    QFile file(licFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString err = QStringLiteral("Cannot open license file: %1").arg(file.errorString());
        if (errorOut != nullptr) {
            *errorOut = err;
        }
        emit activationStepChanged(ActivationStep::Failed, err);
        return false;
    }
    const QByteArray raw = file.readAll();
    file.close();

    const LicenseCertificate candidate = LicenseCertificate::fromSignedJsonBytes(raw);
    emit activationStepChanged(ActivationStep::VerifyingSignature,
        QStringLiteral("Verifying Ed25519 signature & 3-of-4 HWID Quorum..."));

    QString verifyErr;
    if (!verifyAndApplyCertificate(candidate, true, &verifyErr)) {
        if (errorOut != nullptr) {
            *errorOut = verifyErr;
        }
        emit activationStepChanged(ActivationStep::Failed, verifyErr);
        return false;
    }

    emit activationStepChanged(ActivationStep::Completed,
        QStringLiteral("Offline license verified and sealed in local vault."));
    return true;
}

void LicenseCoordinator::deactivateLocalLicense() {
    SecureVault::purgeVaultFile();
    m_activeCert = LicenseCertificate{};
    m_activeCert.tier = LicenseTier::Trial;
    m_activeCert.maxAllowedThreads = (m_remainingTrialDays > 0) ? 32 : 2;
    rebuildCapabilityToken(m_remainingTrialDays > 0);
    emit licenseStateChanged();
}

void LicenseCoordinator::signInAccount(
    const QString& email,
    const QString& password,
    const QString& totpCode
) {
    m_authClient.sendAccountSignInRequest(email, password, totpCode, m_currentHwid);
}

void LicenseCoordinator::revokeDeviceSeat(const QString& seatId) {
    m_authClient.sendDeactivateSeatRequest(seatId);
}

QString LicenseCoordinator::statusBadgeText() const {
    if (m_activeCert.tier == LicenseTier::Enterprise) {
        return QStringLiteral("ENTERPRISE VERIFIED");
    }
    if (m_activeCert.tier == LicenseTier::Pro) {
        return QStringLiteral("PRO VERIFIED");
    }
    return QStringLiteral("TRIAL: %1 DAYS").arg(m_remainingTrialDays);
}

bool LicenseCoordinator::verifyAndApplyCertificate(
    const LicenseCertificate& candidate,
    bool persistToVault,
    QString* errorOut
) {
    const qint64 nowSec = QDateTime::currentSecsSinceEpoch();
    if (candidate.expiresAtUnix > 0 && nowSec > candidate.expiresAtUnix) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("License certificate has expired.");
        }
        return false;
    }

    int matchedHwid = 0;
    if (!HwidEngine::evaluateQuorum3Of4(m_currentHwid, candidate.boundHwid, &matchedHwid)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("HWID Quorum mismatch (%1/4 matched, minimum 3 required).").arg(matchedHwid);
        }
        return false;
    }

    const auto pubKey = ObfuscatedConstants::reconstructEd25519PublicKey();
    if (!CryptoEngine::verifyEd25519Signature(
            candidate.canonicalSigningPayload(), candidate.signatureEd25519, pubKey.toByteArray())) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Ed25519 cryptographic signature verification failed.");
        }
        return false;
    }

    if (persistToVault) {
        emit activationStepChanged(ActivationStep::SealingVault,
            QStringLiteral("Sealing certificate with Windows DPAPI + AES-256-GCM..."));
        if (!SecureVault::sealCertificateToDisk(candidate, m_currentHwid, errorOut)) {
            return false;
        }
    }

    m_matchedHwidCount = matchedHwid;
    m_activeCert = candidate;
    rebuildCapabilityToken(true);
    emit licenseStateChanged();
    return true;
}

void LicenseCoordinator::rebuildCapabilityToken(bool validLicenseSignature) {
    RuntimeCapabilityToken tok{};
    tok.integrityEpochTag = m_integrityGuard.computeLiveIntegrityEpochTag();

    if (!validLicenseSignature) {
        const QByteArray trialKey = CryptoEngine::hkdfSha256(
            m_currentHwid.compositeFingerprint().toLatin1(),
            QByteArrayLiteral("TRIAL-SALT"),
            QByteArrayLiteral("IDM-VIBE-TRIAL-CAP"), 32);
        std::memcpy(tok.derivedKey.data(), trialKey.constData(), 32);
        tok.encodedCapabilityWord = 0ULL;
        m_capabilityToken = tok;
        return;
    }

    const QByteArray liveCodeHash = m_integrityGuard.computeLiveTextSectionSha256();
    const QByteArray derived = CryptoEngine::hkdfSha256(
        m_activeCert.signatureEd25519,
        m_currentHwid.compositeFingerprint().toLatin1() + liveCodeHash,
        QByteArrayLiteral("IDM-VIBE-RUNTIME-CAPABILITY-TOKEN-256"), 32);
    std::memcpy(tok.derivedKey.data(), derived.constData(), 32);

    const uint64_t allowedThreads = static_cast<uint64_t>(std::clamp(m_activeCert.maxAllowedThreads, 1, 32));
    const uint64_t rawCapWord = (0x49444D56ULL << 32) | (0xA5ULL << 8) | allowedThreads;
    tok.encodedCapabilityWord = rawCapWord ^ tok.keyStreamWord();

    m_integrityGuard.verifyAndEnforceToken(tok);
    m_capabilityToken = tok;
}

void LicenseCoordinator::onHeartbeatIntegrityCheck() {
    m_integrityGuard.verifyAndEnforceToken(m_capabilityToken);
}

} // namespace idm::security
