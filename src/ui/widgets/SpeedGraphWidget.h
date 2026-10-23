#pragma once

#include <QList>
#include <QWidget>

namespace idm::ui::widgets {

class SpeedGraphWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SpeedGraphWidget(QWidget* parent = nullptr);
    ~SpeedGraphWidget() override = default;

    void pushSpeedSample(qint64 bytesPerSec);
    void clearHistory();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int kMaxSamples = 60;
    QList<qint64> m_samples;
    qint64 m_peakBytesPerSec{0};
};

} // namespace idm::ui::widgets
