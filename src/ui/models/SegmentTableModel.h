#pragma once

#include "core/Types.h"
#include <QAbstractTableModel>
#include <QList>

namespace idm::ui::models {

class SegmentTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column : int {
        ColId = 0,
        ColRange,
        ColDownloaded,
        ColRemaining,
        ColProgress,
        ColSpeed,
        ColState,
        ColumnCount
    };

    explicit SegmentTableModel(QObject* parent = nullptr);
    ~SegmentTableModel() override = default;

    void setSegments(const QList<core::SegmentInfo>& segments);
    void clear();

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
    ) const override;

private:
    QList<core::SegmentInfo> m_segments;
};

} // namespace idm::ui::models
