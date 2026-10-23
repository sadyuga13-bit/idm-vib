#pragma once

#include "core/Types.h"
#include "system/AntivirusScanner.h"
#include <QDialog>
#include <QLabel>

namespace idm::ui::dialogs {

class DownloadCompleteDialog final : public QDialog {
    Q_OBJECT

public:
    explicit DownloadCompleteDialog(const core::TaskMetrics& metrics, QWidget* parent = nullptr);
    ~DownloadCompleteDialog() override = default;

private:
    void onOpenFile();
    void onOpenFolder();

    core::TaskMetrics m_metrics;
    system::AntivirusScanner m_avScanner;
    QLabel* m_lblAvStatus{nullptr};
};

} // namespace idm::ui::dialogs
