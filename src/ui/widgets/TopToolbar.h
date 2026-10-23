#pragma once

#include "security/SecurityTypes.h"
#include "ui/security/LicenseStatusBadge.h"
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>
#include <QWidget>

namespace idm::ui::widgets {

class TopToolbar final : public QWidget {
    Q_OBJECT

public:
    explicit TopToolbar(QWidget* parent = nullptr);
    ~TopToolbar() override = default;

    void setTaskActionsEnabled(bool canStart, bool canPause, bool canDelete);
    void updateAggregateSpeed(qint64 bytesPerSec, int activeCount);
    void updateLicenseBadge(idm::security::LicenseTier tier, const QString& labelText);
    void setSpeedLimitValue(qint64 kibPerSec);
    void refreshThemeIcons();

signals:
    void addUrlClicked();
    void batchDownloadClicked();
    void resumeSelectedClicked();
    void pauseSelectedClicked();
    void stopAllClicked();
    void deleteSelectedClicked();
    void openFolderClicked();
    void verifyHashClicked();
    void settingsClicked();
    void themeToggleClicked();
    void licenseBadgeClicked();
    void globalSpeedLimitChanged(qint64 kibPerSec);
    void searchQueryChanged(const QString& text);

private:
    static QToolButton* createActionBtn(
        const QString& text,
        const QIcon& icon,
        QWidget* parent
    );
    static QWidget* createVerticalSeparator(QWidget* parent);

    QToolButton* m_btnAddUrl{nullptr};
    QToolButton* m_btnBatch{nullptr};
    QToolButton* m_btnResume{nullptr};
    QToolButton* m_btnPause{nullptr};
    QToolButton* m_btnStopAll{nullptr};
    QToolButton* m_btnDelete{nullptr};
    QToolButton* m_btnOpenFolder{nullptr};
    QToolButton* m_btnVerifyHash{nullptr};
    QToolButton* m_btnThemeToggle{nullptr};
    QToolButton* m_btnSettings{nullptr};

    QSpinBox* m_spinSpeedCap{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QLabel* m_lblLiveSpeed{nullptr};
    security::LicenseStatusBadge* m_licenseBadge{nullptr};
};

} // namespace idm::ui::widgets
