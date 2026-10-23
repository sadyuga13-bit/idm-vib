#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

namespace idm::security {

class CryptoEngine final {
public:
    CryptoEngine() = delete;

    [[nodiscard]] static QByteArray generateCsprngBytes(int byteCount = 32);

    [[nodiscard]] static QByteArray sha256(QByteArrayView data);

    [[nodiscard]] static QByteArray hkdfSha256(
        QByteArrayView inputKeyMaterial,
        QByteArrayView salt,
        QByteArrayView infoContext,
        int outputLength = 32
    );

    [[nodiscard]] static bool verifyEd25519Signature(
        QByteArrayView message,
        QByteArrayView signature64,
        QByteArrayView publicKey32
    );

    // Returns packed format: [12-byte IV/Nonce] || [Ciphertext] || [16-byte GCM Auth Tag]
    [[nodiscard]] static QByteArray encryptAes256Gcm(
        QByteArrayView plaintext,
        QByteArrayView key32,
        QByteArrayView associatedData = {}
    );

    [[nodiscard]] static QByteArray decryptAes256Gcm(
        QByteArrayView packedIvCipherTag,
        QByteArrayView key32,
        QByteArrayView associatedData = {},
        bool* authenticationPassed = nullptr
    );

    [[nodiscard]] static bool constantTimeEquals(
        QByteArrayView lhs,
        QByteArrayView rhs
    ) noexcept;

    [[nodiscard]] static QString computeSerialChecksumSegment(const QString& first20Chars);
    [[nodiscard]] static bool validateSerialKeyChecksum(const QString& formattedSerial);
};

} // namespace idm::security
