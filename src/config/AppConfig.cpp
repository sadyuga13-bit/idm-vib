#include "config/AppConfig.h"
#include "storage/CategoryRouter.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QStandardPaths>
#include <algorithm>

namespace idm::config {

AppConfig& AppConfig::instance() { static AppConfig s_cfg; return s_cfg; }
AppConfig::AppConfig() : m_settings(QStringLiteral("SystemsArchitecture"), QStringLiteral("IdmVibe")) {}

QString AppConfig::languageCode() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/language"), QStringLiteral("auto")).toString(); }
void AppConfig::setLanguageCode(const QString& code) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/language"), code.trimmed()); }
bool AppConfig::isDayMode() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("ui/dayMode"), false).toBool(); }
void AppConfig::setDayMode(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("ui/dayMode"), enabled); }
bool AppConfig::startWithWindows() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/startWithWindows"), false).toBool(); }
void AppConfig::setStartWithWindows(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/startWithWindows"), enabled); }
bool AppConfig::closeToTray() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/closeToTray"), true).toBool(); }
void AppConfig::setCloseToTray(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/closeToTray"), enabled); }
bool AppConfig::preventSystemSleep() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/preventSleep"), true).toBool(); }
void AppConfig::setPreventSystemSleep(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/preventSleep"), enabled); }
bool AppConfig::monitorClipboard() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("ipc/clipboardMonitor"), true).toBool(); }
void AppConfig::setMonitorClipboard(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("ipc/clipboardMonitor"), enabled); }
bool AppConfig::suppressWithAltKey() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/suppressAlt"), true).toBool(); }
void AppConfig::setSuppressWithAltKey(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/suppressAlt"), enabled); }
bool AppConfig::forceWithInsertKey() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("general/forceInsert"), true).toBool(); }
void AppConfig::setForceWithInsertKey(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("general/forceInsert"), enabled); }

QStringList AppConfig::capturedExtensions() const {
    QMutexLocker lock(&m_mutex);
    return m_settings.value(QStringLiteral("files/extensions"), storage::CategoryRouter::defaultCapturedExtensions()).toStringList();
}
void AppConfig::setCapturedExtensions(const QStringList& exts) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("files/extensions"), exts); }
QStringList AppConfig::excludedDomains() const {
    QMutexLocker lock(&m_mutex);
    return m_settings.value(QStringLiteral("files/excludedDomains"), QStringList{QStringLiteral("localhost"), QStringLiteral("127.0.0.1")}).toStringList();
}
void AppConfig::setExcludedDomains(const QStringList& domains) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("files/excludedDomains"), domains); }
DuplicatePolicy AppConfig::duplicatePolicy() const { QMutexLocker lock(&m_mutex); return static_cast<DuplicatePolicy>(m_settings.value(QStringLiteral("files/duplicatePolicy"), 0).toInt()); }
void AppConfig::setDuplicatePolicy(DuplicatePolicy policy) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("files/duplicatePolicy"), static_cast<int>(policy)); }

QString AppConfig::categoryDirectory(core::FileCategory cat) const {
    QMutexLocker lock(&m_mutex);
    const QString def = storage::CategoryRouter::defaultDirectoryForCategory(cat);
    const QString v = m_settings.value(QStringLiteral("paths/cat_%1").arg(static_cast<int>(cat)), def).toString().trimmed();
    return v.isEmpty() ? def : v;
}
void AppConfig::setCategoryDirectory(core::FileCategory cat, const QString& path) {
    QMutexLocker lock(&m_mutex);
    m_settings.setValue(QStringLiteral("paths/cat_%1").arg(static_cast<int>(cat)), path.trimmed());
}
QString AppConfig::tempPartDirectory() const {
    QMutexLocker lock(&m_mutex);
    const QString def = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("IdmVibeParts"));
    return m_settings.value(QStringLiteral("paths/tempPartDir"), def).toString();
}
void AppConfig::setTempPartDirectory(const QString& path) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("paths/tempPartDir"), path.trimmed()); }
bool AppConfig::preallocateDiskSpace() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("paths/preallocateDisk"), true).toBool(); }
void AppConfig::setPreallocateDiskSpace(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("paths/preallocateDisk"), enabled); }

