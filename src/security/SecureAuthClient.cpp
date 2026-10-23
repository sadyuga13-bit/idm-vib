#include "security/SecureAuthClient.h"
#include "security/CryptoEngine.h"
#include "security/ObfuscatedConstants.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSysInfo>

namespace idm::security {

SecureAuthClient::SecureAuthClient(QObject* parent)
    : QObject(parent) {
    // Reject system proxy interception for licensing traffic to harden against local MITM
    m_nam.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
}

SecureAuthClient::~SecureAuthClient() {
    abortActiveRequests();
}

void SecureAuthClient::setEndpointBaseUrl(const QUrl& baseUrl) {
    m_baseUrl = baseUrl;
}

void SecureAuthClient::abortActiveRequests() {
    if (m_activeReply != nullptr) {
        m_activeReply->disconnect(this);
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply.clear();
    }
}

void SecureAuthClient::sendSerialActivationRequest(
    const QString& email,
    const QString& serialKey,
    const HwidComponents& hwid
) {
    abortActiveRequests();

    const QByteArray nonceBytes = CryptoEngine::generateCsprngBytes(32);
    m_lastChallengeNonceHex = QString::fromLatin1(nonceBytes.toHex());

    QJsonObject payload;
    payload[QStringLiteral("email")] = email.trimmed().toLower();
    payload[QStringLiteral("serialKey")] = serialKey.trimmed().toUpper();
    payload[QStringLiteral("nonceHex")] = m_lastChallengeNonceHex;
    payload[QStringLiteral("timestampUnix")] = QDateTime::currentSecsSinceEpoch();
    payload[QStringLiteral("hostname")] = QSysInfo::machineHostName();
    payload[QStringLiteral("osName")] = QSysInfo::prettyProductName();
    payload[QStringLiteral("hwid")] = hwid.toJson();

    const QNetworkRequest req = buildPinnedTls13Request(
        QStringLiteral("/activate"),
        m_lastChallengeNonceHex
    );
    QNetworkReply* reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    connect(reply, &QNetworkReply::encrypted, this, [this, reply]() {
        if (!verifyServerSpkiPin(reply)) {
            reply->abort();
            emit activationResponseReceived(
                false,
                {},
                QStringLiteral("TLS 1.3 SPKI Certificate Pin mismatch! Potential MITM or hosts redirection blocked.")
            );
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            emit activationResponseReceived(
                false,
                {},
                QStringLiteral("Licensing server unreachable (%1). Use Offline Activation (.idm_lic) if air-gapped.")
                    .arg(reply->errorString())
            );
            return;
        }

        const QByteArray body = reply->readAll();
        const LicenseCertificate cert = LicenseCertificate::fromSignedJsonBytes(body);
        if (!CryptoEngine::constantTimeEquals(
                cert.serverNonceHex.toLatin1(),
                m_lastChallengeNonceHex.toLatin1())) {
            emit activationResponseReceived(
                false,
                {},
                QStringLiteral("Cryptographic challenge nonce mismatch (Replay / Fake Server detected).")
            );
            return;
        }

        emit activationResponseReceived(true, cert, QString());
    });
}

void SecureAuthClient::sendAccountSignInRequest(
    const QString& email,
    const QString& password,
    const QString& totpCode,
    const HwidComponents& hwid
) {
    abortActiveRequests();

    const QByteArray nonceBytes = CryptoEngine::generateCsprngBytes(32);
    m_lastChallengeNonceHex = QString::fromLatin1(nonceBytes.toHex());

    const QByteArray passwordHash = CryptoEngine::hkdfSha256(
        password.toUtf8(),
        email.trimmed().toLower().toUtf8(),
        QByteArrayLiteral("IDM-VIBE-CLIENT-AUTH-PREHASH"),
        32
    );

    QJsonObject payload;
    payload[QStringLiteral("email")] = email.trimmed().toLower();
    payload[QStringLiteral("passwordProofHex")] = QString::fromLatin1(passwordHash.toHex());
    payload[QStringLiteral("totp2fa")] = totpCode.trimmed();
    payload[QStringLiteral("nonceHex")] = m_lastChallengeNonceHex;
    payload[QStringLiteral("hwid")] = hwid.toJson();

    const QNetworkRequest req = buildPinnedTls13Request(
        QStringLiteral("/account/seats"),
        m_lastChallengeNonceHex
    );
    QNetworkReply* reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    connect(reply, &QNetworkReply::encrypted, this, [this, reply]() {
        if (!verifyServerSpkiPin(reply)) {
            reply->abort();
            emit accountSeatsReceived(
                false, {}, 0,
                QStringLiteral("TLS 1.3 SPKI Pin verification failed.")
            );
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit accountSeatsReceived(false, {}, 3, reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject root = doc.object();
        m_sessionBearerToken = root.value(QStringLiteral("sessionToken")).toString();
        const int maxSeats = root.value(QStringLiteral("maxSeats")).toInt(3);

        QList<SeatInfo> seats;
        const QJsonArray arr = root.value(QStringLiteral("seats")).toArray();
        for (const QJsonValue& val : arr) {
            const QJsonObject sObj = val.toObject();
            SeatInfo info;
            info.seatId = sObj.value(QStringLiteral("seatId")).toString();
            info.deviceName = sObj.value(QStringLiteral("deviceName")).toString();
            info.hwidPrefix = sObj.value(QStringLiteral("hwidPrefix")).toString();
            info.osName = sObj.value(QStringLiteral("osName")).toString();
            info.activatedAtIso = sObj.value(QStringLiteral("activatedAt")).toString();
            info.isCurrentDevice = sObj.value(QStringLiteral("isCurrent")).toBool(false);
            seats.append(info);
        }
        emit accountSeatsReceived(true, seats, maxSeats, QStringLiteral("Authenticated"));
    });
}

void SecureAuthClient::sendDeactivateSeatRequest(const QString& seatId) {
    abortActiveRequests();
    const QString nonceHex = QString::fromLatin1(CryptoEngine::generateCsprngBytes(32).toHex());

    QJsonObject payload;
    payload[QStringLiteral("seatId")] = seatId;
    payload[QStringLiteral("nonceHex")] = nonceHex;

    QNetworkRequest req = buildPinnedTls13Request(QStringLiteral("/account/seats/revoke"), nonceHex);
    if (!m_sessionBearerToken.isEmpty()) {
        req.setRawHeader("Authorization", "Bearer " + m_sessionBearerToken.toUtf8());
    }

    QNetworkReply* reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, seatId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit seatDeactivated(false, seatId, reply->errorString());
            return;
        }
        emit seatDeactivated(true, seatId, QStringLiteral("Seat revoked successfully."));
    });
}

