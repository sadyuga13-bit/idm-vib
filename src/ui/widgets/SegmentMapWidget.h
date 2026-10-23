#pragma once

#include "core/Types.h"
#include <QList>
#include <QWidget>

namespace idm::ui::widgets {

class SegmentMapWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SegmentMapWidget(QWidget* parent = nullptr);
    ~SegmentMapWidget() override = default;

    void updateMap(
        qint64 totalBytes,
        qint64 downloadedBytes,
        const QList<core::SegmentInfo>& segments
    );
    void clearMap();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawRibbonBar(QPainter& painter, const QRectF& rect) const;
    void drawBlockGrid(QPainter& painter, const QRectF& rect) const;

    qint64 m_totalBytes{0};
    qint64 m_downloadedBytes{0};
    QList<core::SegmentInfo> m_segments;
};

} // namespace idm::ui::widgets
