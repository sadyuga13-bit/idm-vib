#include "storage/CategoryRouter.h"
#include "i18n/I18nManager.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>

namespace idm::storage {

core::FileCategory CategoryRouter::classifyFileName(const QString& fileName) {
    const QString lower = fileName.trimmed().toLower();
    if (lower.endsWith(QStringLiteral(".tar.gz")) ||
        lower.endsWith(QStringLiteral(".tar.xz")) ||
        lower.endsWith(QStringLiteral(".tar.bz2"))) {
        return core::FileCategory::Archive;
    }

    const QString ext = QFileInfo(lower).suffix();
    if (ext.isEmpty()) {
        return core::FileCategory::Other;
    }

    static const QSet<QString> kArchives = {
        QStringLiteral("zip"), QStringLiteral("rar"), QStringLiteral("7z"),
        QStringLiteral("tar"), QStringLiteral("gz"),  QStringLiteral("bz2"),
        QStringLiteral("xz"),  QStringLiteral("zst"), QStringLiteral("cab"),
        QStringLiteral("tgz")
    };
    static const QSet<QString> kVideos = {
        QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("webm"),
        QStringLiteral("avi"), QStringLiteral("mov"), QStringLiteral("flv"),
        QStringLiteral("wmv"), QStringLiteral("m4v"), QStringLiteral("ts")
    };
    static const QSet<QString> kAudio = {
        QStringLiteral("mp3"),  QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("aac"),  QStringLiteral("ogg"),  QStringLiteral("m4a"),
        QStringLiteral("opus"), QStringLiteral("wma")
    };
    static const QSet<QString> kDocuments = {
        QStringLiteral("pdf"),  QStringLiteral("doc"),  QStringLiteral("docx"),
        QStringLiteral("xls"),  QStringLiteral("xlsx"), QStringLiteral("ppt"),
        QStringLiteral("pptx"), QStringLiteral("epub"), QStringLiteral("txt"),
        QStringLiteral("csv"),  QStringLiteral("md")
    };
    static const QSet<QString> kApps = {
        QStringLiteral("exe"),  QStringLiteral("msi"),  QStringLiteral("iso"),
        QStringLiteral("dmg"),  QStringLiteral("pkg"),  QStringLiteral("deb"),
        QStringLiteral("rpm"),  QStringLiteral("apk"),  QStringLiteral("appimage"),
        QStringLiteral("msix"), QStringLiteral("bin")
    };
    static const QSet<QString> kImages = {
        QStringLiteral("png"),  QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("webp"), QStringLiteral("gif"), QStringLiteral("svg"),
        QStringLiteral("bmp"),  QStringLiteral("ico"), QStringLiteral("tiff")
    };

    if (kArchives.contains(ext))  return core::FileCategory::Archive;
    if (kVideos.contains(ext))    return core::FileCategory::Video;
    if (kAudio.contains(ext))     return core::FileCategory::Audio;
    if (kDocuments.contains(ext)) return core::FileCategory::Document;
    if (kApps.contains(ext))      return core::FileCategory::Application;
    if (kImages.contains(ext))    return core::FileCategory::Image;

    return core::FileCategory::Other;
}

QString CategoryRouter::categoryDisplayName(core::FileCategory category) {
    const bool ar = i18n::I18nManager::instance().isRightToLeft();
    switch (category) {
        case core::FileCategory::All:         return ar ? QStringLiteral("جميع التحميلات") : QStringLiteral("All Downloads");
        case core::FileCategory::Archive:     return ar ? QStringLiteral("الملفات المضغوطة") : QStringLiteral("Compressed");
        case core::FileCategory::Video:       return ar ? QStringLiteral("الفيديو والمرئيات") : QStringLiteral("Video");
        case core::FileCategory::Audio:       return ar ? QStringLiteral("الصوتيات والموسيقى") : QStringLiteral("Music");
        case core::FileCategory::Document:    return ar ? QStringLiteral("المستندات والكتب") : QStringLiteral("Documents");
        case core::FileCategory::Application: return ar ? QStringLiteral("البرامج والتطبيقات") : QStringLiteral("Programs");
        case core::FileCategory::Image:       return ar ? QStringLiteral("الصور والرسوميات") : QStringLiteral("Images");
        case core::FileCategory::Other:       return ar ? QStringLiteral("غير مكتمل / أخرى") : QStringLiteral("Unfinished / Other");
    }
    return ar ? QStringLiteral("عام") : QStringLiteral("General");
}

QString CategoryRouter::defaultDirectoryForCategory(core::FileCategory category) {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + QStringLiteral("/Downloads");
    }

    QString subFolder;
    switch (category) {
        case core::FileCategory::Archive:     subFolder = QStringLiteral("Compressed"); break;
        case core::FileCategory::Video:       subFolder = QStringLiteral("Video"); break;
        case core::FileCategory::Audio:       subFolder = QStringLiteral("Music"); break;
        case core::FileCategory::Document:    subFolder = QStringLiteral("Documents"); break;
        case core::FileCategory::Application: subFolder = QStringLiteral("Programs"); break;
        case core::FileCategory::Image:       subFolder = QStringLiteral("Images"); break;
        default:                              subFolder = QString(); break;
    }

    const QDir root(baseDir);
    const QString targetPath = subFolder.isEmpty() ? root.absolutePath() : root.filePath(subFolder);
    QDir().mkpath(targetPath);
    return QDir::toNativeSeparators(targetPath);
}

QString CategoryRouter::defaultDirectoryForFile(const QString& fileName) {
    return defaultDirectoryForCategory(classifyFileName(fileName));
}

QStringList CategoryRouter::defaultCapturedExtensions() {
    return {
        QStringLiteral("zip"), QStringLiteral("rar"), QStringLiteral("7z"),
        QStringLiteral("tar.gz"), QStringLiteral("exe"), QStringLiteral("msi"),
        QStringLiteral("iso"), QStringLiteral("dmg"), QStringLiteral("mp4"),
        QStringLiteral("mkv"), QStringLiteral("mp3"), QStringLiteral("flac"),
        QStringLiteral("pdf"), QStringLiteral("apk")
    };
}

bool CategoryRouter::matchesExtensionList(
    const QString& fileNameOrUrlPath,
    const QStringList& extensions
) {
    const QString lower = fileNameOrUrlPath.trimmed().toLower();
    for (const QString& ext : extensions) {
        const QString cleanExt = ext.trimmed().toLower();
        if (cleanExt.isEmpty()) {
            continue;
        }
        const QString suffix = cleanExt.startsWith(QLatin1Char('.'))
                                   ? cleanExt
                                   : (QLatin1Char('.') + cleanExt);
        if (lower.endsWith(suffix)) {
            return true;
        }
    }
    return false;
}

} // namespace idm::storage
