#include "ipc/BrowserIpcServer.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace idm::ipc {

BrowserIpcServer::BrowserIpcServer(QObject* parent)
    : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &BrowserIpcServer::onNewConnection);
}

BrowserIpcServer::~BrowserIpcServer() {
    stopListening();
}

bool BrowserIpcServer::startListening(quint16 port) {
    if (m_server.isListening()) {
        return true;
    }
    return m_server.listen(QHostAddress::LocalHost, port);
}

void BrowserIpcServer::stopListening() {
    if (m_server.isListening()) {
        m_server.close();
    }
}

bool BrowserIpcServer::isListening() const noexcept {
    return m_server.isListening();
}

quint16 BrowserIpcServer::serverPort() const noexcept {
    return m_server.serverPort();
}

void BrowserIpcServer::onNewConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket* socket = m_server.nextPendingConnection();
        if (socket == nullptr) {
            continue;
        }
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            processClientBuffer(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void BrowserIpcServer::processClientBuffer(QTcpSocket* socket) {
    const QByteArray raw = socket->readAll();
    if (raw.isEmpty()) {
        return;
    }

    if (raw.startsWith("OPTIONS ")) {
        writeHttpResponse(socket, 204, QByteArray());
        return;
    }

    QByteArray payload = raw;
    const int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd >= 0) {
        payload = raw.mid(headerEnd + 4);
    }

    const bool ok = parseAndEmitJsonPayload(payload.trimmed());
    if (ok) {
        writeHttpResponse(socket, 200, R"({"status":"intercepted"})");
    } else {
        writeHttpResponse(socket, 400, R"({"status":"error","message":"invalid_json"})");
    }
}

bool BrowserIpcServer::parseAndEmitJsonPayload(const QByteArray& jsonBody) {
    if (jsonBody.isEmpty()) {
        return false;
    }

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBody, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject obj = doc.object();
    const QString urlStr = obj.value(QStringLiteral("url")).toString().trimmed();
    const QUrl url(urlStr);
    if (!url.isValid() || url.scheme().isEmpty()) {
        return false;
    }

    network::RequestProfile profile;
    const QString ua = obj.value(QStringLiteral("userAgent")).toString().trimmed();
    if (!ua.isEmpty()) {
        profile.userAgent = ua;
    }
    profile.referer = obj.value(QStringLiteral("referer")).toString().trimmed();
    profile.cookies = obj.value(QStringLiteral("cookies")).toString().trimmed();

    const QJsonObject headersObj = obj.value(QStringLiteral("headers")).toObject();
    for (auto it = headersObj.begin(); it != headersObj.end(); ++it) {
        profile.customHeaders.insert(it.key(), it.value().toString());
    }

    const QString suggestedName = obj.value(QStringLiteral("fileName")).toString().trimmed();
    emit browserDownloadIntercepted(urlStr, suggestedName, profile);
    return true;
}

void BrowserIpcServer::writeHttpResponse(
    QTcpSocket* socket,
    int statusCode,
    const QByteArray& body
) {
    const QByteArray statusLine = (statusCode == 204)
                                      ? QByteArray("HTTP/1.1 204 No Content\r\n")
                                      : (statusCode == 200
                                             ? QByteArray("HTTP/1.1 200 OK\r\n")
                                             : QByteArray("HTTP/1.1 400 Bad Request\r\n"));

    QByteArray response = statusLine;
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

} // namespace idm::ipc
