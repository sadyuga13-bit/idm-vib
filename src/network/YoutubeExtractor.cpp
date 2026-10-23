#include "network/YoutubeExtractor.h"
#include "core/Types.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrlQuery>

namespace idm::network {

namespace {
constexpr const char* kAndroidUserAgent =
    "com.google.android.youtube/20.10.38 (Linux; U; Android 14) gzip";
} // namespace

YoutubeExtractor::YoutubeExtractor(QObject* parent) : QObject(parent) {}
YoutubeExtractor::~YoutubeExtractor() { abort(); }

bool YoutubeExtractor::isYoutubeUrl(const QUrl& url) {
    if (!url.isValid()) {
        return false;
    }
    const QString host = url.host().toLower();
    if (host == QStringLiteral("youtu.be") || host.endsWith(QStringLiteral("youtube.com")) ||
        host.endsWith(QStringLiteral("youtube-nocookie.com"))) {
        return !extractVideoId(url).isEmpty();
    }
    return false;
}

QString YoutubeExtractor::extractVideoId(const QUrl& url) {
    const QString host = url.host().toLower();
    const QString path = url.path();
    if (host == QStringLiteral("youtu.be")) {
        const QString id = path.mid(1).split(QLatin1Char('/')).value(0).trimmed();
        return (id.size() >= 10) ? id : QString();
    }
    const QUrlQuery query(url);
    if (query.hasQueryItem(QStringLiteral("v"))) {
        return query.queryItemValue(QStringLiteral("v")).trimmed();
    }
    static const QRegularExpression kPathRx(QStringLiteral(R"(/(?:shorts|embed|live|v)/([A-Za-z0-9_-]{10,15}))"));
    const QRegularExpressionMatch m = kPathRx.match(path);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString YoutubeExtractor::sanitizeVideoFileName(const QString& rawTitle, const QString& qualityTag, const QString& extension) {
    QString base = rawTitle.trimmed().isEmpty() ? QStringLiteral("YouTube_Video") : rawTitle.trimmed();
    static const QRegularExpression kIllegal(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])"));
    base.replace(kIllegal, QStringLiteral("_"));
    base = base.simplified().left(110).trimmed();
    const QString ext = extension.isEmpty() ? QStringLiteral("mp4") : extension;
    return qualityTag.isEmpty() ? QStringLiteral("%1.%2").arg(base, ext)
                                : QStringLiteral("%1 [%2].%3").arg(base, qualityTag, ext);
}

QString YoutubeExtractor::locateYtDlpBinary() {
    const QString appBin = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("yt-dlp.exe"));
    return QFileInfo::exists(appBin) ? appBin : QStandardPaths::findExecutable(QStringLiteral("yt-dlp"));
}

void YoutubeExtractor::abort() {
    m_extracting = false;
    if (m_activeReply != nullptr) {
        m_activeReply->disconnect(this);
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply.clear();
    }
}

void YoutubeExtractor::extractVideoInfo(const QUrl& url) {
    abort();
    const QString videoId = extractVideoId(url);
    if (videoId.isEmpty()) {
        YoutubeVideoInfo info;
        info.errorMessage = QStringLiteral("Could not extract a valid 11-character YouTube video ID.");
        emit extractionCompleted(info);
        return;
    }

    m_extracting = true;
    QNetworkRequest swReq(QUrl(QStringLiteral("https://www.youtube.com/sw.js_data")));
    swReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));

    QNetworkReply* reply = m_nam.get(swReq);
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, videoId]() {
        if (!m_extracting) {
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        reply->deleteLater();
        m_activeReply.clear();

        if (raw.startsWith(")]}'")) {
            raw = raw.mid(4);
        }
        QString visitorData;
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        if (doc.isArray()) {
            visitorData = doc.array().at(0).toArray().at(2).toArray()
                             .at(0).toArray().at(0).toArray().at(13).toString();
        }
        if (visitorData.isEmpty()) {
            static const QRegularExpression kVdRx(QStringLiteral(R"((Cgt[A-Za-z0-9_%-]+))"));
            const auto m = kVdRx.match(QString::fromUtf8(raw));
            if (m.hasMatch()) {
                visitorData = m.captured(1);
            }
        }
        requestInnertubePlayer(videoId, visitorData);
    });
}

