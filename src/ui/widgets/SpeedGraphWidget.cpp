#include "ui/widgets/SpeedGraphWidget.h"
#include "core/Types.h"
#include "ui/theme/ThemeTokens.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace idm::ui::widgets {

using theme::ThemeTokens;

SpeedGraphWidget::SpeedGraphWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(110);
    m_samples.fill(0, kMaxSamples);
}

void SpeedGraphWidget::pushSpeedSample(qint64 bytesPerSec) {
    if (m_samples.size() >= kMaxSamples) {
        m_samples.removeFirst();
    }
    m_samples.append(std::max<qint64>(0, bytesPerSec));

    qint64 currentMax = 64 * 1024;
    for (qint64 val : m_samples) {
        currentMax = std::max(currentMax, val);
    }
    m_peakBytesPerSec = static_cast<qint64>(static_cast<double>(currentMax) * 1.18);
    update();
}

void SpeedGraphWidget::clearHistory() {
    m_samples.fill(0, kMaxSamples);
    m_peakBytesPerSec = 64 * 1024;
    update();
}

void SpeedGraphWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF full = rect();
    painter.fillRect(full, ThemeTokens::bgPanel());

    const QRectF plot = full.adjusted(14, 14, -14, -14);
    if (plot.width() <= 20 || plot.height() <= 20) {
        return;
    }

    painter.setPen(QPen(ThemeTokens::borderHairline(), 1.0));
    painter.setBrush(ThemeTokens::bgWindow());
    painter.drawRoundedRect(plot, 3.0, 3.0);

    QPen gridPen(ThemeTokens::borderHairline(), 1.0, Qt::DotLine);
    painter.setPen(gridPen);
    for (int i = 1; i < 4; ++i) {
        const double y = plot.top() + (plot.height() * i) / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }
    for (int i = 1; i < 6; ++i) {
        const double x = plot.left() + (plot.width() * i) / 6.0;
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }

    if (m_samples.size() >= 2 && m_peakBytesPerSec > 0) {
        QPainterPath linePath;
        QPainterPath areaPath;

        const int count = static_cast<int>(m_samples.size());
        const double stepX = plot.width() / static_cast<double>(count - 1);

        for (int i = 0; i < count; ++i) {
            const double ratio = std::clamp(
                static_cast<double>(m_samples.at(i)) / static_cast<double>(m_peakBytesPerSec),
                0.0,
                1.0
            );
            const double x = plot.left() + i * stepX;
            const double y = plot.bottom() - ratio * (plot.height() - 6.0) - 2.0;

            if (i == 0) {
                linePath.moveTo(x, y);
                areaPath.moveTo(x, plot.bottom());
                areaPath.lineTo(x, y);
            } else {
                linePath.lineTo(x, y);
                areaPath.lineTo(x, y);
            }
        }

        areaPath.lineTo(plot.right(), plot.bottom());
        areaPath.closeSubpath();

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x3B, 0x82, 0xF6, 42));
        painter.drawPath(areaPath);

        painter.setPen(QPen(ThemeTokens::statusActive(), 1.8));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(linePath);
    }

    painter.setFont(ThemeTokens::monoFont(false));
    painter.setPen(ThemeTokens::textTechnical());
    painter.drawText(
        plot.adjusted(8, 6, -8, -6),
        Qt::AlignLeft | Qt::AlignTop,
        QStringLiteral("60s WINDOW | PEAK: %1").arg(core::FormatUtils::formatSpeed(m_peakBytesPerSec))
    );

    const qint64 latest = m_samples.isEmpty() ? 0 : m_samples.last();
    painter.setFont(ThemeTokens::monoFont(true));
    painter.setPen(ThemeTokens::statusActive());
    painter.drawText(
        plot.adjusted(8, 6, -8, -6),
        Qt::AlignRight | Qt::AlignTop,
        QStringLiteral("LIVE: %1").arg(core::FormatUtils::formatSpeed(latest))
    );
}

} // namespace idm::ui::widgets
