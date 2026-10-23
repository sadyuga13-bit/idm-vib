#pragma once

#include <QSettings>
#include <QString>
#include <QtGlobal>

namespace idm::core {

class AppSettings final {
public:
    static AppSettings& instance();

    [[nodiscard]] bool isDayMode() const;
    void setDayMode(bool enabled);

    [[nodiscard]] QString defaultSaveDirectory() const;
    void setDefaultSaveDirectory(const QString& dir);

    [[nodiscard]] int defaultSegments() const;
    void setDefaultSegments(int segments);

    [[nodiscard]] int maxConcurrentTasks() const;
    void setMaxConcurrentTasks(int count);

    [[nodiscard]] qint64 globalSpeedLimitKiB() const;
    void setGlobalSpeedLimitKiB(qint64 kibPerSec);

    [[nodiscard]] bool clipboardMonitoringEnabled() const;
    void setClipboardMonitoringEnabled(bool enabled);

    [[nodiscard]] quint16 ipcPort() const;
    void setIpcPort(quint16 port);

    [[nodiscard]] QString preferredYoutubeQuality() const;
    void setPreferredYoutubeQuality(const QString& qualityTag);

    [[nodiscard]] QString customUserAgent() const;
    void setCustomUserAgent(const QString& ua);

    [[nodiscard]] bool proxyEnabled() const;
    void setProxyEnabled(bool enabled);

    [[nodiscard]] QString proxyHost() const;
    void setProxyHost(const QString& host);

    [[nodiscard]] quint16 proxyPort() const;
    void setProxyPort(quint16 port);

    void sync();

private:
    AppSettings();
    ~AppSettings() = default;

    mutable QSettings m_settings;
};

} // namespace idm::core
