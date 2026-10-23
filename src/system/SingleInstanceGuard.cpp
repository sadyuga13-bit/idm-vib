#include "system/SingleInstanceGuard.h"

#include <QLocalSocket>

namespace idm::system {

SingleInstanceGuard::SingleInstanceGuard(const QString& serverName, QObject* parent)
    : QObject(parent)
    , m_serverName(serverName) {
    connect(&m_server, &QLocalServer::newConnection, this, &SingleInstanceGuard::onNewConnection);
}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_server.isListening()) {
        m_server.close();
        QLocalServer::removeServer(m_serverName);
    }
}

bool SingleInstanceGuard::tryAcquirePrimaryInstance(const QString& forwardPayload) {
    QLocalSocket probe;
    probe.connectToServer(m_serverName, QIODevice::WriteOnly);
    if (probe.waitForConnected(250)) {
        const QByteArray msg = forwardPayload.isEmpty()
            ? QByteArrayLiteral("ACTIVATE")
            : forwardPayload.toUtf8();
        probe.write(msg);
        probe.waitForBytesWritten(250);
        probe.disconnectFromServer();
        return false;
    }

    QLocalServer::removeServer(m_serverName);
    return m_server.listen(m_serverName);
}

bool SingleInstanceGuard::hasSilentTrayArg(const QStringList& args) {
    for (const QString& a : args) {
        if (a.compare(QStringLiteral("--tray"), Qt::CaseInsensitive) == 0 ||
            a.compare(QStringLiteral("-tray"), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void SingleInstanceGuard::onNewConnection() {
    while (m_server.hasPendingConnections()) {
        QLocalSocket* client = m_server.nextPendingConnection();
        if (client == nullptr) {
            continue;
        }
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            const QString payload = QString::fromUtf8(client->readAll()).trimmed();
            emit secondaryInstanceLaunched(payload);
        });
        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
    }
}

} // namespace idm::system
