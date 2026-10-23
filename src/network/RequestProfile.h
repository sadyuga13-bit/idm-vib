#pragma once

#include <QAuthenticator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QString>

namespace idm::network {

struct RequestProfile {
    QString userAgent = QStringLiteral(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36"
    );
    QString referer;
    QString cookies;
    QMap<QString, QString> customHeaders;

    QString authUser;
    QString authPassword;

    bool proxyEnabled = false;
    QString proxyHost;
    quint16 proxyPort = 8080;
    QString proxyUser;
    QString proxyPassword;

    void applyToRequest(QNetworkRequest& request) const {
        request.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy
        );
        request.setMaximumRedirectsAllowed(16);

        if (!userAgent.trimmed().isEmpty()) {
            request.setHeader(QNetworkRequest::UserAgentHeader, userAgent.trimmed().toUtf8());
        }
        if (!referer.trimmed().isEmpty()) {
            request.setRawHeader("Referer", referer.trimmed().toUtf8());
        }
        if (!cookies.trimmed().isEmpty()) {
            request.setRawHeader("Cookie", cookies.trimmed().toUtf8());
        }
        if (!authUser.isEmpty()) {
            const QByteArray credentials = (authUser + QLatin1Char(':') + authPassword).toUtf8().toBase64();
            request.setRawHeader("Authorization", "Basic " + credentials);
        }

        for (auto it = customHeaders.cbegin(); it != customHeaders.cend(); ++it) {
            const QByteArray key = it.key().trimmed().toUtf8();
            const QByteArray val = it.value().trimmed().toUtf8();
            if (!key.isEmpty()) {
                request.setRawHeader(key, val);
            }
        }
    }

    void applyProxyToManager(QNetworkAccessManager* manager) const {
        if (manager == nullptr) {
            return;
        }
        if (proxyEnabled && !proxyHost.trimmed().isEmpty()) {
            QNetworkProxy proxy(
                QNetworkProxy::HttpProxy,
                proxyHost.trimmed(),
                proxyPort,
                proxyUser,
                proxyPassword
            );
            manager->setProxy(proxy);
        } else {
            manager->setProxy(QNetworkProxy::applicationProxy());
        }
    }

    [[nodiscard]] QString toJsonString() const {
        QJsonObject root;
        root[QStringLiteral("userAgent")] = userAgent;
        root[QStringLiteral("referer")] = referer;
        root[QStringLiteral("cookies")] = cookies;
        root[QStringLiteral("authUser")] = authUser;
        root[QStringLiteral("authPassword")] = authPassword;
        root[QStringLiteral("proxyEnabled")] = proxyEnabled;
        root[QStringLiteral("proxyHost")] = proxyHost;
        root[QStringLiteral("proxyPort")] = static_cast<int>(proxyPort);
        root[QStringLiteral("proxyUser")] = proxyUser;
        root[QStringLiteral("proxyPassword")] = proxyPassword;

        QJsonObject headersObj;
        for (auto it = customHeaders.cbegin(); it != customHeaders.cend(); ++it) {
            headersObj[it.key()] = it.value();
        }
        root[QStringLiteral("customHeaders")] = headersObj;
        return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    }

    [[nodiscard]] static RequestProfile fromJsonString(const QString& json) {
        RequestProfile profile;
        if (json.trimmed().isEmpty()) {
            return profile;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (!doc.isObject()) {
            return profile;
        }
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("userAgent"))) {
            profile.userAgent = root[QStringLiteral("userAgent")].toString(profile.userAgent);
        }
        profile.referer = root[QStringLiteral("referer")].toString();
        profile.cookies = root[QStringLiteral("cookies")].toString();
        profile.authUser = root[QStringLiteral("authUser")].toString();
        profile.authPassword = root[QStringLiteral("authPassword")].toString();
        profile.proxyEnabled = root[QStringLiteral("proxyEnabled")].toBool(false);
        profile.proxyHost = root[QStringLiteral("proxyHost")].toString();
        profile.proxyPort = static_cast<quint16>(root[QStringLiteral("proxyPort")].toInt(8080));
        profile.proxyUser = root[QStringLiteral("proxyUser")].toString();
        profile.proxyPassword = root[QStringLiteral("proxyPassword")].toString();

        const QJsonObject headersObj = root[QStringLiteral("customHeaders")].toObject();
        for (auto it = headersObj.begin(); it != headersObj.end(); ++it) {
            profile.customHeaders.insert(it.key(), it.value().toString());
        }
        return profile;
    }
};

} // namespace idm::network
