#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>

namespace idm::core {

class HashVerifier final : public QObject {
    Q_OBJECT

public:
    explicit HashVerifier(QObject* parent = nullptr);
    ~HashVerifier() override;

    void verifyFileAsync(
        const QString& taskId,
        const QString& filePath,
        const QString& expectedHash = QString(),
        qint64 expectedSizeBytes = 0
    );

    void cancel();
    [[nodiscard]] bool isRunning() const noexcept;

signals:
    void verificationProgress(const QString& taskId, int percent);
    void verificationFinished(
        const QString& taskId,
        const QString& sha256Hex,
        const QString& md5Hex,
        bool matchedExpected,
        const QString& errorMessage
    );

private:
    void cleanupThread();

    QThread* m_workerThread{nullptr};
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_running{false};
};

} // namespace idm::core
