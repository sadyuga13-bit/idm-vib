#pragma once

#include <QList>
#include <QString>

namespace idm::config {

struct SiteLoginEntry {
    QString domain;
    QString username;
    QString password;
};

class SiteLoginVault final {
public:
    SiteLoginVault() = delete;

    [[nodiscard]] static QList<SiteLoginEntry> loadAllEntries();
    static void saveAllEntries(const QList<SiteLoginEntry>& entries);
    static void upsertEntry(const QString& domain, const QString& username, const QString& password);
    static void removeEntry(const QString& domain);
    [[nodiscard]] static bool findCredentialsForHost(const QString& host, QString* userOut, QString* passOut);

private:
    [[nodiscard]] static QByteArray protectBytesDpapi(const QByteArray& plain);
    [[nodiscard]] static QByteArray unprotectBytesDpapi(const QByteArray& cipher);
};

} // namespace idm::config