int AppConfig::defaultSegments() const { QMutexLocker lock(&m_mutex); return std::clamp(m_settings.value(QStringLiteral("downloads/defaultSegments"), 8).toInt(), 1, 32); }
void AppConfig::setDefaultSegments(int count) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("downloads/defaultSegments"), std::clamp(count, 1, 32)); }
int AppConfig::maxConcurrentDownloads() const { QMutexLocker lock(&m_mutex); return std::clamp(m_settings.value(QStringLiteral("downloads/maxConcurrent"), 4).toInt(), 1, 16); }
void AppConfig::setMaxConcurrentDownloads(int count) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("downloads/maxConcurrent"), std::clamp(count, 1, 16)); }
qint64 AppConfig::globalSpeedLimitKiB() const { QMutexLocker lock(&m_mutex); return std::max<qint64>(0, m_settings.value(QStringLiteral("downloads/speedLimitKiB"), 0).toLongLong()); }
void AppConfig::setGlobalSpeedLimitKiB(qint64 kib) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("downloads/speedLimitKiB"), std::max<qint64>(0, kib)); }

QMap<QString, int> AppConfig::domainConnectionCaps() const {
    QMutexLocker lock(&m_mutex);
    QMap<QString, int> out{{QStringLiteral("archive.org"), 4}, {QStringLiteral("huggingface.co"), 8}};
    const QByteArray raw = m_settings.value(QStringLiteral("network/domainCapsJson")).toByteArray();
    if (!raw.isEmpty()) {
        out.clear();
        const QJsonObject obj = QJsonDocument::fromJson(raw).object();
        for (auto it = obj.begin(); it != obj.end(); ++it) { out.insert(it.key(), std::clamp(it.value().toInt(4), 1, 32)); }
    }
    return out;
}
void AppConfig::setDomainConnectionCaps(const QMap<QString, int>& caps) {
    QMutexLocker lock(&m_mutex);
    QJsonObject obj;
    for (auto it = caps.cbegin(); it != caps.cend(); ++it) { obj.insert(it.key().toLower().trimmed(), std::clamp(it.value(), 1, 32)); }
    m_settings.setValue(QStringLiteral("network/domainCapsJson"), QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
int AppConfig::maxConnectionsForDomain(const QString& host) const {
    const QMap<QString, int> caps = domainConnectionCaps();
    const QString h = host.toLower().trimmed();
    for (auto it = caps.cbegin(); it != caps.cend(); ++it) {
        if (h == it.key() || h.endsWith(QLatin1Char('.') + it.key())) { return it.value(); }
    }
    return defaultSegments();
}

int AppConfig::connectionTimeoutSec() const { QMutexLocker lock(&m_mutex); return std::clamp(m_settings.value(QStringLiteral("network/timeoutSec"), 30).toInt(), 5, 300); }
void AppConfig::setConnectionTimeoutSec(int sec) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/timeoutSec"), std::clamp(sec, 5, 300)); }
int AppConfig::retryDelaySec() const { QMutexLocker lock(&m_mutex); return std::clamp(m_settings.value(QStringLiteral("network/retryDelaySec"), 5).toInt(), 1, 120); }
void AppConfig::setRetryDelaySec(int sec) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/retryDelaySec"), std::clamp(sec, 1, 120)); }
int AppConfig::maxRetries() const { QMutexLocker lock(&m_mutex); return std::clamp(m_settings.value(QStringLiteral("network/maxRetries"), 10).toInt(), 0, 99); }
void AppConfig::setMaxRetries(int count) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/maxRetries"), std::clamp(count, 0, 99)); }
QString AppConfig::userAgent() const {
    QMutexLocker lock(&m_mutex);
    return m_settings.value(QStringLiteral("network/userAgent"),
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36")).toString();
}
void AppConfig::setUserAgent(const QString& ua) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/userAgent"), ua.trimmed()); }
quint16 AppConfig::ipcPort() const { QMutexLocker lock(&m_mutex); return static_cast<quint16>(std::clamp(m_settings.value(QStringLiteral("ipc/port"), 16800).toInt(), 1024, 65535)); }
void AppConfig::setIpcPort(quint16 port) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("ipc/port"), static_cast<int>(port)); }

ProxyMode AppConfig::proxyMode() const { QMutexLocker lock(&m_mutex); return static_cast<ProxyMode>(m_settings.value(QStringLiteral("proxy/mode"), 0).toInt()); }
void AppConfig::setProxyMode(ProxyMode mode) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("proxy/mode"), static_cast<int>(mode)); }
ProxyProtocol AppConfig::proxyProtocol() const { QMutexLocker lock(&m_mutex); return static_cast<ProxyProtocol>(m_settings.value(QStringLiteral("proxy/protocol"), 0).toInt()); }
void AppConfig::setProxyProtocol(ProxyProtocol proto) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("proxy/protocol"), static_cast<int>(proto)); }
QString AppConfig::proxyHost() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("network/proxyHost"), QStringLiteral("127.0.0.1")).toString(); }
void AppConfig::setProxyHost(const QString& host) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/proxyHost"), host.trimmed()); }
quint16 AppConfig::proxyPort() const { QMutexLocker lock(&m_mutex); return static_cast<quint16>(std::clamp(m_settings.value(QStringLiteral("network/proxyPort"), 8080).toInt(), 1, 65535)); }
void AppConfig::setProxyPort(quint16 port) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("network/proxyPort"), static_cast<int>(port)); }
QString AppConfig::proxyUsername() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("proxy/username")).toString(); }
void AppConfig::setProxyUsername(const QString& user) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("proxy/username"), user.trimmed()); }
QString AppConfig::proxyPassword() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("proxy/password")).toString(); }
void AppConfig::setProxyPassword(const QString& pass) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("proxy/password"), pass); }
QStringList AppConfig::proxyBypassList() const {
    QMutexLocker lock(&m_mutex);
    return m_settings.value(QStringLiteral("proxy/bypass"), QStringList{QStringLiteral("localhost"), QStringLiteral("127.0.0.1")}).toStringList();
}
void AppConfig::setProxyBypassList(const QStringList& bypass) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("proxy/bypass"), bypass); }

