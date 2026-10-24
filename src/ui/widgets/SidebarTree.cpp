#include "ui/widgets/SidebarTree.h"
#include "storage/CategoryRouter.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QHBoxLayout>
#include <QStorageInfo>
#include <QVBoxLayout>
#include <algorithm>

namespace idm::ui::widgets {

using storage::CategoryRouter;
using theme::IconId;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

namespace {
constexpr int kRoleItemType = Qt::UserRole + 10;
constexpr int kRoleItemValue = Qt::UserRole + 11;
constexpr int kTypeCategory = 1;
constexpr int kTypeQueue = 2;
} // namespace

SidebarTree::SidebarTree(QWidget* parent)
    : QWidget(parent) {
    setMinimumWidth(185);
    setMaximumWidth(250);
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setIndentation(14);
    m_tree->setAnimated(true);
    m_tree->setIconSize(QSize(16, 16));
    populateTree();
    mainLayout->addWidget(m_tree, 1);

    auto* queuePanel = new QWidget(this);
    auto* qLayout = new QHBoxLayout(queuePanel);
    qLayout->setContentsMargins(8, 8, 8, 8);
    qLayout->setSpacing(6);

    m_btnStartQueue = new QPushButton(QStringLiteral("Start"), queuePanel);
    m_btnStartQueue->setToolTip(QStringLiteral("Start Selected Queue"));
    m_btnStartQueue->setIcon(IconRegistry::icon(IconId::StartPlay, 14));
    m_btnStartQueue->setFixedHeight(30);
    m_btnStartQueue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_btnStopQueue = new QPushButton(QStringLiteral("Stop"), queuePanel);
    m_btnStopQueue->setToolTip(QStringLiteral("Stop Selected Queue"));
    m_btnStopQueue->setIcon(IconRegistry::icon(IconId::StopSquare, 14));
    m_btnStopQueue->setFixedHeight(30);
    m_btnStopQueue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    qLayout->addWidget(m_btnStartQueue, 1);
    qLayout->addWidget(m_btnStopQueue, 1);
    mainLayout->addWidget(queuePanel);

    auto* diskFooter = new QWidget(this);
    auto* diskLayout = new QVBoxLayout(diskFooter);
    diskLayout->setContentsMargins(10, 8, 10, 10);
    diskLayout->setSpacing(5);

    auto* titleRow = new QHBoxLayout();
    auto* driveIcon = new QLabel(diskFooter);
    driveIcon->setPixmap(IconRegistry::pixmap(IconId::HardDrive, 15, IconState::Normal));
    m_lblDrivePath = new QLabel(QStringLiteral("STORAGE VOLUME"), diskFooter);
    m_lblDrivePath->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600;"));
    m_lblDrivePath->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    titleRow->addWidget(driveIcon);
    titleRow->addWidget(m_lblDrivePath, 1);
    diskLayout->addLayout(titleRow);

    m_diskBar = new QProgressBar(diskFooter);
    m_diskBar->setRange(0, 100);
    m_diskBar->setTextVisible(false);
    m_diskBar->setFixedHeight(5);
    m_diskBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background-color: rgba(128,128,128,45); border: none; border-radius: 2px; }"
        "QProgressBar::chunk { background-color: #3B82F6; border-radius: 2px; }"
    ));
    diskLayout->addWidget(m_diskBar);

    m_lblDiskNumbers = new QLabel(diskFooter);
    m_lblDiskNumbers->setFont(ThemeTokens::monoFont(false));
    m_lblDiskNumbers->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    diskLayout->addWidget(m_lblDiskNumbers);

    mainLayout->addWidget(diskFooter);

    connect(m_tree, &QTreeWidget::currentItemChanged, this, &SidebarTree::onCurrentItemChanged);
    connect(m_btnStartQueue, &QPushButton::clicked, this, [this]() {
        emit startQueueRequested(m_activeQueueName);
    });
    connect(m_btnStopQueue, &QPushButton::clicked, this, [this]() {
        emit stopQueueRequested(m_activeQueueName);
    });

    refreshDiskTelemetry();
}

