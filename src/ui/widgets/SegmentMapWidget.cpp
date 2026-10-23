#include "ui/widgets/SegmentMapWidget.h"
#include "ui/theme/ThemeTokens.h"

#include <QPainter>
#include <algorithm>

namespace idm::ui::widgets {

using theme::ThemeTokens;

SegmentMapWidget::SegmentMapWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(64);
    setMaximumHeight(82);
}

void SegmentMapWidget::updateMap(
    qint64 totalBytes,
    qint64 downloadedBytes,
    const QList<core::SegmentInfo>& segments
) {
    m_totalBytes = totalBytes;
    m_downloadedBytes = downloadedBytes;
    m_segments = segments;
    update();
}

void SegmentMapWidget::clearMap() {
    m_totalBytes = 0;
    m_downloadedBytes = 0;
    m_segments.clear();
    update();
}

void SegmentMapWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF fullRect = rect();
    painter.fillRect(fullRect, ThemeTokens::bgPanel());

    const QRectF content = fullRect.adjusted(10, 6, -10, -6);
    if (content.width() <= 20 || content.height() <= 20) {
        return;
    }

    const QRectF ribbonRect(content.left(), content.top(), content.width(), 16);
    drawRibbonBar(painter, ribbonRect);

    const QRectF gridRect(content.left(), ribbonRect.bottom() + 8, content.width(), content.height() - 24);
    drawBlockGrid(painter, gridRect);
}

void SegmentMapWidget::drawRibbonBar(QPainter& painter, const QRectF& rect) const {
    painter.setPen(QPen(ThemeTokens::borderHairline(), 1.0));
    painter.setBrush(ThemeTokens::bgWindow());
    painter.drawRoundedRect(rect, 3.0, 3.0);

    if (m_totalBytes <= 0 || m_segments.isEmpty()) {
        return;
    }

    const double totalD = static_cast<double>(m_totalBytes);
    for (const core::SegmentInfo& seg : m_segments) {
        if (seg.endOffset < seg.startOffset) {
            continue;
        }
        const double startRatio = std::clamp(static_cast<double>(seg.startOffset) / totalD, 0.0, 1.0);
        const double curRatio = std::clamp(static_cast<double>(seg.currentOffset) / totalD, 0.0, 1.0);
        const double endRatio = std::clamp(static_cast<double>(seg.endOffset + 1) / totalD, 0.0, 1.0);

        const double xStart = rect.left() + startRatio * rect.width();
        const double xCur = rect.left() + curRatio * rect.width();
        const double xEnd = rect.left() + endRatio * rect.width();

        if (xCur > xStart) {
            const QColor fillColor = (seg.state == core::SegmentState::Completed)
                                         ? ThemeTokens::statusDone()
                                         : (seg.state == core::SegmentState::Error
                                                ? ThemeTokens::statusError()
                                                : ThemeTokens::statusActive());
            painter.fillRect(QRectF(xStart, rect.top() + 1, xCur - xStart, rect.height() - 2), fillColor);
        }

        painter.setPen(QPen(ThemeTokens::borderHairline(), 1.0));
        painter.drawLine(QPointF(xEnd, rect.top()), QPointF(xEnd, rect.bottom()));
    }
}

void SegmentMapWidget::drawBlockGrid(QPainter& painter, const QRectF& rect) const {
    constexpr int kTotalBlocks = 96;
    constexpr double kGap = 2.0;

    const double blockW = std::max(3.0, (rect.width() - (kTotalBlocks - 1) * kGap) / kTotalBlocks);
    const double blockH = std::clamp(rect.height(), 8.0, 18.0);
    const double yTop = rect.top() + (rect.height() - blockH) / 2.0;

    for (int i = 0; i < kTotalBlocks; ++i) {
        const double x = rect.left() + i * (blockW + kGap);
        const QRectF cell(x, yTop, blockW, blockH);

        QColor cellColor = ThemeTokens::borderHairline();
        if (m_totalBytes > 0 && !m_segments.isEmpty()) {
            const qint64 blockByteStart = (m_totalBytes * i) / kTotalBlocks;
            const qint64 blockByteEnd = (m_totalBytes * (i + 1)) / kTotalBlocks - 1;

            for (const core::SegmentInfo& seg : m_segments) {
                if (blockByteStart >= seg.startOffset && blockByteStart <= seg.endOffset) {
                    if (seg.currentOffset > blockByteEnd) {
                        cellColor = ThemeTokens::statusDone();
                    } else if (seg.currentOffset > blockByteStart) {
                        cellColor = ThemeTokens::statusActive();
                    }
                    break;
                }
            }
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(cellColor);
        painter.drawRoundedRect(cell, 1.5, 1.5);
    }
}

} // namespace idm::ui::widgets