bool AppConfig::showStartConfirmDialog() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("dialogs/showConfirm"), true).toBool(); }
void AppConfig::setShowStartConfirmDialog(bool show) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("dialogs/showConfirm"), show); }
bool AppConfig::showLiveProgressDialog() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("dialogs/showLiveProgress"), true).toBool(); }
void AppConfig::setShowLiveProgressDialog(bool show) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("dialogs/showLiveProgress"), show); }
bool AppConfig::showCompletionDialog() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("dialogs/showComplete"), true).toBool(); }
void AppConfig::setShowCompletionDialog(bool show) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("dialogs/showComplete"), show); }
bool AppConfig::enableAntivirusScan() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("av/enabled"), false).toBool(); }
void AppConfig::setEnableAntivirusScan(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("av/enabled"), enabled); }
bool AppConfig::useWindowsDefender() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("av/useDefender"), true).toBool(); }
void AppConfig::setUseWindowsDefender(bool useDefender) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("av/useDefender"), useDefender); }
QString AppConfig::customAntivirusPath() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("av/customPath")).toString(); }
void AppConfig::setCustomAntivirusPath(const QString& path) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("av/customPath"), path.trimmed()); }
bool AppConfig::writeMarkOfTheWeb() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("av/markOfTheWeb"), true).toBool(); }
void AppConfig::setWriteMarkOfTheWeb(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("av/markOfTheWeb"), enabled); }
bool AppConfig::enableSoundAlerts() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("sound/enabled"), true).toBool(); }
void AppConfig::setEnableSoundAlerts(bool enabled) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("sound/enabled"), enabled); }
QString AppConfig::completionWavePath() const { QMutexLocker lock(&m_mutex); return m_settings.value(QStringLiteral("sound/wavePath"), QStringLiteral("C:/Windows/Media/notify.wav")).toString(); }
void AppConfig::setCompletionWavePath(const QString& path) { QMutexLocker lock(&m_mutex); m_settings.setValue(QStringLiteral("sound/wavePath"), path.trimmed()); }
void AppConfig::sync() { QMutexLocker lock(&m_mutex); m_settings.sync(); }

} // namespace idm::config
