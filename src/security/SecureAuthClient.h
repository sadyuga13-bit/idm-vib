#pragma once

#include "security/SecurityTypes.h"
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

namespace idm::security {

class SecureAuthClient final : public QObject {
    Q_OBJECT

public:
    explicit SecureAuthClient(QObject* parent = nullptr);
    ~SecureAuthClient() override;

    void setEndpointBaseUrl(const QUrl& baseUrl);
    [[nodiscard]] QUrl endpointBaseUrl() const { return m_baseUrl; }

    void sendSerialActivationRequest(
        const QString& email,
        const QString& serialKey,
        const HwidComponents& hwid
    );

    void sendAccountSignInRequest(
        const QString& email,
        const QString& password,
        const QString& totpCode,
        const HwidComponents& hwid
    );

    void sendDeactivateSeatRequest(const QString& seatId);

    void abortActiveRequests();

    [[nodiscard]] static bool verifyServerSpkiPin(const QNetworkReply* reply);

signals:
    void activationResponseReceived(
        bool success,
        const idm::security::LicenseCertificate& cert,
        const QString& errorMessage
    );
    void accountSeatsReceived(
        bool success,
        const QList<idm::security::SeatInfo>& seats,
        int maxSeats,
        const QString& statusMessage
    );
    void seatDeactivated(bool success, const QString& seatId, const QString& message);

private:
    QNetworkRequest buildPinnedTls13Request(
        const QString& pathSuffix,
        const QString& challengeNonceHex
    ) const;

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_activeReply;
    QUrl m_baseUrl{QStringLiteral("https://licensing.idm-vibe.internal/api/v1")};
    QString m_lastChallengeNonceHex;
    QString m_sessionBearerToken;
};

} // namespace idm::security
