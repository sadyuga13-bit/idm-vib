#pragma once

#include <QWidget>
#include <QtGlobal>

namespace idm::system {

class WindowsSystemIntegration final {
public:
    WindowsSystemIntegration() = delete;

    static void setAutoStartWithWindows(bool enabled);
    [[nodiscard]] static bool isAutoStartEnabled();

    static void registerCustomUrlProtocol();
    static void setPreventSystemSleep(bool keepAwake);
    static void setTaskbarProgress(QWidget* window, qint64 completed, qint64 total, bool paused = false);
    static void clearTaskbarProgress(QWidget* window);
};

} // namespace idm::system
