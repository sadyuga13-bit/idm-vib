#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>
#include <algorithm>
#include <array>
#include <cstdint>

namespace idm::security {

enum class LicenseTier : int {
    Trial = 0,
    Pro = 1,
    Enterprise = 2
};

enum class ActivationStep : int {
    Idle = 0,
    ValidatingFormat = 1,
    BindingHwid = 2,
    VerifyingSignature = 3,
    SealingVault = 4,
    Completed = 5,
    Failed = 6
};

struct HwidComponents {
    QString smbiosUuidHash;
    QString cpuIdHash;
    QString diskSerialHash;
    QString machineGuidHash;

    [[nodiscard]] QString compositeFingerprint() const {
        return QStringLiteral("%1-%2-%3-%4")
            .arg(smbiosUuidHash.left(8),
                 cpuIdHash.left(8),
                 diskSerialHash.left(8),
                 machineGuidHash.left(8))
            .toUpper();
    }

    [[nodiscard]] QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("smbios")] = smbiosUuidHash;
        obj[QStringLiteral("cpuid")] = cpuIdHash;
        obj[QStringLiteral("disk")] = diskSerialHash;
        obj[QStringLiteral("mguid")] = machineGuidHash;
        return obj;
    }

    [[nodiscard]] static HwidComponents fromJson(const QJsonObject& obj) {
        HwidComponents c;
        c.smbiosUuidHash = obj.value(QStringLiteral("smbios")).toString();
        c.cpuIdHash = obj.value(QStringLiteral("cpuid")).toString();
        c.diskSerialHash = obj.value(QStringLiteral("disk")).toString();
        c.machineGuidHash = obj.value(QStringLiteral("mguid")).toString();
        return c;
    }
};

struct LicenseCertificate {
    QString licenseId;
    QString email;
    QString serialKey;
    LicenseTier tier = LicenseTier::Trial;
    HwidComponents boundHwid;
    qint64 issuedAtUnix = 0;
    qint64 expiresAtUnix = 0;
    int maxAllowedThreads = 2;
    QString serverNonceHex;
    QByteArray signatureEd25519;

    [[nodiscard]] QByteArray canonicalSigningPayload() const {
        QJsonObject obj;
        obj[QStringLiteral("licenseId")] = licenseId;
        obj[QStringLiteral("email")] = email.trimmed().toLower();
        obj[QStringLiteral("serialKey")] = serialKey.trimmed().toUpper();
        obj[QStringLiteral("tier")] = static_cast<int>(tier);
        obj[QStringLiteral("boundHwid")] = boundHwid.toJson();
        obj[QStringLiteral("issuedAtUnix")] = issuedAtUnix;
        obj[QStringLiteral("expiresAtUnix")] = expiresAtUnix;
        obj[QStringLiteral("maxAllowedThreads")] = maxAllowedThreads;
        obj[QStringLiteral("serverNonceHex")] = serverNonceHex;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    [[nodiscard]] QByteArray toSignedJsonBytes() const {
        QJsonObject root = QJsonDocument::fromJson(canonicalSigningPayload()).object();
        root[QStringLiteral("signatureHex")] = QString::fromLatin1(signatureEd25519.toHex());
        return QJsonDocument(root).toJson(QJsonDocument::Compact);
    }

    [[nodiscard]] static LicenseCertificate fromSignedJsonBytes(const QByteArray& raw) {
        LicenseCertificate cert;
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        if (!doc.isObject()) {
            return cert;
        }
        const QJsonObject obj = doc.object();
        cert.licenseId = obj.value(QStringLiteral("licenseId")).toString();
        cert.email = obj.value(QStringLiteral("email")).toString();
        cert.serialKey = obj.value(QStringLiteral("serialKey")).toString();
        cert.tier = static_cast<LicenseTier>(obj.value(QStringLiteral("tier")).toInt(0));
        cert.boundHwid = HwidComponents::fromJson(obj.value(QStringLiteral("boundHwid")).toObject());
        cert.issuedAtUnix = obj.value(QStringLiteral("issuedAtUnix")).toVariant().toLongLong();
        cert.expiresAtUnix = obj.value(QStringLiteral("expiresAtUnix")).toVariant().toLongLong();
        cert.maxAllowedThreads = obj.value(QStringLiteral("maxAllowedThreads")).toInt(2);
        cert.serverNonceHex = obj.value(QStringLiteral("serverNonceHex")).toString();
        cert.signatureEd25519 = QByteArray::fromHex(
            obj.value(QStringLiteral("signatureHex")).toString().toLatin1()
        );
        return cert;
    }
};

struct SeatInfo {
    QString seatId;
    QString deviceName;
    QString hwidPrefix;
    QString osName;
    QString activatedAtIso;
    bool isCurrentDevice = false;
};

struct RuntimeCapabilityToken {
    std::array<uint8_t, 32> derivedKey{};
    uint64_t encodedCapabilityWord{0};
    uint64_t integrityEpochTag{0};

    [[nodiscard]] uint64_t keyStreamWord() const noexcept {
        uint64_t w = 0;
        for (int i = 0; i < 8; ++i) {
            w |= (static_cast<uint64_t>(derivedKey[static_cast<size_t>(i)]) << (i * 8));
        }
        return w ^ integrityEpochTag;
    }

    [[nodiscard]] int resolvePermittedSegments(int userRequested) const noexcept {
        const uint64_t unmasked = encodedCapabilityWord ^ keyStreamWord();
        const uint32_t magicHeader = static_cast<uint32_t>((unmasked >> 32) & 0xFFFFFFFFULL);
        if (magicHeader != 0x49444D56U) { // 'IDMV'
            return std::clamp(userRequested, 1, 2);
        }
        const int cap = static_cast<int>(unmasked & 0xFFULL);
        return std::clamp(userRequested, 1, std::clamp(cap, 1, 32));
    }

    [[nodiscard]] qint64 resolveInFlightSplitThreshold() const noexcept {
        const uint64_t unmasked = encodedCapabilityWord ^ keyStreamWord();
        const uint32_t magicHeader = static_cast<uint32_t>((unmasked >> 32) & 0xFFFFFFFFULL);
        if (magicHeader != 0x49444D56U) {
            return (1LL << 60); // Disables dynamic work-stealing in trial/tampered state
        }
        const uint32_t splitFlag = static_cast<uint32_t>((unmasked >> 8) & 0xFFULL);
        return (splitFlag == 0xA5U) ? 1048576LL : (1LL << 60);
    }

    void poisonSilently() noexcept {
        integrityEpochTag ^= 0xDEAD10CC55AA3377ULL;
        derivedKey[0] ^= 0xFFU;
    }
};

} // namespace idm::security
