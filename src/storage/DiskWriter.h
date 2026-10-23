#pragma once

#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QString>

namespace idm::storage {

class DiskWriter final {
public:
    DiskWriter() = default;
    ~DiskWriter();

    DiskWriter(const DiskWriter&) = delete;
    DiskWriter& operator=(const DiskWriter&) = delete;

    [[nodiscard]] bool openAndPreallocate(
        const QString& targetFilePath,
        qint64 totalBytes,
        bool preserveExistingPart,
        QString* errorOut = nullptr
    );

    [[nodiscard]] bool writeAt(
        qint64 offset,
        const char* data,
        qint64 length,
        QString* errorOut = nullptr
    );

    [[nodiscard]] bool writeAt(
        qint64 offset,
        const QByteArray& buffer,
        QString* errorOut = nullptr
    );

    void flush();
    void close();

    [[nodiscard]] bool finalizeFile(QString* errorOut = nullptr);
    void removePartFile();

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] QString targetFilePath() const;
    [[nodiscard]] QString partFilePath() const;

private:
    mutable QMutex m_mutex;
    QFile m_file;
    QString m_targetPath;
    QString m_partPath;
};

} // namespace idm::storage
