#include "ui/models/SegmentTableModel.h"
#include "ui/theme/ThemeTokens.h"

namespace idm::ui::models {

SegmentTableModel::SegmentTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

void SegmentTableModel::setSegments(const QList<core::SegmentInfo>& segments) {
    if (m_segments.size() != segments.size()) {
        beginResetModel();
        m_segments = segments;
        endResetModel();
        return;
    }

    m_segments = segments;
    if (!m_segments.isEmpty()) {
        emit dataChanged(
            index(0, 0),
            index(static_cast<int>(m_segments.size()) - 1, ColumnCount - 1)
        );
    }
}

void SegmentTableModel::clear() {
    beginResetModel();
    m_segments.clear();
    endResetModel();
}

int SegmentTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_segments.size());
}

int SegmentTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant SegmentTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_segments.size()) {
        return {};
    }

    const core::SegmentInfo& seg = m_segments.at(index.row());

    if (role == Qt::FontRole) {
        return (index.column() == ColState)
                   ? theme::ThemeTokens::uiFont(false)
                   : theme::ThemeTokens::monoFont(false);
    }

    if (role == Qt::ForegroundRole) {
        if (index.column() == ColState) {
            switch (seg.state) {
                case core::SegmentState::Downloading:
                case core::SegmentState::Connecting:
                    return theme::ThemeTokens::statusActive();
                case core::SegmentState::Completed:
                    return theme::ThemeTokens::statusDone();
                case core::SegmentState::Paused:
                    return theme::ThemeTokens::statusPaused();
                case core::SegmentState::Error:
                    return theme::ThemeTokens::statusError();
                default:
                    return theme::ThemeTokens::textSecondary();
            }
        }
        return theme::ThemeTokens::textPrimary();
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColId || index.column() == ColProgress) {
            return static_cast<int>(Qt::AlignCenter);
        }
        if (index.column() == ColDownloaded ||
            index.column() == ColRemaining ||
            index.column() == ColSpeed) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColId:
                return QStringLiteral("#%1").arg(seg.id, 2, 10, QLatin1Char('0'));
            case ColRange:
                return (seg.endOffset >= 0)
                           ? QStringLiteral("%1 - %2").arg(seg.startOffset).arg(seg.endOffset)
                           : QStringLiteral("%1 - EOF").arg(seg.startOffset);
            case ColDownloaded:
                return core::FormatUtils::formatBytes(seg.downloadedBytes());
            case ColRemaining:
                return (seg.endOffset >= 0)
                           ? core::FormatUtils::formatBytes(seg.remainingBytes())
                           : QStringLiteral("--");
            case ColProgress:
                return QStringLiteral("%1%").arg(seg.progressRatio() * 100.0, 0, 'f', 1);
            case ColSpeed:
                return seg.state == core::SegmentState::Downloading
                           ? core::FormatUtils::formatSpeed(seg.speedBytesPerSec)
                           : QStringLiteral("0.0 KB/s");
            case ColState:
                return seg.statusText.isEmpty()
                           ? core::FormatUtils::segmentStateToString(seg.state)
                           : seg.statusText;
            default:
                return {};
        }
    }

    return {};
}

QVariant SegmentTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
        case ColId:         return QStringLiteral("Thread");
        case ColRange:      return QStringLiteral("Byte Offset Range");
        case ColDownloaded: return QStringLiteral("Received");
        case ColRemaining:  return QStringLiteral("Remaining");
        case ColProgress:   return QStringLiteral("Progress");
        case ColSpeed:      return QStringLiteral("Speed");
        case ColState:      return QStringLiteral("Status");
        default:            return {};
    }
}

} // namespace idm::ui::models
