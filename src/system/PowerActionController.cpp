#include "system/PowerActionController.h"

#include <QProcess>

namespace idm::system {

PowerActionController::PowerActionController(QObject* parent)
    : QObject(parent) {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &PowerActionController::onTimerTick);
}

void PowerActionController::triggerCountdown(int seconds) {
    if (m_action == PostDownloadPowerAction::None) {
        return;
    }
    m_remainingSeconds = seconds;
    m_timer.start();
    emit countdownTick(m_remainingSeconds, m_action);
}

void PowerActionController::cancelCountdown() {
    if (m_timer.isActive()) {
        m_timer.stop();
        m_remainingSeconds = 0;
        emit countdownCanceled();
    }
}

void PowerActionController::onTimerTick() {
    --m_remainingSeconds;
    if (m_remainingSeconds <= 0) {
        m_timer.stop();
        executePowerCommand(m_action);
        return;
    }
    emit countdownTick(m_remainingSeconds, m_action);
}

void PowerActionController::executePowerCommand(PostDownloadPowerAction action) {
#ifdef _WIN32
    switch (action) {
        case PostDownloadPowerAction::None:
            break;
        case PostDownloadPowerAction::Sleep:
            QProcess::startDetached(QStringLiteral("rundll32.exe"),
                {QStringLiteral("powrprof.dll,SetSuspendState"), QStringLiteral("0,1,0")});
            break;
        case PostDownloadPowerAction::Hibernate:
            QProcess::startDetached(QStringLiteral("shutdown.exe"), {QStringLiteral("/h")});
            break;
        case PostDownloadPowerAction::Shutdown:
            QProcess::startDetached(QStringLiteral("shutdown.exe"),
                {QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")});
            break;
    }
#else
    Q_UNUSED(action);
#endif
}

} // namespace idm::system
