#include "core/HashVerifier.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QMetaObject>

namespace idm::core {

HashVerifier::HashVerifier(QObject* parent)
    : QObject(parent) {}

HashVerifier::~HashVerifier() {
    cancel();
    cleanupThread();
}

void HashVerifier::verifyFileAsync(
    const QString& taskId,
    const QString& filePath,
    const QString& expectedHash
) {
    cancel();
    cleanupThread();

    m_cancelRequested.store(false, std::memory_order_relaxed);
    m_running.store(true, std::memory_order_relaxed);

    m_workerThread = QThread::create([this, taskId, filePath, expectedHash]() {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            const QString err = file.errorString();
            m_running.store(false, std::memory_order_relaxed);
            QMetaObject::invokeMethod(
                this,
                [this, taskId, err]() {
                    emit verificationFinished(taskId, QString(), QString(), false, err);
                },
                Qt::QueuedConnection
            );
            return;
        }

        const qint64 totalSize = file.size();
        qint64 bytesReadTotal = 0;
        int lastReportedPercent = -1;

        QCryptographicHash sha256(QCryptographicHash::Sha256);
        QCryptographicHash md5(QCryptographicHash::Md5);

        QByteArray buffer(256 * 1024, Qt::Uninitialized);

        while (!file.atEnd()) {
            if (m_cancelRequested.load(std::memory_order_relaxed)) {
                file.close();
                m_running.store(false, std::memory_order_relaxed);
                return;
            }

            const qint64 chunkBytes = file.read(buffer.data(), buffer.size());
            if (chunkBytes < 0) {
                const QString err = file.errorString();
                file.close();
                m_running.store(false, std::memory_order_relaxed);
                QMetaObject::invokeMethod(
                    this,
                    [this, taskId, err]() {
                        emit verificationFinished(taskId, QString(), QString(), false, err);
                    },
                    Qt::QueuedConnection
                );
                return;
            }

            const QByteArrayView view(buffer.constData(), chunkBytes);
            sha256.addData(view);
            md5.addData(view);
            bytesReadTotal += chunkBytes;

            if (totalSize > 0) {
                const int pct = static_cast<int>((bytesReadTotal * 100) / totalSize);
                if (pct != lastReportedPercent) {
                    lastReportedPercent = pct;
                    QMetaObject::invokeMethod(
                        this,
                        [this, taskId, pct]() { emit verificationProgress(taskId, pct); },
                        Qt::QueuedConnection
                    );
                }
            }
        }

        file.close();

        const QString sha256Hex = QString::fromLatin1(sha256.result().toHex()).toLower();
        const QString md5Hex = QString::fromLatin1(md5.result().toHex()).toLower();
        const QString normalizedExpected = expectedHash.trimmed().toLower();

        bool matched = true;
        if (!normalizedExpected.isEmpty()) {
            matched = (normalizedExpected == sha256Hex) || (normalizedExpected == md5Hex);
        }

        m_running.store(false, std::memory_order_relaxed);
        QMetaObject::invokeMethod(
            this,
            [this, taskId, sha256Hex, md5Hex, matched]() {
                emit verificationFinished(taskId, sha256Hex, md5Hex, matched, QString());
            },
            Qt::QueuedConnection
        );
    });

    m_workerThread->start();
}

void HashVerifier::cancel() {
    m_cancelRequested.store(true, std::memory_order_relaxed);
}

bool HashVerifier::isRunning() const noexcept {
    return m_running.load(std::memory_order_relaxed);
}

void HashVerifier::cleanupThread() {
    if (m_workerThread != nullptr) {
        if (m_workerThread->isRunning()) {
            m_workerThread->quit();
            m_workerThread->wait(3000);
        }
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

} // namespace idm::core
