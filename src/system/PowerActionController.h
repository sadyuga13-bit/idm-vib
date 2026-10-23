#pragma once

#include <QObject>
#include <QTimer>

namespace idm::system {

enum class PostDownloadPowerAction : int {
    None = 0,
    Sleep = 1,
    Hibernate = 2,
    Shutdown = 3
};

class PowerActionController final : public QObject {
    Q_OBJECT

public:
    explicit PowerActionController(QObject* parent = nullptr);
    ~PowerActionController() override = default;

    void setScheduledAction(PostDownloadPowerAction action) noexcept { m_action = action; }
    [[nodiscard]] PostDownloadPowerAction scheduledAction() const noexcept { return m_action; }

    void triggerCountdown(int seconds = 60);
    void cancelCountdown();

signals:
    void countdownTick(int remainingSeconds, idm::system::PostDownloadPowerAction action);
    void countdownCanceled();

private:
    void onTimerTick();
    static void executePowerCommand(PostDownloadPowerAction action);

    PostDownloadPowerAction m_action{PostDownloadPowerAction::None};
    QTimer m_timer;
    int m_remainingSeconds{0};
};

} // namespace idm::system
