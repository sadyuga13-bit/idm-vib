#include "core/AppSettings.h"
#include "storage/CategoryRouter.h"

#include <algorithm>

namespace idm::core {

AppSettings& AppSettings::instance() {
    static AppSettings s_instance;
    return s_instance;
}

AppSettings::AppSettings()
    : m_settings(QStringLiteral("SystemsArchitecture"), QStringLiteral("IdmVibe")) {}

bool AppSettings::isDayMode() const {
    return m_settings.value(QStringLiteral("ui/dayMode"), false).toBool();
}

void AppSettings::setDayMode(bool enabled) {
    m_settings.setValue(QStringLiteral("ui/dayMode"), enabled);
}

QString AppSettings::defaultSaveDirectory() const {
    const QString fallback = storage::CategoryRouter::defaultDirectoryForCategory(FileCategory::All);
    const QString val = m_settings.value(QStringLiteral("downloads/defaultDir"), fallback).toString().trimmed();
    return val.isEmpty() ? fallback : val;
}

void AppSettings::setDefaultSaveDirectory(const QString& dir) {
    m_settings.setValue(QStringLiteral("downloads/defaultDir"), dir.trimmed());
}

int AppSettings::defaultSegments() const {
    return std::clamp(m_settings.value(QStringLiteral("downloads/defaultSegments"), 8).toInt(), 1, 32);
}

void AppSettings::setDefaultSegments(int segments) {
    m_settings.setValue(QStringLiteral("downloads/defaultSegments"), std::clamp(segments, 1, 32));
}

int AppSettings::maxConcurrentTasks() const {
    return std::clamp(m_settings.value(QStringLiteral("downloads/maxConcurrent"), 4).toInt(), 1, 16);
}

void AppSettings::setMaxConcurrentTasks(int count) {
    m_settings.setValue(QStringLiteral("downloads/maxConcurrent"), std::clamp(count, 1, 16));
}

qint64 AppSettings::globalSpeedLimitKiB() const {
    return std::max<qint64>(0, m_settings.value(QStringLiteral("downloads/speedLimitKiB"), 0).toLongLong());
}

void AppSettings::setGlobalSpeedLimitKiB(qint64 kibPerSec) {
    m_settings.setValue(QStringLiteral("downloads/speedLimitKiB"), std::max<qint64>(0, kibPerSec));
}

bool AppSettings::clipboardMonitoringEnabled() const {
    return m_settings.value(QStringLiteral("ipc/clipboardMonitor"), true).toBool();
}

void AppSettings::setClipboardMonitoringEnabled(bool enabled) {
    m_settings.setValue(QStringLiteral("ipc/clipboardMonitor"), enabled);
}

quint16 AppSettings::ipcPort() const {
    const int p = m_settings.value(QStringLiteral("ipc/port"), 16800).toInt();
    return static_cast<quint16>(std::clamp(p, 1024, 65535));
}

void AppSettings::setIpcPort(quint16 port) {
    m_settings.setValue(QStringLiteral("ipc/port"), static_cast<int>(port));
}

QString AppSettings::preferredYoutubeQuality() const {
    return m_settings.value(QStringLiteral("youtube/preferredQuality"), QStringLiteral("muxed_mp4")).toString();
}

void AppSettings::setPreferredYoutubeQuality(const QString& qualityTag) {
    m_settings.setValue(QStringLiteral("youtube/preferredQuality"), qualityTag);
}

QString AppSettings::customUserAgent() const {
    return m_settings.value(
        QStringLiteral("network/userAgent"),
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36")
    ).toString();
}

void AppSettings::setCustomUserAgent(const QString& ua) {
    m_settings.setValue(QStringLiteral("network/userAgent"), ua.trimmed());
}

bool AppSettings::proxyEnabled() const {
    return m_settings.value(QStringLiteral("network/proxyEnabled"), false).toBool();
}

void AppSettings::setProxyEnabled(bool enabled) {
    m_settings.setValue(QStringLiteral("network/proxyEnabled"), enabled);
}

QString AppSettings::proxyHost() const {
    return m_settings.value(QStringLiteral("network/proxyHost"), QStringLiteral("127.0.0.1")).toString();
}

void AppSettings::setProxyHost(const QString& host) {
    m_settings.setValue(QStringLiteral("network/proxyHost"), host.trimmed());
}

quint16 AppSettings::proxyPort() const {
    const int p = m_settings.value(QStringLiteral("network/proxyPort"), 8080).toInt();
    return static_cast<quint16>(std::clamp(p, 1, 65535));
}

void AppSettings::setProxyPort(quint16 port) {
    m_settings.setValue(QStringLiteral("network/proxyPort"), static_cast<int>(port));
}

void AppSettings::sync() {
    m_settings.sync();
}

} // namespace idm::core
