#pragma once

#include "core/Types.h"
#include <QColor>
#include <QStyledItemDelegate>

namespace idm::ui::delegates {

class DownloadItemDelegate final : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit DownloadItemDelegate(QObject* parent = nullptr);
    ~DownloadItemDelegate() override = default;

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    [[nodiscard]] QSize sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

private:
    static void paintRowBackground(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    );
    static void paintFileNameCell(
        QPainter* painter,
        const QRect& rect,
        const QModelIndex& index
    );
    static void paintProgressCell(
        QPainter* painter,
        const QRect& rect,
        const QModelIndex& index
    );
    static void paintStatusCell(
        QPainter* painter,
        const QRect& rect,
        const QModelIndex& index
    );
    [[nodiscard]] static QColor semanticColorForState(core::TaskState state) noexcept;
};

} // namespace idm::ui::delegates
