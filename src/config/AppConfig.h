#pragma once

#include "core/Types.h"
#include <QMap>
#include <QMutex>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace idm::config {

enum class DuplicatePolicy : int {
    AutoRenameNumbered = 0,
    ResumeIfMatch = 1,
    OverwriteExisting = 2,
    AskUser = 3
};

enum class ProxyMode : int {
    DirectConnection = 0,
    SystemWinHttp = 1,
    ManualProxy = 2
};

enum class ProxyProtocol : int {
    Http = 0,
    Https = 1,
    Socks5 = 2
};

class AppConfig final {
public:
    static AppConfig& instance();

    // 1. General, Theme & Language
    [[nodiscard]] QString languageCode() const;
    void setLanguageCode(const QString& code);

    [[nodiscard]] bool isDayMode() const;
    void setDayMode(bool enabled);

    [[nodiscard]] bool startWithWindows() const;
    void setStartWithWindows(bool enabled);

    [[nodiscard]] bool closeToTray() const;
    void setCloseToTray(bool enabled);

    [[nodiscard]] bool preventSystemSleep() const;
    void setPreventSystemSleep(bool enabled);

    [[nodiscard]] bool monitorClipboard() const;
    void setMonitorClipboard(bool enabled);

    [[nodiscard]] bool suppressWithAltKey() const;
    void setSuppressWithAltKey(bool enabled);

    [[nodiscard]] bool forceWithInsertKey() const;
    void setForceWithInsertKey(bool enabled);

    // 2. File Types, Exclusions & Duplicates
    [[nodiscard]] QStringList capturedExtensions() const;
    void setCapturedExtensions(const QStringList& exts);

    [[nodiscard]] QStringList excludedDomains() const;
    void setExcludedDomains(const QStringList& domains);

    [[nodiscard]] DuplicatePolicy duplicatePolicy() const;
    void setDuplicatePolicy(DuplicatePolicy policy);

    // 3. Save Paths & Temp .part Directory
    [[nodiscard]] QString categoryDirectory(core::FileCategory cat) const;
    void setCategoryDirectory(core::FileCategory cat, const QString& path);

    [[nodiscard]] QString tempPartDirectory() const;
    void setTempPartDirectory(const QString& path);

    [[nodiscard]] bool preallocateDiskSpace() const;
    void setPreallocateDiskSpace(bool enabled);

    // 4. Connections, Domain Caps & Network
    [[nodiscard]] int defaultSegments() const;
    void setDefaultSegments(int count);

    [[nodiscard]] int maxConcurrentDownloads() const;
    void setMaxConcurrentDownloads(int count);

    [[nodiscard]] qint64 globalSpeedLimitKiB() const;
    void setGlobalSpeedLimitKiB(qint64 kib);

    [[nodiscard]] QMap<QString, int> domainConnectionCaps() const;
    void setDomainConnectionCaps(const QMap<QString, int>& caps);
    [[nodiscard]] int maxConnectionsForDomain(const QString& host) const;

    [[nodiscard]] int connectionTimeoutSec() const;
    void setConnectionTimeoutSec(int sec);

    [[nodiscard]] int retryDelaySec() const;
    void setRetryDelaySec(int sec);

    [[nodiscard]] int maxRetries() const;
    void setMaxRetries(int count);

    [[nodiscard]] QString userAgent() const;
    void setUserAgent(const QString& ua);

    [[nodiscard]] quint16 ipcPort() const;
    void setIpcPort(quint16 port);

    // 5. Proxy Configuration
    [[nodiscard]] ProxyMode proxyMode() const;
    void setProxyMode(ProxyMode mode);

    [[nodiscard]] ProxyProtocol proxyProtocol() const;
    void setProxyProtocol(ProxyProtocol proto);

    [[nodiscard]] QString proxyHost() const;
    void setProxyHost(const QString& host);

    [[nodiscard]] quint16 proxyPort() const;
    void setProxyPort(quint16 port);

    [[nodiscard]] QString proxyUsername() const;
    void setProxyUsername(const QString& user);

    [[nodiscard]] QString proxyPassword() const;
    void setProxyPassword(const QString& pass);

    [[nodiscard]] QStringList proxyBypassList() const;
    void setProxyBypassList(const QStringList& bypass);

    // 6. Popups, Antivirus & Sounds
    [[nodiscard]] bool showStartConfirmDialog() const;
    void setShowStartConfirmDialog(bool show);

    [[nodiscard]] bool showLiveProgressDialog() const;
    void setShowLiveProgressDialog(bool show);

    [[nodiscard]] bool showCompletionDialog() const;
    void setShowCompletionDialog(bool show);

    [[nodiscard]] bool enableAntivirusScan() const;
    void setEnableAntivirusScan(bool enabled);

    [[nodiscard]] bool useWindowsDefender() const;
    void setUseWindowsDefender(bool useDefender);

    [[nodiscard]] QString customAntivirusPath() const;
    void setCustomAntivirusPath(const QString& path);

    [[nodiscard]] bool writeMarkOfTheWeb() const;
    void setWriteMarkOfTheWeb(bool enabled);

    [[nodiscard]] bool enableSoundAlerts() const;
    void setEnableSoundAlerts(bool enabled);

    [[nodiscard]] QString completionWavePath() const;
    void setCompletionWavePath(const QString& path);

    void sync();

private:
    AppConfig();
    ~AppConfig() = default;

    mutable QMutex m_mutex;
    mutable QSettings m_settings;
};

} // namespace idm::config
