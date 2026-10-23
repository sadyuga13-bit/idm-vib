#include "security/SecureVault.h"
#include "security/CryptoEngine.h"
#include "security/ObfuscatedConstants.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dpapi.h>
#endif

namespace idm::security {

namespace {
constexpr const char* kVaultAad = "IDM-VIBE-AES256GCM-VAULT-AAD-V1";
} // namespace

bool SecureVault::sealCertificateToDisk(
    const LicenseCertificate& cert,
    const HwidComponents& currentHwid,
    QString* errorOut
) {
    const QByteArray key32 = deriveMachineVaultKey(currentHwid);
    if (key32.size() != 32) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Failed to derive DPAPI machine vault key.");
        }
        return false;
    }

    const QByteArray plainPayload = cert.toSignedJsonBytes();
    const QByteArray encryptedBlob = CryptoEngine::encryptAes256Gcm(
        plainPayload,
        key32,
        QByteArrayView(kVaultAad, static_cast<qsizetype>(qstrlen(kVaultAad)))
    );
    if (encryptedBlob.isEmpty()) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("AES-256-GCM encryption failed while sealing vault.");
        }
        return false;
    }

    QFile file(vaultFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Cannot write license vault file.");
        }
        return false;
    }
    file.write(encryptedBlob);
    file.flush();
    file.close();
    return true;
}

bool SecureVault::unsealCertificateFromDisk(
    const HwidComponents& currentHwid,
    LicenseCertificate* certOut,
    QString* errorOut
) {
    QFile file(vaultFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("No local license vault found.");
        }
        return false;
    }

    const QByteArray encryptedBlob = file.readAll();
    file.close();

    const QByteArray key32 = deriveMachineVaultKey(currentHwid);
    if (key32.size() != 32) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("DPAPI key unseal failed on this machine.");
        }
        return false;
    }

    bool authOk = false;
    const QByteArray plainPayload = CryptoEngine::decryptAes256Gcm(
        encryptedBlob,
        key32,
        QByteArrayView(kVaultAad, static_cast<qsizetype>(qstrlen(kVaultAad))),
        &authOk
    );
    if (!authOk || plainPayload.isEmpty()) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Vault AES-256-GCM authentication tag mismatch.");
        }
        return false;
    }

    if (certOut != nullptr) {
        *certOut = LicenseCertificate::fromSignedJsonBytes(plainPayload);
    }
    return true;
}

void SecureVault::purgeVaultFile() {
    QFile::remove(vaultFilePath());
}

int SecureVault::queryRemainingTrialDays() {
    const QString path = trialSealFilePath();
    const qint64 nowSec = QDateTime::currentSecsSinceEpoch();
    qint64 firstRunSec = nowSec;

    QFile file(path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QByteArray unsealed = dpapiUnprotect(file.readAll());
        file.close();
        if (!unsealed.isEmpty()) {
            bool ok = false;
            const qint64 parsed = unsealed.toLongLong(&ok);
            if (ok && parsed > 1700000000LL) {
                firstRunSec = parsed;
            }
        }
    } else if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(dpapiProtect(QByteArray::number(nowSec)));
        file.close();
    }

    if (nowSec < firstRunSec) {
        return 0; // Clock rollback detected
    }
    const qint64 elapsedDays = (nowSec - firstRunSec) / 86400;
    return static_cast<int>(std::clamp<qint64>(14 - elapsedDays, 0, 14));
}

QString SecureVault::vaultFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.idm-vibe");
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("idm_vibe_license.vault"));
}

QString SecureVault::dpapiKeyBlobPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.idm-vibe");
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("idm_vibe_master.dpapi"));
}

QString SecureVault::trialSealFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.idm-vibe");
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("idm_vibe_epoch.dpapi"));
}

QByteArray SecureVault::deriveMachineVaultKey(const HwidComponents& hwid) {
    QByteArray masterSeed;
    QFile keyFile(dpapiKeyBlobPath());
    if (keyFile.exists() && keyFile.open(QIODevice::ReadOnly)) {
        masterSeed = dpapiUnprotect(keyFile.readAll());
        keyFile.close();
    }

    if (masterSeed.size() != 32) {
        masterSeed = CryptoEngine::generateCsprngBytes(32);
        const QByteArray sealed = dpapiProtect(masterSeed);
        if (!sealed.isEmpty() && keyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            keyFile.write(sealed);
            keyFile.close();
        }
    }

    const QByteArray hwidBind = (hwid.machineGuidHash + hwid.cpuIdHash).toLatin1();
    return CryptoEngine::hkdfSha256(
        masterSeed,
        hwidBind,
        QByteArrayLiteral("IDM-VIBE-DPAPI-VAULT-KEY-V1"),
        32
    );
}

QByteArray SecureVault::dpapiProtect(const QByteArray& plainBytes) {
    if (plainBytes.isEmpty()) {
        return {};
    }
#ifdef _WIN32
    const auto entropy = ObfuscatedConstants::reconstructDpapiEntropy();
    DATA_BLOB inBlob{
        static_cast<DWORD>(plainBytes.size()),
        reinterpret_cast<BYTE*>(const_cast<char*>(plainBytes.constData()))
    };
    DATA_BLOB entBlob{
        static_cast<DWORD>(entropy.size()),
        const_cast<BYTE*>(entropy.data())
    };
    DATA_BLOB outBlob{0, nullptr};

    if (CryptProtectData(&inBlob, L"IdmVibeVault", &entBlob, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        QByteArray result(reinterpret_cast<const char*>(outBlob.pbData),
                          static_cast<qsizetype>(outBlob.cbData));
        LocalFree(outBlob.pbData);
        return result;
    }
#endif
    return plainBytes;
}

QByteArray SecureVault::dpapiUnprotect(const QByteArray& protectedBlob) {
    if (protectedBlob.isEmpty()) {
        return {};
    }
#ifdef _WIN32
    const auto entropy = ObfuscatedConstants::reconstructDpapiEntropy();
    DATA_BLOB inBlob{
        static_cast<DWORD>(protectedBlob.size()),
        reinterpret_cast<BYTE*>(const_cast<char*>(protectedBlob.constData()))
    };
    DATA_BLOB entBlob{
        static_cast<DWORD>(entropy.size()),
        const_cast<BYTE*>(entropy.data())
    };
    DATA_BLOB outBlob{0, nullptr};

    if (CryptUnprotectData(&inBlob, nullptr, &entBlob, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        QByteArray result(reinterpret_cast<const char*>(outBlob.pbData),
                          static_cast<qsizetype>(outBlob.cbData));
        LocalFree(outBlob.pbData);
        return result;
    }
    return {};
#else
    return protectedBlob;
#endif
}

} // namespace idm::security