void SidebarTree::populateTree() {
    auto* catRoot = new QTreeWidgetItem(m_tree, {QStringLiteral("CATEGORIES")});
    catRoot->setFlags(Qt::ItemIsEnabled);
    catRoot->setFont(0, ThemeTokens::uiFont(true));

    const QList<core::FileCategory> cats = {
        core::FileCategory::All,
        core::FileCategory::Archive,
        core::FileCategory::Video,
        core::FileCategory::Audio,
        core::FileCategory::Document,
        core::FileCategory::Application,
        core::FileCategory::Image,
        core::FileCategory::Other
    };

    QTreeWidgetItem* firstSelectable = nullptr;
    for (core::FileCategory c : cats) {
        auto* item = new QTreeWidgetItem(catRoot, {CategoryRouter::categoryDisplayName(c)});
        item->setIcon(0, IconRegistry::icon(IconRegistry::iconForCategory(c), 16));
        item->setData(0, kRoleItemType, kTypeCategory);
        item->setData(0, kRoleItemValue, static_cast<int>(c));
        if (firstSelectable == nullptr) {
            firstSelectable = item;
        }
    }

    auto* queueRoot = new QTreeWidgetItem(m_tree, {QStringLiteral("SCHEDULER QUEUES")});
    queueRoot->setFlags(Qt::ItemIsEnabled);
    queueRoot->setFont(0, ThemeTokens::uiFont(true));

    auto* mainQ = new QTreeWidgetItem(queueRoot, {QStringLiteral("Main Queue")});
    mainQ->setIcon(0, IconRegistry::icon(IconId::QueueMain, 16));
    mainQ->setData(0, kRoleItemType, kTypeQueue);
    mainQ->setData(0, kRoleItemValue, QStringLiteral("Main Queue"));

    auto* nightQ = new QTreeWidgetItem(queueRoot, {QStringLiteral("Night Queue")});
    nightQ->setIcon(0, IconRegistry::icon(IconId::QueueNight, 16));
    nightQ->setData(0, kRoleItemType, kTypeQueue);
    nightQ->setData(0, kRoleItemValue, QStringLiteral("Night Queue"));

    m_tree->expandAll();
    if (firstSelectable != nullptr) {
        m_tree->setCurrentItem(firstSelectable);
    }
}

void SidebarTree::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*) {
    if (current == nullptr) {
        return;
    }
    const int itemType = current->data(0, kRoleItemType).toInt();
    if (itemType == kTypeCategory) {
        const auto cat = static_cast<core::FileCategory>(current->data(0, kRoleItemValue).toInt());
        emit categorySelected(cat);
    } else if (itemType == kTypeQueue) {
        m_activeQueueName = current->data(0, kRoleItemValue).toString();
        emit queueSelected(m_activeQueueName);
    }
}

void SidebarTree::refreshDiskTelemetry(const QString& path) {
    const QString target = path.isEmpty()
                               ? CategoryRouter::defaultDirectoryForCategory(core::FileCategory::All)
                               : path;
    const QStorageInfo storage(target);
    if (!storage.isValid() || storage.bytesTotal() <= 0) {
        m_lblDrivePath->setText(QStringLiteral("STORAGE VOLUME"));
        m_lblDiskNumbers->setText(QStringLiteral("Volume telemetry unavailable"));
        m_diskBar->setValue(0);
        return;
    }

    const qint64 total = storage.bytesTotal();
    const qint64 avail = storage.bytesAvailable();
    const qint64 used = std::max<qint64>(0, total - avail);
    const int usedPct = static_cast<int>((used * 100) / total);

    m_lblDrivePath->setText(QStringLiteral("VOLUME (%1)").arg(storage.rootPath()));
    m_diskBar->setValue(std::clamp(usedPct, 0, 100));
    m_lblDiskNumbers->setText(QStringLiteral("%1 free / %2")
                                  .arg(core::FormatUtils::formatBytes(avail),
                                       core::FormatUtils::formatBytes(total)));
}

} // namespace idm::ui::widgets
