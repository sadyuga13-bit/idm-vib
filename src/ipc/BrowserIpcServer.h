#pragma once

#include "network/RequestProfile.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

namespace idm::ipc {

class BrowserIpcServer final : public QObject {
    Q_OBJECT

public:
    explicit BrowserIpcServer(QObject* parent = nullptr);
    ~BrowserIpcServer() override;

    bool startListening(quint16 port = 16800);
    void stopListening();
    [[nodiscard]] bool isListening() const noexcept;
    [[nodiscard]] quint16 serverPort() const noexcept;

signals:
    void browserDownloadIntercepted(
        const QString& url,
        const QString& suggestedFileName,
        const idm::network::RequestProfile& profile
    );

private:
    void onNewConnection();
    void processClientBuffer(QTcpSocket* socket);
    bool parseAndEmitJsonPayload(const QByteArray& jsonBody);
    static void writeHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& body);

    QTcpServer m_server;
};

} // namespace idm::ipc
