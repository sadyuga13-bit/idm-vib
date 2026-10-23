#pragma once

#include <QMutex>
#include <QObject>
#include <atomic>
#include <chrono>

namespace idm::core {

class SpeedLimiter final : public QObject {
    Q_OBJECT

public:
    explicit SpeedLimiter(QObject* parent = nullptr);
    ~SpeedLimiter() override = default;

    void setLimitKiBPerSec(qint64 kibPerSec);
    [[nodiscard]] qint64 limitKiBPerSec() const noexcept;
    [[nodiscard]] bool isEnabled() const noexcept;
    void setEnabled(bool enabled);

    [[nodiscard]] qint64 requestQuota(qint64 requestedBytes);

signals:
    void limitChanged(qint64 kibPerSec, bool enabled);

private:
    void refillTokensLocked();

    mutable QMutex m_mutex;
    std::atomic<qint64> m_limitBytesPerSec{0};
    std::atomic<bool> m_enabled{false};
    double m_availableTokens{0.0};
    std::chrono::steady_clock::time_point m_lastRefillTime;
};

} // namespace idm::core
