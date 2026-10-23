#include "ui/delegates/DownloadItemDelegate.h"
#include "ui/models/DownloadTableModel.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

namespace idm::ui::delegates {

using models::DownloadTableModel;
using theme::IconRegistry;
using theme::IconState;
using theme::ThemeTokens;

DownloadItemDelegate::DownloadItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QSize DownloadItemDelegate::sizeHint(
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setHeight(ThemeTokens::kRowHeightPx);
    return sz;
}

void DownloadItemDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    paintRowBackground(painter, option, index);

    const QRect contentRect = option.rect.adjusted(10, 0, -10, -1);
    const int col = index.column();

    if (col == DownloadTableModel::ColFileName) {
        paintFileNameCell(painter, contentRect, index);
    } else if (col == DownloadTableModel::ColProgress) {
        paintProgressCell(painter, contentRect, index);
    } else if (col == DownloadTableModel::ColStatus) {
        paintStatusCell(painter, contentRect, index);
    } else {
        const QFont font = (col == DownloadTableModel::ColQueue)
                               ? ThemeTokens::uiFont(false)
                               : ThemeTokens::monoFont(false);
        painter->setFont(font);

        if (col == DownloadTableModel::ColSpeed) {
            const auto st = static_cast<core::TaskState>(
                index.data(DownloadTableModel::RoleTaskState).toInt()
            );
            painter->setPen(st == core::TaskState::Active
                                ? ThemeTokens::statusActive()
                                : ThemeTokens::textTechnical());
        } else if (col == DownloadTableModel::ColSegments ||
                   col == DownloadTableModel::ColQueue) {
            painter->setPen(ThemeTokens::textSecondary());
        } else {
            painter->setPen(ThemeTokens::textPrimary());
        }

        const int align = index.data(Qt::TextAlignmentRole).toInt();
        const QString text = index.data(Qt::DisplayRole).toString();
        painter->drawText(contentRect, align, text);
    }

    painter->restore();
}

void DownloadItemDelegate::paintRowBackground(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) {
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const bool isHovered = option.state.testFlag(QStyle::State_MouseOver);

    QColor bg = (index.row() % 2 == 1)
                    ? ThemeTokens::bgRowAlt()
                    : ThemeTokens::bgWindow();
    if (isSelected) {
        bg = ThemeTokens::bgSelected();
    } else if (isHovered) {
        bg = ThemeTokens::bgHover();
    }

    painter->fillRect(option.rect, bg);

    if (isSelected && index.column() == 0) {
        const QRect accentBar(
            option.rect.left(),
            option.rect.top(),
            ThemeTokens::kSelectionBarWidth,
            option.rect.height()
        );
        painter->fillRect(accentBar, ThemeTokens::statusActive());
    }

    painter->setPen(ThemeTokens::borderHairline());
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
}

void DownloadItemDelegate::paintFileNameCell(
    QPainter* painter,
    const QRect& rect,
    const QModelIndex& index
) {
    const auto category = static_cast<core::FileCategory>(
        index.data(DownloadTableModel::RoleCategory).toInt()
    );
    const auto state = static_cast<core::TaskState>(
        index.data(DownloadTableModel::RoleTaskState).toInt()
    );

    IconState icoState = IconState::Normal;
    if (state == core::TaskState::Active)    icoState = IconState::Active;
    if (state == core::TaskState::Completed) icoState = IconState::Success;
    if (state == core::TaskState::Failed)    icoState = IconState::Danger;

    const QPixmap ico = IconRegistry::pixmap(
        IconRegistry::iconForCategory(category),
        16,
        icoState
    );

    const int iconY = rect.top() + (rect.height() - 16) / 2;
    painter->drawPixmap(rect.left(), iconY, 16, 16, ico);

    const QRect textRect(rect.left() + 24, rect.top(), rect.width() - 24, rect.height());
    painter->setFont(ThemeTokens::uiFont(false));
    painter->setPen(ThemeTokens::textPrimary());

    const QFontMetrics fm(painter->font());
    const QString fileName = index.data(DownloadTableModel::RoleFileName).toString();
    const QString elided = fm.elidedText(fileName, Qt::ElideMiddle, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
}

void DownloadItemDelegate::paintProgressCell(
    QPainter* painter,
    const QRect& rect,
    const QModelIndex& index
) {
    const double pct = std::clamp(
        index.data(DownloadTableModel::RoleProgressPercent).toDouble(),
        0.0,
        100.0
    );
    const auto state = static_cast<core::TaskState>(
        index.data(DownloadTableModel::RoleTaskState).toInt()
    );

    const QFont mono = ThemeTokens::monoFont(false);
    painter->setFont(mono);
    painter->setPen(ThemeTokens::textSecondary());

    const int labelWidth = 48;
    const QRect labelRect(rect.right() - labelWidth + 1, rect.top(), labelWidth, rect.height());
    const QString pctText = QStringLiteral("%1%").arg(pct, 0, 'f', 1);
    painter->drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, pctText);

    const int barWidth = std::max(16, rect.width() - labelWidth - 8);
    const int barY = rect.top() + (rect.height() - ThemeTokens::kProgressBarHeight) / 2;
    const QRectF trackRect(rect.left(), barY, barWidth, ThemeTokens::kProgressBarHeight);

    painter->setPen(Qt::NoPen);
    painter->setBrush(ThemeTokens::borderHairline());
    painter->drawRoundedRect(trackRect, 2.0, 2.0);

    if (pct > 0.1) {
        const double fillW = ( static_cast<double>(barWidth) * pct ) / 100.0;
        const QRectF fillRect(rect.left(), barY, std::max(2.0, fillW), ThemeTokens::kProgressBarHeight);
        painter->setBrush(semanticColorForState(state));
        painter->drawRoundedRect(fillRect, 2.0, 2.0);
    }
}

void DownloadItemDelegate::paintStatusCell(
    QPainter* painter,
    const QRect& rect,
    const QModelIndex& index
) {
    const auto state = static_cast<core::TaskState>(
        index.data(DownloadTableModel::RoleTaskState).toInt()
    );
    const QColor statusColor = semanticColorForState(state);

    const int dotSize = 6;
    const int dotY = rect.top() + (rect.height() - dotSize) / 2;
    painter->setPen(Qt::NoPen);
    painter->setBrush(statusColor);
    painter->drawEllipse(QRectF(rect.left(), dotY, dotSize, dotSize));

    const QRect textRect(rect.left() + 12, rect.top(), rect.width() - 12, rect.height());
    painter->setFont(ThemeTokens::uiFont(true));
    painter->setPen(statusColor);
    painter->drawText(
        textRect,
        Qt::AlignLeft | Qt::AlignVCenter,
        index.data(Qt::DisplayRole).toString()
    );
}

QColor DownloadItemDelegate::semanticColorForState(core::TaskState state) noexcept {
    switch (state) {
        case core::TaskState::Active:
        case core::TaskState::Probing:
        case core::TaskState::Verifying:
            return ThemeTokens::statusActive();
        case core::TaskState::Completed:
            return ThemeTokens::statusDone();
        case core::TaskState::Paused:
        case core::TaskState::Queued:
            return ThemeTokens::statusPaused();
        case core::TaskState::Failed:
        case core::TaskState::Canceled:
            return ThemeTokens::statusError();
    }
    return ThemeTokens::textSecondary();
}

} // namespace idm::ui::delegates
