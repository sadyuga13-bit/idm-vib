#include "system/TrayController.h"
#include "config/AppConfig.h"
#include "i18n/I18nManager.h"
#include "system/WindowsSystemIntegration.h"
#include "ui/theme/IconRegistry.h"

namespace idm::system {

using i18n::trText;
using ui::theme::IconId;
using ui::theme::IconRegistry;

TrayController::TrayController(QObject* parent)
    : QObject(parent) {
    m_trayIcon.setIcon(IconRegistry::icon(IconId::CategoryAll, 20));
    m_trayIcon.setToolTip(QStringLiteral("IDM Vibe - Ready"));

    m_actShow = m_menu.addAction(IconRegistry::icon(IconId::DeviceLaptop, 14), QString());
    m_actAdd = m_menu.addAction(IconRegistry::icon(IconId::AddUrl, 14), QString());
    m_menu.addSeparator();
    m_actResumeAll = m_menu.addAction(IconRegistry::icon(IconId::StartPlay, 14), QString());
    m_actStopAll = m_menu.addAction(IconRegistry::icon(IconId::StopAll, 14), QString());
    m_menu.addSeparator();

    QMenu* speedMenu = m_menu.addMenu(IconRegistry::icon(IconId::SpeedGauge, 14), QStringLiteral("Speed Limiter"));
    const QList<qint64> presets{0, 512, 2048, 8192};
    for (qint64 kib : presets) {
        const QString lbl = (kib == 0) ? QStringLiteral("Unlimited") : QStringLiteral("%1 KB/s").arg(kib);
        QAction* act = speedMenu->addAction(lbl);
        connect(act, &QAction::triggered, this, [this, kib]() { emit speedLimitPresetSelected(kib); });
    }

    m_menu.addSeparator();
    m_actAutoStart = m_menu.addAction(QString());
    m_actAutoStart->setCheckable(true);
    m_actAutoStart->setChecked(WindowsSystemIntegration::isAutoStartEnabled());

    m_menu.addSeparator();
    m_actExit = m_menu.addAction(IconRegistry::icon(IconId::TrashDelete, 14), QString());

    m_trayIcon.setContextMenu(&m_menu);
    retranslateMenu();

    connect(m_actShow, &QAction::triggered, this, &TrayController::showMainWindowRequested);
    connect(m_actAdd, &QAction::triggered, this, &TrayController::addUrlRequested);
    connect(m_actResumeAll, &QAction::triggered, this, &TrayController::resumeAllRequested);
    connect(m_actStopAll, &QAction::triggered, this, &TrayController::stopAllRequested);
    connect(m_actAutoStart, &QAction::toggled, this, [](bool checked) {
        config::AppConfig::instance().setStartWithWindows(checked);
        WindowsSystemIntegration::setAutoStartWithWindows(checked);
    });
    connect(m_actExit, &QAction::triggered, this, &TrayController::exitApplicationRequested);
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &TrayController::onTrayActivated);
}

TrayController::~TrayController() {
    m_trayIcon.hide();
}

void TrayController::showTrayIcon() {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon.show();
    }
}

void TrayController::showNotification(const QString& title, const QString& message) {
    if (m_trayIcon.isVisible()) {
        m_trayIcon.showMessage(title, message, IconRegistry::icon(IconId::CategoryAll, 20), 3500);
    }
}

void TrayController::updateSpeedTooltip(const QString& speedText, int activeCount) {
    m_trayIcon.setToolTip(QStringLiteral("IDM Vibe | Active: %1 | Speed: %2").arg(activeCount).arg(speedText));
}

void TrayController::retranslateMenu() {
    m_actShow->setText(trText(QStringLiteral("tray.show"), QStringLiteral("Show Main Window")));
    m_actAdd->setText(trText(QStringLiteral("tray.add_url"), QStringLiteral("Add New Download...")));
    m_actResumeAll->setText(trText(QStringLiteral("tray.resume_all"), QStringLiteral("Start / Resume Main Queue")));
    m_actStopAll->setText(trText(QStringLiteral("tray.stop_all"), QStringLiteral("Stop All Active Downloads")));
    m_actAutoStart->setText(trText(QStringLiteral("tray.autostart"), QStringLiteral("Launch at Windows Startup")));
    m_actExit->setText(trText(QStringLiteral("tray.exit"), QStringLiteral("Exit Completely")));
}

void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit showMainWindowRequested();
    }
}

} // namespace idm::system
