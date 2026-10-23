#include "ui/widgets/SegmentBarWidget.h"
#include "ui/theme/ThemeTokens.h"

#include <QPainter>
#include <algorithm>

namespace idm::ui::widgets {

using theme::ThemeTokens;

SegmentBarWidget::SegmentBarWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(22);
}

void SegmentBarWidget::setSegments(qint64 totalBytes, const QList<core::SegmentInfo>& segments) {
    m_totalBytes = totalBytes;
    m_segments = segments;
    update();
}

void SegmentBarWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    p.setPen(QPen(ThemeTokens::borderHairline(), 1.0));
    p.setBrush(ThemeTokens::bgWindow());
    p.drawRoundedRect(bounds, 3.0, 3.0);

    if (m_totalBytes <= 0 || m_segments.isEmpty()) {
        return;
    }

    const double totalD = static_cast<double>(m_totalBytes);
    for (const core::SegmentInfo& seg : m_segments) {
        if (seg.endOffset < seg.startOffset) {
            continue;
        }
        const double rStart = std::clamp(static_cast<double>(seg.startOffset) / totalD, 0.0, 1.0);
        const double rCur = std::clamp(static_cast<double>(seg.currentOffset) / totalD, 0.0, 1.0);
        const double rEnd = std::clamp(static_cast<double>(seg.endOffset + 1) / totalD, 0.0, 1.0);

        const double xStart = bounds.left() + rStart * bounds.width();
        const double xCur = bounds.left() + rCur * bounds.width();
        const double xEnd = bounds.left() + rEnd * bounds.width();

        if (xCur > xStart) {
            const QColor col = (seg.state == core::SegmentState::Completed)
                ? ThemeTokens::statusDone()
                : (seg.state == core::SegmentState::Error ? ThemeTokens::statusError() : ThemeTokens::statusActive());
            p.fillRect(QRectF(xStart, bounds.top() + 1.0, xCur - xStart, bounds.height() - 2.0), col);
        }

        p.setPen(QPen(ThemeTokens::borderHairline(), 1.0));
        p.drawLine(QPointF(xEnd, bounds.top()), QPointF(xEnd, bounds.bottom()));
    }
}

} // namespace idm::ui::widgets
