#pragma once

#include "core/Types.h"
#include <QList>
#include <QWidget>

namespace idm::ui::widgets {

class SegmentBarWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SegmentBarWidget(QWidget* parent = nullptr);
    ~SegmentBarWidget() override = default;

    void setSegments(qint64 totalBytes, const QList<core::SegmentInfo>& segments);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qint64 m_totalBytes{0};
    QList<core::SegmentInfo> m_segments;
};

} // namespace idm::ui::widgets
