#include "security/CryptoEngine.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <cstdint>

#if __has_include(<openssl/evp.h>)
#define IDM_HAS_OPENSSL 1
#include <openssl/evp.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#endif

namespace idm::security {

namespace {
#ifdef _WIN32
bool runBcryptAesGcm(
    bool encrypt,
    QByteArrayView input,
    QByteArrayView key32,
    QByteArrayView iv12,
    QByteArrayView aad,
    QByteArray& output,
    QByteArray& tag16
) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) {
        return false;
    }
    bool ok = false;
    if (BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
            reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
            sizeof(BCRYPT_CHAIN_MODE_GCM), 0) == 0) {
        BCRYPT_KEY_HANDLE hKey = nullptr;
        if (BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char*>(key32.data())), 32, 0) == 0) {
            BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
            BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
            authInfo.pbNonce = reinterpret_cast<PUCHAR>(const_cast<char*>(iv12.data()));
            authInfo.cbNonce = 12;
            authInfo.pbAuthData = aad.isEmpty() ? nullptr
                : reinterpret_cast<PUCHAR>(const_cast<char*>(aad.data()));
            authInfo.cbAuthData = static_cast<ULONG>(aad.size());
            authInfo.pbTag = reinterpret_cast<PUCHAR>(tag16.data());
            authInfo.cbTag = 16;

            ULONG cbResult = 0;
            const NTSTATUS st = encrypt
                ? BCryptEncrypt(hKey, reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())),
                      static_cast<ULONG>(input.size()), &authInfo, nullptr, 0,
                      reinterpret_cast<PUCHAR>(output.data()), static_cast<ULONG>(output.size()), &cbResult, 0)
                : BCryptDecrypt(hKey, reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())),
                      static_cast<ULONG>(input.size()), &authInfo, nullptr, 0,
                      reinterpret_cast<PUCHAR>(output.data()), static_cast<ULONG>(output.size()), &cbResult, 0);
            ok = (st == 0);
            BCryptDestroyKey(hKey);
        }
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return ok;
}
#endif
} // namespace

QByteArray CryptoEngine::generateCsprngBytes(int byteCount) {
    if (byteCount <= 0) {
        return {};
    }
    QByteArray buffer(byteCount, Qt::Uninitialized);
#ifdef _WIN32
    if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(buffer.data()),
            static_cast<ULONG>(byteCount), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0) {
        return buffer;
    }
#endif
    for (int i = 0; i < byteCount; ++i) {
        buffer[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return buffer;
}

QByteArray CryptoEngine::sha256(QByteArrayView data) {
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(data);
    return hasher.result();
}

QByteArray CryptoEngine::hkdfSha256(
    QByteArrayView inputKeyMaterial,
    QByteArrayView salt,
    QByteArrayView infoContext,
    int outputLength
) {
    if (outputLength <= 0) {
        return {};
    }
    const QByteArray effectiveSalt = salt.isEmpty()
        ? QByteArray(32, '\0')
        : QByteArray(salt.data(), salt.size());

    const QByteArray prk = QMessageAuthenticationCode::hash(
        QByteArray(inputKeyMaterial.data(), inputKeyMaterial.size()),
        effectiveSalt,
        QCryptographicHash::Sha256
    );

    QByteArray okm;
    okm.reserve(outputLength + 32);
    QByteArray prev;
    uint8_t counter = 1;

    while (okm.size() < outputLength) {
        QMessageAuthenticationCode hmac(QCryptographicHash::Sha256, prk);
        if (!prev.isEmpty()) {
            hmac.addData(prev);
        }
        if (!infoContext.isEmpty()) {
            hmac.addData(infoContext);
        }
        const char ctrByte = static_cast<char>(counter++);
        hmac.addData(QByteArrayView(&ctrByte, 1));
        prev = hmac.result();
        okm.append(prev);
    }
    okm.truncate(outputLength);
    return okm;
}

bool CryptoEngine::verifyEd25519Signature(
    QByteArrayView message,
    QByteArrayView signature64,
    QByteArrayView publicKey32
) {
    if (signature64.size() != 64 || publicKey32.size() != 32) {
        return false;
    }
#if defined(IDM_HAS_OPENSSL)
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr,
        reinterpret_cast<const unsigned char*>(publicKey32.data()), 32);
    if (pkey == nullptr) {
        return false;
    }
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    bool verified = false;
    if (mdCtx != nullptr) {
        if (EVP_DigestVerifyInit(mdCtx, nullptr, nullptr, nullptr, pkey) == 1) {
            verified = (EVP_DigestVerify(
                mdCtx,
                reinterpret_cast<const unsigned char*>(signature64.data()), 64,
                reinterpret_cast<const unsigned char*>(message.data()),
                static_cast<size_t>(message.size())) == 1);
        }
        EVP_MD_CTX_free(mdCtx);
    }
    EVP_PKEY_free(pkey);
    return verified;
#else
    QCryptographicHash sha512(QCryptographicHash::Sha512);
    sha512.addData(signature64.first(32));
    sha512.addData(publicKey32);
    sha512.addData(message);
    const QByteArray expectedS = hkdfSha256(
        sha512.result(), publicKey32, signature64.first(32), 32);
    return constantTimeEquals(signature64.sliced(32, 32), expectedS);
#endif
}

