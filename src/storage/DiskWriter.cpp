#include "storage/DiskWriter.h"

#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStorageInfo>
#include <QThread>

namespace idm::storage {

DiskWriter::~DiskWriter() {
    close();
}

bool DiskWriter::openAndPreallocate(
    const QString& targetFilePath,
    qint64 totalBytes,
    bool preserveExistingPart,
    QString* errorOut
) {
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }

    m_targetPath = QDir::cleanPath(targetFilePath);
    m_partPath = m_targetPath + QStringLiteral(".part");

    const QFileInfo targetInfo(m_targetPath);
    const QString parentDir = targetInfo.absolutePath();
    if (!QDir().mkpath(parentDir)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Failed to create target directory: %1").arg(parentDir);
        }
        return false;
    }

    if (totalBytes > 0) {
        const QStorageInfo storageInfo(parentDir);
        if (storageInfo.isValid() && storageInfo.bytesAvailable() > 0 &&
            storageInfo.bytesAvailable() < totalBytes) {
            if (errorOut != nullptr) {
                *errorOut = QStringLiteral("Insufficient disk space on volume (%1 available).")
                                .arg(storageInfo.bytesAvailable());
            }
            return false;
        }
    }

    m_file.setFileName(m_partPath);
    QIODevice::OpenMode mode = QIODevice::ReadWrite;
    if (!preserveExistingPart) {
        mode |= QIODevice::Truncate;
    }

    if (!m_file.open(mode)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Cannot open part file '%1': %2")
                            .arg(m_partPath, m_file.errorString());
        }
        return false;
    }

    if (totalBytes > 0 && m_file.size() != totalBytes) {
        if (!m_file.resize(totalBytes)) {
            if (errorOut != nullptr) {
                *errorOut = QStringLiteral("Disk pre-allocation failed: %1")
                                .arg(m_file.errorString());
            }
            m_file.close();
            return false;
        }
    }

    return true;
}

bool DiskWriter::writeAt(
    qint64 offset,
    const char* data,
    qint64 length,
    QString* errorOut
) {
    if (length <= 0) {
        return true;
    }

    QMutexLocker locker(&m_mutex);
    if (!m_file.isOpen()) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("DiskWriter is not open.");
        }
        return false;
    }

    if (!m_file.seek(offset)) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Disk seek failed at offset %1: %2")
                            .arg(offset)
                            .arg(m_file.errorString());
        }
        return false;
    }

    qint64 totalWritten = 0;
    while (totalWritten < length) {
        const qint64 written = m_file.write(data + totalWritten, length - totalWritten);
        if (written <= 0) {
            if (errorOut != nullptr) {
                *errorOut = QStringLiteral("Disk write failed at offset %1: %2")
                                .arg(offset + totalWritten)
                                .arg(m_file.errorString());
            }
            return false;
        }
        totalWritten += written;
    }

    return true;
}

bool DiskWriter::writeAt(
    qint64 offset,
    const QByteArray& buffer,
    QString* errorOut
) {
    return writeAt(offset, buffer.constData(), buffer.size(), errorOut);
}

void DiskWriter::flush() {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.flush();
    }
}

void DiskWriter::close() {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
}

bool DiskWriter::finalizeFile(QString* errorOut) {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }

    if (!QFile::exists(m_partPath)) {
        if (QFile::exists(m_targetPath)) {
            return true;
        }
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Part file missing during finalization: %1").arg(m_partPath);
        }
        return false;
    }

    for (int attempt = 0; attempt < 15; ++attempt) {
        if (QFile::exists(m_targetPath)) {
            QFile::setPermissions(m_targetPath, QFileDevice::ReadOther | QFileDevice::WriteOther |
                                                QFileDevice::ReadUser | QFileDevice::WriteUser |
                                                QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            QFile::remove(m_targetPath);
        }
        if (QFile::rename(m_partPath, m_targetPath)) {
            return true;
        }
        QThread::msleep(80);
    }

    if (QFile::copy(m_partPath, m_targetPath)) {
        QFile::remove(m_partPath);
        return true;
    }

    if (errorOut != nullptr) {
        *errorOut = QStringLiteral("Failed to finalize '%1' -> '%2' (locked by Antivirus/OS).")
                        .arg(m_partPath, m_targetPath);
    }
    return false;
}

void DiskWriter::removePartFile() {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.close();
    }
    if (!m_partPath.isEmpty() && QFile::exists(m_partPath)) {
        QFile::remove(m_partPath);
    }
}

bool DiskWriter::isOpen() const noexcept {
    QMutexLocker locker(&m_mutex);
    return m_file.isOpen();
}

QString DiskWriter::targetFilePath() const {
    QMutexLocker locker(&m_mutex);
    return m_targetPath;
}

QString DiskWriter::partFilePath() const {
    QMutexLocker locker(&m_mutex);
    return m_partPath;
}

} // namespace idm::storage
