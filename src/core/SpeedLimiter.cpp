#include "core/SpeedLimiter.h"

#include <QMutexLocker>
#include <algorithm>

namespace idm::core {

SpeedLimiter::SpeedLimiter(QObject* parent)
    : QObject(parent)
    , m_lastRefillTime(std::chrono::steady_clock::now()) {}

void SpeedLimiter::setLimitKiBPerSec(qint64 kibPerSec) {
    const qint64 clampedKiB = std::max<qint64>(0, kibPerSec);
    const qint64 bytesPerSec = clampedKiB * 1024;
    {
        QMutexLocker locker(&m_mutex);
        m_limitBytesPerSec.store(bytesPerSec, std::memory_order_relaxed);
        m_enabled.store(bytesPerSec > 0, std::memory_order_relaxed);
        m_availableTokens = static_cast<double>(bytesPerSec) * 0.25;
        m_lastRefillTime = std::chrono::steady_clock::now();
    }
    emit limitChanged(clampedKiB, bytesPerSec > 0);
}

qint64 SpeedLimiter::limitKiBPerSec() const noexcept {
    return m_limitBytesPerSec.load(std::memory_order_relaxed) / 1024;
}

bool SpeedLimiter::isEnabled() const noexcept {
    return m_enabled.load(std::memory_order_relaxed) &&
           m_limitBytesPerSec.load(std::memory_order_relaxed) > 0;
}

void SpeedLimiter::setEnabled(bool enabled) {
    {
        QMutexLocker locker(&m_mutex);
        m_enabled.store(enabled, std::memory_order_relaxed);
        m_lastRefillTime = std::chrono::steady_clock::now();
        if (enabled) {
            const qint64 rate = m_limitBytesPerSec.load(std::memory_order_relaxed);
            m_availableTokens = static_cast<double>(rate) * 0.25;
        }
    }
    emit limitChanged(limitKiBPerSec(), isEnabled());
}

qint64 SpeedLimiter::requestQuota(qint64 requestedBytes) {
    if (requestedBytes <= 0) {
        return 0;
    }
    if (!isEnabled()) {
        return requestedBytes;
    }

    QMutexLocker locker(&m_mutex);
    refillTokensLocked();

    if (m_availableTokens < 1.0) {
        return 0;
    }

    const qint64 granted = std::min<qint64>(
        requestedBytes,
        static_cast<qint64>(m_availableTokens)
    );
    m_availableTokens -= static_cast<double>(granted);
    return granted;
}

void SpeedLimiter::refillTokensLocked() {
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = now - m_lastRefillTime;
    m_lastRefillTime = now;

    const double rate = static_cast<double>(m_limitBytesPerSec.load(std::memory_order_relaxed));
    if (rate <= 0.0) {
        m_availableTokens = 0.0;
        return;
    }

    const double maxBurst = std::max(rate * 0.5, 16384.0);
    m_availableTokens = std::min(maxBurst, m_availableTokens + elapsed.count() * rate);
}

} // namespace idm::core
