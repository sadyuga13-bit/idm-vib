#include "system/WindowsSystemIntegration.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>
#endif

namespace idm::system {

void WindowsSystemIntegration::setAutoStartWithWindows(bool enabled) {
#ifdef _WIN32
    QSettings runReg(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    if (enabled) {
        const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runReg.setValue(QStringLiteral("IdmVibe"), QStringLiteral("\"%1\" --tray").arg(exePath));
    } else {
        runReg.remove(QStringLiteral("IdmVibe"));
    }
#else
    Q_UNUSED(enabled);
#endif
}

bool WindowsSystemIntegration::isAutoStartEnabled() {
#ifdef _WIN32
    QSettings runReg(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    return runReg.contains(QStringLiteral("IdmVibe"));
#else
    return false;
#endif
}

void WindowsSystemIntegration::registerCustomUrlProtocol() {
#ifdef _WIN32
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QSettings protoReg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\hdm"), QSettings::NativeFormat);
    protoReg.setValue(QStringLiteral("."), QStringLiteral("URL:IDM Vibe Download Protocol"));
    protoReg.setValue(QStringLiteral("URL Protocol"), QStringLiteral(""));
    protoReg.setValue(QStringLiteral("shell/open/command/."), QStringLiteral("\"%1\" \"%2\"").arg(exePath, QStringLiteral("%1")));
#endif
}

void WindowsSystemIntegration::setPreventSystemSleep(bool keepAwake) {
#ifdef _WIN32
    if (keepAwake) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
    } else {
        SetThreadExecutionState(ES_CONTINUOUS);
    }
#else
    Q_UNUSED(keepAwake);
#endif
}

void WindowsSystemIntegration::setTaskbarProgress(QWidget* window, qint64 completed, qint64 total, bool paused) {
#ifdef _WIN32
    if (window == nullptr || total <= 0) {
        return;
    }
    ITaskbarList3* taskbar = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar))) && taskbar != nullptr) {
        if (SUCCEEDED(taskbar->HrInit())) {
            const HWND hwnd = reinterpret_cast<HWND>(window->winId());
            taskbar->SetProgressState(hwnd, paused ? TBPF_PAUSED : TBPF_NORMAL);
            taskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(completed), static_cast<ULONGLONG>(total));
        }
        taskbar->Release();
    }
#else
    Q_UNUSED(window);
    Q_UNUSED(completed);
    Q_UNUSED(total);
    Q_UNUSED(paused);
#endif
}

void WindowsSystemIntegration::clearTaskbarProgress(QWidget* window) {
#ifdef _WIN32
    if (window == nullptr) {
        return;
    }
    ITaskbarList3* taskbar = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar))) && taskbar != nullptr) {
        if (SUCCEEDED(taskbar->HrInit())) {
            const HWND hwnd = reinterpret_cast<HWND>(window->winId());
            taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
        }
        taskbar->Release();
    }
#else
    Q_UNUSED(window);
#endif
}

} // namespace idm::system
