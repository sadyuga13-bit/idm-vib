#include "ui/widgets/InspectorDock.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace idm::ui::widgets {

using theme::IconId;
using theme::IconRegistry;
using theme::ThemeTokens;

InspectorDock::InspectorDock(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(140);
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_tabs = new QTabWidget(this);
    m_tabs->setIconSize(QSize(15, 15));
    m_tabs->setMinimumWidth(0);

    auto* segPage = new QWidget(m_tabs);
    auto* segLayout = new QVBoxLayout(segPage);
    segLayout->setContentsMargins(0, 0, 0, 0);
    segLayout->setSpacing(0);

    m_segmentMap = new SegmentMapWidget(segPage);
    segLayout->addWidget(m_segmentMap);

    m_segmentModel = new models::SegmentTableModel(this);
    m_segmentTable = new QTableView(segPage);
    m_segmentTable->setModel(m_segmentModel);
    m_segmentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_segmentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_segmentTable->setAlternatingRowColors(true);
    m_segmentTable->verticalHeader()->setVisible(false);
    m_segmentTable->verticalHeader()->setDefaultSectionSize(24);

    QHeaderView* sh = m_segmentTable->horizontalHeader();
    sh->setFixedHeight(26);
    sh->setStretchLastSection(true);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColId, 56);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColRange, 180);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColDownloaded, 95);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColRemaining, 95);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColProgress, 75);
    m_segmentTable->setColumnWidth(models::SegmentTableModel::ColSpeed, 95);
    sh->setSectionResizeMode(models::SegmentTableModel::ColRange, QHeaderView::Stretch);
    segLayout->addWidget(m_segmentTable, 1);

    m_tabs->addTab(segPage, IconRegistry::icon(IconId::CpuThreads, 15), QStringLiteral("Segments & Byte Map"));

    auto* speedPage = new QWidget(m_tabs);
    auto* speedLayout = new QHBoxLayout(speedPage);
    speedLayout->setContentsMargins(10, 8, 10, 8);
    speedLayout->setSpacing(12);

    m_speedGraph = new SpeedGraphWidget(speedPage);
    m_speedGraph->setMinimumWidth(160);
    speedLayout->addWidget(m_speedGraph, 3);

    auto* metaBox = new QWidget(speedPage);
    metaBox->setMinimumWidth(0);
    metaBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* form = new QFormLayout(metaBox);
    form->setContentsMargins(6, 4, 6, 4);
    form->setSpacing(8);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeBoundedLabel = [metaBox](const QString& initText) {
        auto* lbl = new QLabel(initText, metaBox);
        lbl->setFont(ThemeTokens::monoFont(false));
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lbl->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        lbl->setMinimumWidth(0);
        return lbl;
    };

    m_lblUrlValue = makeBoundedLabel(QStringLiteral("--"));
    m_lblPathValue = makeBoundedLabel(QStringLiteral("--"));
    m_lblHashValue = makeBoundedLabel(QStringLiteral("Not verified yet"));

    form->addRow(QStringLiteral("Endpoint URL:"), m_lblUrlValue);
    form->addRow(QStringLiteral("Disk Target:"), m_lblPathValue);
    form->addRow(QStringLiteral("SHA-256 / MD5:"), m_lblHashValue);
    speedLayout->addWidget(metaBox, 2);

    m_tabs->addTab(speedPage, IconRegistry::icon(IconId::SpeedGauge, 15), QStringLiteral("Throughput Telemetry"));

    m_logConsole = new QPlainTextEdit(m_tabs);
    m_logConsole->setReadOnly(true);
    m_logConsole->setFont(ThemeTokens::monoFont(false));
    m_tabs->addTab(m_logConsole, IconRegistry::icon(IconId::TerminalLog, 15), QStringLiteral("Protocol & Engine Log"));

    mainLayout->addWidget(m_tabs);
}

void InspectorDock::inspectTask(network::DownloadTask* task) {
    if (m_currentTask == task) {
        refreshFromCurrentTask();
        return;
    }
    if (m_currentTask != nullptr) {
        m_currentTask->disconnect(this);
    }
    m_currentTask = task;
    m_logConsole->clear();

    if (m_currentTask == nullptr) {
        m_segmentMap->clearMap();
        m_segmentModel->clear();
        m_lblUrlValue->setText(QStringLiteral("--"));
        m_lblPathValue->setText(QStringLiteral("--"));
        m_lblHashValue->setText(QStringLiteral("--"));
        return;
    }

    for (const QString& line : m_currentTask->logs()) {
        m_logConsole->appendPlainText(line);
    }
    connect(m_currentTask, &network::DownloadTask::metricsUpdated, this, [this](const QString&) {
        refreshFromCurrentTask();
    });
    connect(m_currentTask, &network::DownloadTask::logAdded, this, &InspectorDock::appendLogLine);
    refreshFromCurrentTask();
}

void InspectorDock::pushGlobalSpeedSample(qint64 bytesPerSec) {
    if (m_currentTask != nullptr && m_currentTask->state() == core::TaskState::Active) {
        m_speedGraph->pushSpeedSample(m_currentTask->metrics().speedBytesPerSec);
    } else {
        m_speedGraph->pushSpeedSample(bytesPerSec);
    }
}

void InspectorDock::refreshFromCurrentTask() {
    if (m_currentTask == nullptr) return;
    const core::TaskMetrics& m = m_currentTask->metrics();
    m_segmentMap->updateMap(m.totalBytes, m.downloadedBytes, m.segments);
    m_segmentModel->setSegments(m.segments);

    m_lblUrlValue->setText(m.url);
    m_lblUrlValue->setToolTip(m.url);
    m_lblPathValue->setText(m.fullSavePath());
    m_lblPathValue->setToolTip(m.fullSavePath());
    if (!m.computedSha256.isEmpty()) {
        const QString hTxt = QStringLiteral("SHA256: %1 | MD5: %2").arg(m.computedSha256, m.computedMd5);
        m_lblHashValue->setText(hTxt);
        m_lblHashValue->setToolTip(hTxt);
        m_lblHashValue->setStyleSheet(QStringLiteral("color: #10B981;"));
    } else {
        m_lblHashValue->setText(QStringLiteral("Awaiting completion / verification"));
        m_lblHashValue->setStyleSheet(QStringLiteral("color: #64748B;"));
    }
}

void InspectorDock::appendLogLine(const QString& taskId, const QString& line) {
    if (m_currentTask != nullptr && m_currentTask->taskId() == taskId) {
        m_logConsole->appendPlainText(line);
    }
}

} // namespace idm::ui::widgets
