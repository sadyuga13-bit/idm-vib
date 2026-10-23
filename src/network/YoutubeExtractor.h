#pragma once

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

namespace idm::network {

struct YoutubeFormat {
    int itag{0};
    int height{0};
    QString qualityTag;
    QString displayLabel;
    QString directUrl;
    QString userAgent;
    QString fileExtension{QStringLiteral("mp4")};
    qint64 contentLength{0};
    bool hasVideo{true};
    bool hasAudio{false};
};

struct YoutubeVideoInfo {
    bool success{false};
    QString videoId;
    QString title;
    QString author;
    QString activeClientName;
    QString errorMessage;
    QList<YoutubeFormat> formats;
};

class YoutubeExtractor final : public QObject {
    Q_OBJECT

public:
    explicit YoutubeExtractor(QObject* parent = nullptr);
    ~YoutubeExtractor() override;

    [[nodiscard]] static bool isYoutubeUrl(const QUrl& url);
    [[nodiscard]] static QString extractVideoId(const QUrl& url);
    [[nodiscard]] static QString sanitizeVideoFileName(
        const QString& rawTitle,
        const QString& qualityTag,
        const QString& extension
    );

    void extractVideoInfo(const QUrl& url);
    void abort();
    [[nodiscard]] bool isExtracting() const noexcept { return m_extracting; }

signals:
    void extractionCompleted(const idm::network::YoutubeVideoInfo& info);

private:
    void requestInnertubeClientStage(
        const QString& videoId,
        const QString& visitorData,
        int stageIndex
    );
    bool parseInnertubeResponse(
        const QString& videoId,
        const QByteArray& rawJson,
        const QString& clientLabel,
        const QString& clientUserAgent,
        YoutubeVideoInfo* outInfo
    );

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_activeReply;
    bool m_extracting{false};
};

} // namespace idm::network

Q_DECLARE_METATYPE(idm::network::YoutubeVideoInfo)