void YoutubeExtractor::requestInnertubePlayer(const QString& videoId, const QString& visitorData) {
    QJsonObject clientObj{
        {QStringLiteral("clientName"), QStringLiteral("ANDROID")},
        {QStringLiteral("clientVersion"), QStringLiteral("20.10.38")},
        {QStringLiteral("osName"), QStringLiteral("Android")},
        {QStringLiteral("osVersion"), QStringLiteral("14")},
        {QStringLiteral("androidSdkVersion"), 34},
        {QStringLiteral("hl"), QStringLiteral("en")}
    };
    if (!visitorData.isEmpty()) {
        clientObj[QStringLiteral("visitorData")] = visitorData;
    }
    const QJsonObject root{
        {QStringLiteral("context"), QJsonObject{{QStringLiteral("client"), clientObj}}},
        {QStringLiteral("videoId"), videoId},
        {QStringLiteral("contentCheckOk"), true},
        {QStringLiteral("racyCheckOk"), true}
    };

    QNetworkRequest req(QUrl(QStringLiteral("https://www.youtube.com/youtubei/v1/player?prettyPrint=false")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("User-Agent", kAndroidUserAgent);
    if (!visitorData.isEmpty()) {
        req.setRawHeader("X-Goog-Visitor-Id", visitorData.toUtf8());
    }

    QNetworkReply* reply = m_nam.post(req, QJsonDocument(root).toJson(QJsonDocument::Compact));
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, videoId]() {
        if (!m_extracting) {
            reply->deleteLater();
            return;
        }
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        m_activeReply.clear();
        m_extracting = false;
        parseInnertubeResponse(videoId, body);
    });
}

void YoutubeExtractor::parseInnertubeResponse(const QString& videoId, const QByteArray& rawJson) {
    YoutubeVideoInfo info;
    info.videoId = videoId;

    const QJsonObject root = QJsonDocument::fromJson(rawJson).object();
    const QJsonObject details = root.value(QStringLiteral("videoDetails")).toObject();
    info.title = details.value(QStringLiteral("title")).toString().trimmed();
    info.author = details.value(QStringLiteral("author")).toString().trimmed();

    const QJsonObject streaming = root.value(QStringLiteral("streamingData")).toObject();
    auto appendFormatItem = [&info](const QJsonObject& f, bool isMuxed) {
        const QString url = f.value(QStringLiteral("url")).toString();
        if (url.isEmpty()) {
            return;
        }
        const QString mime = f.value(QStringLiteral("mimeType")).toString();
        const int itag = f.value(QStringLiteral("itag")).toInt();
        const qint64 len = f.value(QStringLiteral("contentLength")).toString().toLongLong();
        const QString qLabel = f.value(QStringLiteral("qualityLabel")).toString();

        YoutubeFormat fmt{itag, {}, {}, url, QString::fromLatin1(kAndroidUserAgent), QStringLiteral("mp4"), len, true, false};
        if (isMuxed) {
            fmt.hasAudio = true;
            fmt.qualityTag = qLabel.isEmpty() ? QStringLiteral("360p") : qLabel;
            fmt.displayLabel = QStringLiteral("[Video + Audio] %1 MP4 (Ready to Play)").arg(fmt.qualityTag);
            info.formats.prepend(fmt);
        } else if (mime.startsWith(QStringLiteral("audio/mp4"))) {
            fmt.hasVideo = false;
            fmt.hasAudio = true;
            fmt.fileExtension = QStringLiteral("m4a");
            fmt.qualityTag = QStringLiteral("Audio_M4A");
            fmt.displayLabel = QStringLiteral("[Audio Only] M4A AAC (%1 - itag %2)")
                                   .arg(core::FormatUtils::formatBytes(len)).arg(itag);
            info.formats.append(fmt);
        } else if (mime.startsWith(QStringLiteral("video/mp4")) && !qLabel.isEmpty()) {
            fmt.qualityTag = qLabel;
            const QString codec = mime.contains(QStringLiteral("avc1")) ? QStringLiteral("H.264") : QStringLiteral("AV1");
            fmt.displayLabel = QStringLiteral("[HD Video] %1 MP4 %2 (%3)")
                                   .arg(qLabel, codec, core::FormatUtils::formatBytes(len));
            info.formats.append(fmt);
        }
    };

    for (const QJsonValue& v : streaming.value(QStringLiteral("formats")).toArray()) {
        appendFormatItem(v.toObject(), true);
    }
    for (const QJsonValue& v : streaming.value(QStringLiteral("adaptiveFormats")).toArray()) {
        appendFormatItem(v.toObject(), false);
    }

    if (!info.formats.isEmpty()) {
        info.success = true;
        emit extractionCompleted(info);
        return;
    }
    if (tryFallbackYtDlpProcess(videoId)) {
        return;
    }
    info.errorMessage = root.value(QStringLiteral("playabilityStatus")).toObject()
                            .value(QStringLiteral("reason"))
                            .toString(QStringLiteral("No direct streams returned by YouTube Innertube API."));
    emit extractionCompleted(info);
}

