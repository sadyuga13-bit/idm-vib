#pragma once

#include "network/RequestProfile.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

namespace idm::network {

struct ProbeResult {
    bool success = false;
    QUrl finalUrl;
    QString suggestedFileName;
    QString mimeType;
    qint64 contentLength = -1;
    bool supportsRanges = false;
    int httpStatusCode = 0;
    QString errorMessage;
};

class UrlProber final : public QObject {
    Q_OBJECT

public:
    explicit UrlProber(QObject* parent = nullptr);
    ~UrlProber() override;

    void probeUrl(const QUrl& url, const RequestProfile& profile = RequestProfile{});
    void abort();
    [[nodiscard]] bool isProbing() const noexcept;

    [[nodiscard]] static QString extractFileName(
        const QUrl& url,
        const QByteArray& contentDispositionHeader
    );
    [[nodiscard]] static qint64 parseTotalFromContentRange(const QByteArray& contentRangeHeader);

signals:
    void probeCompleted(const idm::network::ProbeResult& result);

private:
    void startHeadRequest();
    void startRangeGetFallback();
    void inspectAndFinalizeReply(QNetworkReply* reply, bool isFallbackGet);

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_activeReply;
    QUrl m_targetUrl;
    RequestProfile m_profile;
    bool m_probing{false};
    bool m_finalized{false};
};

} // namespace idm::network