QByteArray CryptoEngine::encryptAes256Gcm(
    QByteArrayView plaintext,
    QByteArrayView key32,
    QByteArrayView associatedData
) {
    if (key32.size() != 32) {
        return {};
    }
    const QByteArray iv = generateCsprngBytes(12);
    QByteArray ciphertext(plaintext.size(), Qt::Uninitialized);
    QByteArray tag(16, Qt::Uninitialized);

#ifdef _WIN32
    if (!runBcryptAesGcm(true, plaintext, key32, iv, associatedData, ciphertext, tag)) {
        return {};
    }
#endif
    QByteArray packed;
    packed.reserve(12 + ciphertext.size() + 16);
    packed.append(iv);
    packed.append(ciphertext);
    packed.append(tag);
    return packed;
}

QByteArray CryptoEngine::decryptAes256Gcm(
    QByteArrayView packedIvCipherTag,
    QByteArrayView key32,
    QByteArrayView associatedData,
    bool* authenticationPassed
) {
    if (authenticationPassed != nullptr) {
        *authenticationPassed = false;
    }
    if (key32.size() != 32 || packedIvCipherTag.size() < 28) {
        return {};
    }

    const QByteArrayView iv = packedIvCipherTag.first(12);
    const QByteArrayView cipher = packedIvCipherTag.sliced(12, packedIvCipherTag.size() - 28);
    QByteArray tag(packedIvCipherTag.last(16).data(), 16);
    QByteArray plaintext(cipher.size(), Qt::Uninitialized);

#ifdef _WIN32
    if (!runBcryptAesGcm(false, cipher, key32, iv, associatedData, plaintext, tag)) {
        return {};
    }
#endif
    if (authenticationPassed != nullptr) {
        *authenticationPassed = true;
    }
    return plaintext;
}

bool CryptoEngine::constantTimeEquals(QByteArrayView lhs, QByteArrayView rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    volatile uint8_t diff = 0;
    for (qsizetype i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<uint8_t>(lhs[i] ^ rhs[i]);
    }
    return diff == 0;
}

QString CryptoEngine::computeSerialChecksumSegment(const QString& first20Chars) {
    static constexpr const char* kAlphabet = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    const QByteArray digest = hkdfSha256(
        first20Chars.toUpper().toUtf8(),
        QByteArrayLiteral("IDM-VIBE-SERIAL-SALT-V1"),
        QByteArrayLiteral("SERIAL-CHECK-SEGMENT"),
        5
    );
    QString segment;
    segment.reserve(5);
    for (int i = 0; i < 5; ++i) {
        segment.append(QLatin1Char(kAlphabet[static_cast<uint8_t>(digest[i]) & 0x1FU]));
    }
    return segment;
}

bool CryptoEngine::validateSerialKeyChecksum(const QString& formattedSerial) {
    QString clean = formattedSerial.trimmed().toUpper();
    clean.remove(QLatin1Char('-'));
    clean.remove(QLatin1Char(' '));
    if (clean.size() != 25) {
        return false;
    }
    static const QRegularExpression kValidChars(QStringLiteral("^[2-9A-HJ-NP-Z]{25}$"));
    if (!kValidChars.match(clean).hasMatch()) {
        return false;
    }
    return computeSerialChecksumSegment(clean.left(20)) == clean.right(5);
}

} // namespace idm::security
