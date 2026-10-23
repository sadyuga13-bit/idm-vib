#include "ui/delegates/DownloadItemDelegate.h"
#include "ui/models/DownloadTableModel.h"
#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QFontMetrics>
#include <QLinearGradient>
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

    const QRect contentRect = option.rect.adjusted(12, 0, -12, -1);
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
                               : ThemeTokens::monoFont(col == DownloadTableModel::ColSpeed);
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
            option.rect.top() + 4,
            ThemeTokens::kSelectionBarWidth,
            option.rect.height() - 8
        );
        painter->setPen(Qt::NoPen);
        painter->setBrush(ThemeTokens::statusActive());
        painter->drawRoundedRect(accentBar, 1.5, 1.5);
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

    const QColor tint = semanticColorForState(state);
    QColor badgeBg = tint;
    badgeBg.setAlpha(ThemeTokens::isDayMode() ? 24 : 32);

    const int badgeSize = 26;
    const int badgeY = rect.top() + (rect.height() - badgeSize) / 2;
    const QRectF badgeRect(rect.left(), badgeY, badgeSize, badgeSize);
    painter->setPen(Qt::NoPen);
    painter->setBrush(badgeBg);
    painter->drawRoundedRect(badgeRect, 6.0, 6.0);

    const QPixmap ico = IconRegistry::pixmap(
        IconRegistry::iconForCategory(category),
        16,
        icoState
    );
    painter->drawPixmap(rect.left() + 5, badgeY + 5, 16, 16, ico);

    const QRect textRect(rect.left() + 34, rect.top(), rect.width() - 34, rect.height());
    painter->setFont(ThemeTokens::uiFont(true));
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

    painter->setFont(ThemeTokens::monoFont(true));
    painter->setPen(ThemeTokens::textSecondary());

    const int labelWidth = 52;
    const QRect labelRect(rect.right() - labelWidth + 1, rect.top(), labelWidth, rect.height());
    const QString pctText = QStringLiteral("%1%").arg(pct, 0, 'f', 1);
    painter->drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, pctText);

    const int barWidth = std::max(24, rect.width() - labelWidth - 10);
    const int barY = rect.top() + (rect.height() - ThemeTokens::kProgressBarHeight) / 2;
    const QRectF trackRect(rect.left(), barY, barWidth, ThemeTokens::kProgressBarHeight);

    painter->setPen(Qt::NoPen);
    painter->setBrush(ThemeTokens::borderHairline());
    painter->drawRoundedRect(trackRect, 3.5, 3.5);

    if (pct > 0.1) {
        const double fillW = (static_cast<double>(barWidth) * pct) / 100.0;
        const QRectF fillRect(rect.left(), barY, std::max(4.0, fillW), ThemeTokens::kProgressBarHeight);
        const QColor baseCol = semanticColorForState(state);
        QLinearGradient grad(fillRect.topLeft(), fillRect.topRight());
        grad.setColorAt(0.0, baseCol);
        grad.setColorAt(1.0, baseCol.lighter(118));
        painter->setBrush(grad);
        painter->drawRoundedRect(fillRect, 3.5, 3.5);
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
    const QString label = index.data(Qt::DisplayRole).toString();

    const QFont f = ThemeTokens::uiFont(true);
    painter->setFont(f);
    const QFontMetrics fm(f);
    const int pillW = std::min(rect.width(), fm.horizontalAdvance(label) + 26);
    const int pillH = 24;
    const int pillY = rect.top() + (rect.height() - pillH) / 2;
    const QRectF pillRect(rect.left(), pillY, pillW, pillH);

    QColor pillBg = statusColor;
    pillBg.setAlpha(ThemeTokens::isDayMode() ? 24 : 34);
    QColor pillBorder = statusColor;
    pillBorder.setAlpha(95);

    painter->setPen(QPen(pillBorder, 1.0));
    painter->setBrush(pillBg);
    painter->drawRoundedRect(pillRect, 12.0, 12.0);

    const int dotSize = 6;
    const int dotY = pillY + (pillH - dotSize) / 2;
    painter->setPen(Qt::NoPen);
    painter->setBrush(statusColor);
    painter->drawEllipse(QRectF(rect.left() + 8, dotY, dotSize, dotSize));

    const QRect textRect(rect.left() + 19, pillY, pillW - 24, pillH);
    painter->setPen(statusColor);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
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
