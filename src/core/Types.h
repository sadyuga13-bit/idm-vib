#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QString>
#include <QtGlobal>
#include <algorithm>

namespace idm::core {

enum class TaskState : int {
    Queued = 0,
    Probing = 1,
    Active = 2,
    Paused = 3,
    Verifying = 4,
    Completed = 5,
    Failed = 6,
    Canceled = 7
};

enum class SegmentState : int {
    Idle = 0,
    Connecting = 1,
    Downloading = 2,
    Completed = 3,
    Paused = 4,
    Error = 5
};

enum class FileCategory : int {
    All = 0,
    Archive = 1,
    Video = 2,
    Audio = 3,
    Document = 4,
    Application = 5,
    Image = 6,
    Other = 7
};

struct SegmentInfo {
    int id = 0;
    qint64 startOffset = 0;
    qint64 currentOffset = 0;
    qint64 endOffset = -1;
    qint64 speedBytesPerSec = 0;
    SegmentState state = SegmentState::Idle;
    QString statusText;

    [[nodiscard]] qint64 totalBytes() const noexcept {
        if (endOffset < startOffset) {
            return 0;
        }
        return (endOffset - startOffset) + 1;
    }

    [[nodiscard]] qint64 downloadedBytes() const noexcept {
        return std::max<qint64>(0, currentOffset - startOffset);
    }

    [[nodiscard]] qint64 remainingBytes() const noexcept {
        if (endOffset < currentOffset) {
            return 0;
        }
        return (endOffset - currentOffset) + 1;
    }

    [[nodiscard]] double progressRatio() const noexcept {
        const qint64 total = totalBytes();
        if (total <= 0) {
            return state == SegmentState::Completed ? 1.0 : 0.0;
        }
        return std::clamp(static_cast<double>(downloadedBytes()) / static_cast<double>(total), 0.0, 1.0);
    }
};

struct TaskMetrics {
    QString taskId;
    QString url;
    QString fileName;
    QString saveDirectory;
    QString queueName = QStringLiteral("Main Queue");
    FileCategory category = FileCategory::Other;
    TaskState state = TaskState::Queued;
    qint64 totalBytes = 0;
    qint64 downloadedBytes = 0;
    qint64 speedBytesPerSec = 0;
    qint64 etaSeconds = -1;
    int maxSegments = 8;
    bool resumeSupported = false;
    QString expectedHash;
    QString computedSha256;
    QString computedMd5;
    QString errorMessage;
    QDateTime createdAt = QDateTime::currentDateTimeUtc();
    QList<SegmentInfo> segments;

    [[nodiscard]] QString fullSavePath() const {
        return QDir(saveDirectory).filePath(fileName);
    }

    [[nodiscard]] double progressPercent() const noexcept {
        if (totalBytes <= 0) {
            return state == TaskState::Completed ? 100.0 : 0.0;
        }
        return std::clamp((static_cast<double>(downloadedBytes) * 100.0) / static_cast<double>(totalBytes), 0.0, 100.0);
    }
};

struct FormatUtils {
    [[nodiscard]] static QString formatBytes(qint64 bytes) {
        if (bytes < 0) {
            return QStringLiteral("Unknown");
        }
        constexpr double kKiB = 1024.0;
        constexpr double kMiB = kKiB * 1024.0;
        constexpr double kGiB = kMiB * 1024.0;

        if (bytes >= static_cast<qint64>(kGiB)) {
            return QStringLiteral("%1 GB").arg(static_cast<double>(bytes) / kGiB, 0, 'f', 2);
        }
        if (bytes >= static_cast<qint64>(kMiB)) {
            return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / kMiB, 0, 'f', 2);
        }
        if (bytes >= static_cast<qint64>(kKiB)) {
            return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / kKiB, 0, 'f', 1);
        }
        return QStringLiteral("%1 B").arg(bytes);
    }

    [[nodiscard]] static QString formatSpeed(qint64 bytesPerSec) {
        if (bytesPerSec <= 0) {
            return QStringLiteral("0.0 KB/s");
        }
        return formatBytes(bytesPerSec) + QStringLiteral("/s");
    }

    [[nodiscard]] static QString formatEta(qint64 seconds) {
        if (seconds < 0 || seconds > 359999) {
            return QStringLiteral("--:--");
        }
        const qint64 hrs = seconds / 3600;
        const qint64 mins = (seconds % 3600) / 60;
        const qint64 secs = seconds % 60;
        if (hrs > 0) {
            return QStringLiteral("%1:%2:%3")
                .arg(hrs, 2, 10, QLatin1Char('0'))
                .arg(mins, 2, 10, QLatin1Char('0'))
                .arg(secs, 2, 10, QLatin1Char('0'));
        }
        return QStringLiteral("%1:%2")
            .arg(mins, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'));
    }

    [[nodiscard]] static QString stateToString(TaskState state) {
        switch (state) {
            case TaskState::Queued:    return QStringLiteral("Queued");
            case TaskState::Probing:   return QStringLiteral("Probing");
            case TaskState::Active:    return QStringLiteral("Downloading");
            case TaskState::Paused:    return QStringLiteral("Paused");
            case TaskState::Verifying: return QStringLiteral("Verifying");
            case TaskState::Completed: return QStringLiteral("Completed");
            case TaskState::Failed:    return QStringLiteral("Error");
            case TaskState::Canceled:  return QStringLiteral("Canceled");
        }
        return QStringLiteral("Unknown");
    }

    [[nodiscard]] static QString segmentStateToString(SegmentState state) {
        switch (state) {
            case SegmentState::Idle:        return QStringLiteral("Idle");
            case SegmentState::Connecting:  return QStringLiteral("Connecting");
            case SegmentState::Downloading: return QStringLiteral("Receiving");
            case SegmentState::Completed:   return QStringLiteral("Finished");
            case SegmentState::Paused:      return QStringLiteral("Paused");
            case SegmentState::Error:       return QStringLiteral("Error");
        }
        return QStringLiteral("Idle");
    }
};

} // namespace idm::core
