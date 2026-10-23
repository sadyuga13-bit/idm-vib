#pragma once

#include <QLocalServer>
#include <QObject>
#include <QString>

namespace idm::system {

class SingleInstanceGuard final : public QObject {
    Q_OBJECT

public:
    explicit SingleInstanceGuard(
        const QString& serverName = QStringLiteral("IdmVibe_SingleInstance_V1"),
        QObject* parent = nullptr
    );
    ~SingleInstanceGuard() override;

    bool tryAcquirePrimaryInstance(const QString& forwardPayload = QString());
    [[nodiscard]] static bool hasSilentTrayArg(const QStringList& args);

signals:
    void secondaryInstanceLaunched(const QString& payload);

private:
    void onNewConnection();

    QString m_serverName;
    QLocalServer m_server;
};

} // namespace idm::system
