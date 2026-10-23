#include "config/SiteLoginVault.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace idm::config {

QList<SiteLoginEntry> SiteLoginVault::loadAllEntries() {
    QSettings settings(QStringLiteral("SystemsArchitecture"), QStringLiteral("IdmVibe"));
    const QByteArray encryptedBlob = settings.value(QStringLiteral("vault/siteLoginsDpapi")).toByteArray();
    if (encryptedBlob.isEmpty()) {
        return {};
    }
    const QByteArray plainJson = unprotectBytesDpapi(encryptedBlob);
    const QJsonDocument doc = QJsonDocument::fromJson(plainJson);
    if (!doc.isArray()) {
        return {};
    }

    QList<SiteLoginEntry> list;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject obj = v.toObject();
        SiteLoginEntry e;
        e.domain = obj.value(QStringLiteral("domain")).toString().trimmed().toLower();
        e.username = obj.value(QStringLiteral("username")).toString().trimmed();
        e.password = obj.value(QStringLiteral("password")).toString();
        if (!e.domain.isEmpty()) {
            list.append(e);
        }
    }
    return list;
}

void SiteLoginVault::saveAllEntries(const QList<SiteLoginEntry>& entries) {
    QJsonArray arr;
    for (const SiteLoginEntry& e : entries) {
        if (e.domain.trimmed().isEmpty()) {
            continue;
        }
        QJsonObject obj;
        obj[QStringLiteral("domain")] = e.domain.trimmed().toLower();
        obj[QStringLiteral("username")] = e.username.trimmed();
        obj[QStringLiteral("password")] = e.password;
        arr.append(obj);
    }
    const QByteArray plain = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    const QByteArray cipher = protectBytesDpapi(plain);
    QSettings settings(QStringLiteral("SystemsArchitecture"), QStringLiteral("IdmVibe"));
    settings.setValue(QStringLiteral("vault/siteLoginsDpapi"), cipher);
    settings.sync();
}

void SiteLoginVault::upsertEntry(const QString& domain, const QString& username, const QString& password) {
    const QString cleanDomain = domain.trimmed().toLower();
    if (cleanDomain.isEmpty()) {
        return;
    }
    QList<SiteLoginEntry> list = loadAllEntries();
    for (SiteLoginEntry& e : list) {
        if (e.domain == cleanDomain) {
            e.username = username.trimmed();
            e.password = password;
            saveAllEntries(list);
            return;
        }
    }
    list.append(SiteLoginEntry{cleanDomain, username.trimmed(), password});
    saveAllEntries(list);
}

void SiteLoginVault::removeEntry(const QString& domain) {
    const QString cleanDomain = domain.trimmed().toLower();
    QList<SiteLoginEntry> list = loadAllEntries();
    list.removeIf([&cleanDomain](const SiteLoginEntry& e) { return e.domain == cleanDomain; });
    saveAllEntries(list);
}

bool SiteLoginVault::findCredentialsForHost(const QString& host, QString* userOut, QString* passOut) {
    const QString h = host.trimmed().toLower();
    const QList<SiteLoginEntry> list = loadAllEntries();
    for (const SiteLoginEntry& e : list) {
        if (h == e.domain || h.endsWith(QLatin1Char('.') + e.domain)) {
            if (userOut != nullptr) { *userOut = e.username; }
            if (passOut != nullptr) { *passOut = e.password; }
            return true;
        }
    }
    return false;
}

QByteArray SiteLoginVault::protectBytesDpapi(const QByteArray& plain) {
#ifdef _WIN32
    DATA_BLOB inBlob{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()))};
    DATA_BLOB outBlob{};
    if (CryptProtectData(&inBlob, L"IdmVibeSiteVault", nullptr, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        QByteArray out(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
        LocalFree(outBlob.pbData);
        return out;
    }
#endif
    return plain;
}

QByteArray SiteLoginVault::unprotectBytesDpapi(const QByteArray& cipher) {
#ifdef _WIN32
    DATA_BLOB inBlob{static_cast<DWORD>(cipher.size()), reinterpret_cast<BYTE*>(const_cast<char*>(cipher.data()))};
    DATA_BLOB outBlob{};
    if (CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr,
                           CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        QByteArray out(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
        LocalFree(outBlob.pbData);
        return out;
    }
#endif
    return cipher;
}

} // namespace idm::config
