#pragma once

#include "network/DownloadTask.h"
#include "ui/models/SegmentTableModel.h"
#include "ui/widgets/SegmentMapWidget.h"
#include "ui/widgets/SpeedGraphWidget.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTabWidget>
#include <QTableView>
#include <QWidget>

namespace idm::ui::widgets {

class InspectorDock final : public QWidget {
    Q_OBJECT

public:
    explicit InspectorDock(QWidget* parent = nullptr);
    ~InspectorDock() override = default;

    void inspectTask(network::DownloadTask* task);
    void pushGlobalSpeedSample(qint64 bytesPerSec);

private:
    void refreshFromCurrentTask();
    void appendLogLine(const QString& taskId, const QString& line);

    QPointer<network::DownloadTask> m_currentTask;
    QTabWidget* m_tabs{nullptr};

    // Tab 1: Segments & Byte Map
    SegmentMapWidget* m_segmentMap{nullptr};
    QTableView* m_segmentTable{nullptr};
    models::SegmentTableModel* m_segmentModel{nullptr};

    // Tab 2: Speed Telemetry & File Checksums
    SpeedGraphWidget* m_speedGraph{nullptr};
    QLabel* m_lblUrlValue{nullptr};
    QLabel* m_lblPathValue{nullptr};
    QLabel* m_lblHashValue{nullptr};

    // Tab 3: Engine Log
    QPlainTextEdit* m_logConsole{nullptr};
};

} // namespace idm::ui::widgets