bool YoutubeExtractor::tryFallbackYtDlpProcess(const QString& videoId) {
    const QString bin = locateYtDlpBinary();
    if (bin.isEmpty()) {
        return false;
    }
    auto* proc = new QProcess(this);
    const QString watchUrl = QStringLiteral("https://www.youtube.com/watch?v=%1").arg(videoId);
    connect(proc, &QProcess::finished, this, [this, proc, videoId](int, QProcess::ExitStatus) {
        const QJsonObject obj = QJsonDocument::fromJson(proc->readAllStandardOutput()).object();
        proc->deleteLater();
        YoutubeVideoInfo info{false, videoId, obj.value(QStringLiteral("title")).toString(), {}, {}, {}};
        for (const QJsonValue& v : obj.value(QStringLiteral("formats")).toArray()) {
            const QJsonObject f = v.toObject();
            const QString u = f.value(QStringLiteral("url")).toString();
            const QString ext = f.value(QStringLiteral("ext")).toString();
            if (u.startsWith(QStringLiteral("http")) && (ext == QStringLiteral("mp4") || ext == QStringLiteral("m4a"))) {
                YoutubeFormat yf{0, f.value(QStringLiteral("format_note")).toString(ext), {}, u, {}, ext,
                                 f.value(QStringLiteral("filesize")).toVariant().toLongLong(), true, true};
                yf.displayLabel = QStringLiteral("[%1] %2 (%3)")
                    .arg(ext.toUpper(), yf.qualityTag, core::FormatUtils::formatBytes(yf.contentLength));
                info.formats.prepend(yf);
            }
        }
        info.success = !info.formats.isEmpty();
        emit extractionCompleted(info);
    });
    proc->start(bin, {QStringLiteral("-J"), QStringLiteral("--no-playlist"), QStringLiteral("--no-warnings"), watchUrl});
    return true;
}

void YoutubeExtractor::downloadYtDlpBinary(const QString& targetPath) {
    QNetworkRequest req(QUrl(QStringLiteral("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe")));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("IdmVibe-Updater/1.0"));

    QNetworkReply* reply = m_nam.get(req);
    connect(reply, &QNetworkReply::downloadProgress, this, &YoutubeExtractor::ytDlpDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetPath]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            reply->deleteLater();
            emit ytDlpDownloadFinished(false, err);
            return;
        }
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            reply->deleteLater();
            emit ytDlpDownloadFinished(false, file.errorString());
            return;
        }
        file.write(reply->readAll());
        file.close();
        reply->deleteLater();
        emit ytDlpDownloadFinished(true, QStringLiteral("yt-dlp.exe installed at %1").arg(targetPath));
    });
}

} // namespace idm::network
