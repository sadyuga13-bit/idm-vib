#pragma once

#include "network/DownloadTask.h"
#include "ui/models/SegmentTableModel.h"
#include "ui/widgets/SegmentBarWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>

namespace idm::ui::dialogs {

class DownloadProgressDialog final : public QDialog {
    Q_OBJECT

public:
    explicit DownloadProgressDialog(network::DownloadTask* task, QWidget* parent = nullptr);
    ~DownloadProgressDialog() override;

private:
    QWidget* buildStatusTab();
    QWidget* buildSpeedLimiterTab();
    QWidget* buildCompletionOptionsTab();

    void refreshTelemetry();
    void onToggleAlwaysOnTop(bool pinned);
    void onPauseResumeClicked();

    QPointer<network::DownloadTask> m_task;

    QLabel* m_lblUrl{nullptr};
    QLabel* m_lblStatus{nullptr};
    QLabel* m_lblFileSize{nullptr};
    QLabel* m_lblDownloaded{nullptr};
    QLabel* m_lblTransferRate{nullptr};
    QLabel* m_lblTimeLeft{nullptr};
    QLabel* m_lblResumeCapability{nullptr};

    QProgressBar* m_mainProgress{nullptr};
    widgets::SegmentBarWidget* m_segmentBar{nullptr};
    QComboBox* m_liveConnectionsCombo{nullptr};
    models::SegmentTableModel* m_segModel{nullptr};
    QTableView* m_segTable{nullptr};

    QCheckBox* m_chkTaskSpeedLimit{nullptr};
    QSpinBox* m_spinTaskSpeedKiB{nullptr};

    QCheckBox* m_chkCloseOnComplete{nullptr};
    QCheckBox* m_chkShowCompleteDlg{nullptr};
    QComboBox* m_powerActionCombo{nullptr};

    QPushButton* m_btnPinTop{nullptr};
    QPushButton* m_btnPauseResume{nullptr};
    QPushButton* m_btnCancelTask{nullptr};
};

} // namespace idm::ui::dialogs
