#pragma once

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

namespace idm::system {

class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(QObject* parent = nullptr);
    ~TrayController() override;

    void showTrayIcon();
    void showNotification(const QString& title, const QString& message);
    void updateSpeedTooltip(const QString& speedText, int activeCount);
    void retranslateMenu();

signals:
    void showMainWindowRequested();
    void addUrlRequested();
    void resumeAllRequested();
    void stopAllRequested();
    void speedLimitPresetSelected(qint64 kibPerSec);
    void exitApplicationRequested();

private:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    QSystemTrayIcon m_trayIcon;
    QMenu m_menu;
    QAction* m_actShow{nullptr};
    QAction* m_actAdd{nullptr};
    QAction* m_actResumeAll{nullptr};
    QAction* m_actStopAll{nullptr};
    QAction* m_actAutoStart{nullptr};
    QAction* m_actExit{nullptr};
};

} // namespace idm::system
