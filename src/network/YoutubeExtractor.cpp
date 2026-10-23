#include "network/YoutubeExtractor.h"
#include "core/Types.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>
#include <algorithm>

namespace idm::network {

namespace {
struct InnertubeClientProfile {
    const char* clientName;
    const char* clientVersion;
    const char* deviceMake;
    const char* deviceModel;
    const char* osName;
    const char* osVersion;
    int sdkVersion;
    const char* userAgent;
};

constexpr InnertubeClientProfile kClientProfiles[] = {
    {
        "ANDROID_VR", "1.60.19", "Oculus", "Quest 3", "Android", "12L", 32,
        "com.google.android.apps.youtube.vr.oculus/1.60.19 (Linux; U; Android 12L; eureka-user Build/SQ3A.220605.009.A1) gzip"
    },
    {
        "IOS", "20.10.4", "Apple", "iPhone16,2", "iPhone", "18.3.1.22D72", 0,
        "com.google.ios.youtube/20.10.4 (iPhone16,2; U; CPU iOS 18_3_1 like Mac OS X;)"
    },
    {
        "ANDROID", "20.10.38", "Google", "Pixel 8 Pro", "Android", "14", 34,
        "com.google.android.youtube/20.10.38 (Linux; U; Android 14) gzip"
    }
};
} // namespace

YoutubeExtractor::YoutubeExtractor(QObject* parent) : QObject(parent) {}
YoutubeExtractor::~YoutubeExtractor() { abort(); }

bool YoutubeExtractor::isYoutubeUrl(const QUrl& url) {
    if (!url.isValid()) return false;
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

QString YoutubeExtractor::sanitizeVideoFileName(
    const QString& rawTitle,
    const QString& qualityTag,
    const QString& extension
) {
    QString base = rawTitle.trimmed().isEmpty() ? QStringLiteral("YouTube_Video") : rawTitle.trimmed();
    static const QRegularExpression kIllegal(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])"));
    base.replace(kIllegal, QStringLiteral("_"));
    base = base.simplified().left(110).trimmed();
    const QString ext = extension.isEmpty() ? QStringLiteral("mp4") : extension;
    return qualityTag.isEmpty()
        ? QStringLiteral("%1.%2").arg(base, ext)
        : QStringLiteral("%1 [%2].%3").arg(base, qualityTag, ext);
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
            if (m.hasMatch()) visitorData = m.captured(1);
        }
        requestInnertubeClientStage(videoId, visitorData, 0);
    });
}

void YoutubeExtractor::requestInnertubeClientStage(
    const QString& videoId,
    const QString& visitorData,
    int stageIndex
) {
    if (stageIndex < 0 || stageIndex >= 3) {
        m_extracting = false;
        YoutubeVideoInfo failInfo;
        failInfo.videoId = videoId;
        failInfo.errorMessage = QStringLiteral("All native VibeTube client profiles exhausted.");
        emit extractionCompleted(failInfo);
        return;
    }

    const InnertubeClientProfile& prof = kClientProfiles[stageIndex];
    QJsonObject clientObj{
        {QStringLiteral("clientName"), QString::fromLatin1(prof.clientName)},
        {QStringLiteral("clientVersion"), QString::fromLatin1(prof.clientVersion)},
        {QStringLiteral("deviceMake"), QString::fromLatin1(prof.deviceMake)},
        {QStringLiteral("deviceModel"), QString::fromLatin1(prof.deviceModel)},
        {QStringLiteral("osName"), QString::fromLatin1(prof.osName)},
        {QStringLiteral("osVersion"), QString::fromLatin1(prof.osVersion)},
        {QStringLiteral("hl"), QStringLiteral("en")},
        {QStringLiteral("gl"), QStringLiteral("US")}
    };
    if (prof.sdkVersion > 0) {
        clientObj[QStringLiteral("androidSdkVersion")] = prof.sdkVersion;
    }
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
    req.setRawHeader("User-Agent", prof.userAgent);
    if (!visitorData.isEmpty()) {
        req.setRawHeader("X-Goog-Visitor-Id", visitorData.toUtf8());
    }

    QNetworkReply* reply = m_nam.post(req, QJsonDocument(root).toJson(QJsonDocument::Compact));
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, videoId, visitorData, stageIndex, prof]() {
        if (!m_extracting) {
            reply->deleteLater();
            return;
        }
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        m_activeReply.clear();

        YoutubeVideoInfo parsed;
        if (parseInnertubeResponse(videoId, body, QString::fromLatin1(prof.clientName),
                                   QString::fromLatin1(prof.userAgent), &parsed)) {
            m_extracting = false;
            emit extractionCompleted(parsed);
            return;
        }
        requestInnertubeClientStage(videoId, visitorData, stageIndex + 1);
    });
}