bool SecureAuthClient::verifyServerSpkiPin(const QNetworkReply* reply) {
    if (reply == nullptr) {
        return false;
    }
    const QSslCertificate peerCert = reply->sslConfiguration().peerCertificate();
    if (peerCert.isNull()) {
        return false;
    }
    const QByteArray spkiDer = peerCert.publicKey().toDer();
    if (spkiDer.isEmpty()) {
        return false;
    }
    const QByteArray actualSpkiSha256 = CryptoEngine::sha256(spkiDer);
    const auto expectedPin = ObfuscatedConstants::reconstructServerSpkiSha256Pin();
    return CryptoEngine::constantTimeEquals(actualSpkiSha256, expectedPin.toByteArray());
}

QNetworkRequest SecureAuthClient::buildPinnedTls13Request(
    const QString& pathSuffix,
    const QString& challengeNonceHex
) const {
    QUrl url = m_baseUrl;
    url.setPath(m_baseUrl.path() + pathSuffix);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("X-Client-Nonce", challengeNonceHex.toLatin1());
    req.setRawHeader("X-Protocol-Security", "TLS1.3-SPKI-Ed25519");

    QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
    sslCfg.setProtocol(QSsl::TlsV1_3OrLater);
    sslCfg.setPeerVerifyMode(QSslSocket::VerifyPeer);
    req.setSslConfiguration(sslCfg);
    return req;
}

} // namespace idm::security