bool YoutubeExtractor::parseInnertubeResponse(
    const QString& videoId,
    const QByteArray& rawJson,
    const QString& clientLabel,
    const QString& clientUserAgent,
    YoutubeVideoInfo* outInfo
) {
    const QJsonObject root = QJsonDocument::fromJson(rawJson).object();
    const QJsonObject details = root.value(QStringLiteral("videoDetails")).toObject();
    outInfo->videoId = videoId;
    outInfo->activeClientName = clientLabel;
    outInfo->title = details.value(QStringLiteral("title")).toString().trimmed();
    outInfo->author = details.value(QStringLiteral("author")).toString().trimmed();

    const QJsonObject streaming = root.value(QStringLiteral("streamingData")).toObject();
    auto appendFormatItem = [&outInfo, &clientUserAgent](const QJsonObject& f, bool isMuxed) {
        const QString url = f.value(QStringLiteral("url")).toString();
        if (url.isEmpty()) return;

        const QString mime = f.value(QStringLiteral("mimeType")).toString();
        const int itag = f.value(QStringLiteral("itag")).toInt();
        const int height = f.value(QStringLiteral("height")).toInt();
        const qint64 len = f.value(QStringLiteral("contentLength")).toString().toLongLong();
        const QString qLabel = f.value(QStringLiteral("qualityLabel")).toString();

        YoutubeFormat fmt{itag, height, {}, {}, url, clientUserAgent, QStringLiteral("mp4"), len, true, false};
        if (isMuxed) {
            fmt.hasAudio = true;
            fmt.qualityTag = qLabel.isEmpty() ? QStringLiteral("360p") : qLabel;
            fmt.displayLabel = QStringLiteral("[Video + Audio] %1 MP4 (%2)")
                .arg(fmt.qualityTag, len > 0 ? core::FormatUtils::formatBytes(len) : QStringLiteral("Direct Stream"));
            outInfo->formats.append(fmt);
        } else if (mime.startsWith(QStringLiteral("audio/mp4"))) {
            fmt.hasVideo = false;
            fmt.hasAudio = true;
            fmt.fileExtension = QStringLiteral("m4a");
            fmt.qualityTag = QStringLiteral("Audio_M4A");
            fmt.displayLabel = QStringLiteral("[Audio M4A] High-Bitrate AAC (%1)")
                .arg(core::FormatUtils::formatBytes(len));
            outInfo->formats.append(fmt);
        } else if (mime.startsWith(QStringLiteral("video/mp4")) && !qLabel.isEmpty()) {
            fmt.qualityTag = qLabel;
            const QString codec = mime.contains(QStringLiteral("avc1")) ? QStringLiteral("H.264") : QStringLiteral("AV1/HDR");
            fmt.displayLabel = QStringLiteral("[Ultra HD] %1 MP4 %2 (%3)")
                .arg(qLabel, codec, core::FormatUtils::formatBytes(len));
            outInfo->formats.append(fmt);
        }
    };

    for (const QJsonValue& v : streaming.value(QStringLiteral("formats")).toArray()) {
        appendFormatItem(v.toObject(), true);
    }
    for (const QJsonValue& v : streaming.value(QStringLiteral("adaptiveFormats")).toArray()) {
        appendFormatItem(v.toObject(), false);
    }

    std::sort(outInfo->formats.begin(), outInfo->formats.end(), [](const YoutubeFormat& a, const YoutubeFormat& b) {
        if (a.hasAudio != b.hasAudio && a.hasVideo && b.hasVideo) return a.hasAudio > b.hasAudio;
        return a.height > b.height;
    });

    outInfo->success = !outInfo->formats.isEmpty();
    return outInfo->success;
}

} // namespace idm::network
