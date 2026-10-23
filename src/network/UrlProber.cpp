#include "network/UrlProber.h"

#include <QFileInfo>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

namespace idm::network {

UrlProber::UrlProber(QObject* parent)
    : QObject(parent) {}

UrlProber::~UrlProber() {
    abort();
}

void UrlProber::probeUrl(const QUrl& url, const RequestProfile& profile) {
    abort();

    if (!url.isValid() || url.scheme().isEmpty()) {
        ProbeResult res;
        res.errorMessage = QStringLiteral("Invalid URL specified.");
        emit probeCompleted(res);
        return;
    }

    m_targetUrl = url;
    m_profile = profile;
    m_probing = true;
    m_finalized = false;
    m_profile.applyProxyToManager(&m_nam);

    startHeadRequest();
}

void UrlProber::abort() {
    m_probing = false;
    m_finalized = true;
    if (m_activeReply != nullptr) {
        m_activeReply->disconnect(this);
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply.clear();
    }
}

bool UrlProber::isProbing() const noexcept {
    return m_probing;
}

void UrlProber::startHeadRequest() {
    QNetworkRequest request(m_targetUrl);
    m_profile.applyToRequest(request);

    QNetworkReply* reply = m_nam.head(request);
    m_activeReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (!m_probing || m_finalized) {
            reply->deleteLater();
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const qint64 length = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        const QByteArray acceptRanges = reply->rawHeader("Accept-Ranges").toLower();

        const bool headSucceeded = (reply->error() == QNetworkReply::NoError) &&
                                   (statusCode >= 200 && statusCode < 300) &&
                                   (length > 0) &&
                                   acceptRanges.contains("bytes");

        if (headSucceeded) {
            inspectAndFinalizeReply(reply, false);
            reply->deleteLater();
        } else {
            reply->deleteLater();
            m_activeReply.clear();
            startRangeGetFallback();
        }
    });
}

void UrlProber::startRangeGetFallback() {
    QNetworkRequest request(m_targetUrl);
    m_profile.applyToRequest(request);
    request.setRawHeader("Range", "bytes=0-0");

    QNetworkReply* reply = m_nam.get(request);
    m_activeReply = reply;

    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply]() {
        if (!m_probing || m_finalized) {
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 200 && status < 300) {
            inspectAndFinalizeReply(reply, true);
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (!m_probing || m_finalized) {
            reply->deleteLater();
            return;
        }
        inspectAndFinalizeReply(reply, true);
        reply->deleteLater();
    });
}

void UrlProber::inspectAndFinalizeReply(QNetworkReply* reply, bool isFallbackGet) {
    if (m_finalized) {
        return;
    }
    m_finalized = true;
    m_probing = false;

    ProbeResult result;
    result.finalUrl = reply->url().isValid() ? reply->url() : m_targetUrl;
    result.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.mimeType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    const bool abortedAfterMeta = (reply->error() == QNetworkReply::OperationCanceledError) &&
                                  (result.httpStatusCode >= 200 && result.httpStatusCode < 300);

    if (reply->error() != QNetworkReply::NoError && !abortedAfterMeta) {
        result.success = false;
        result.errorMessage = reply->errorString();
        result.suggestedFileName = extractFileName(result.finalUrl, QByteArray());
        emit probeCompleted(result);
        return;
    }

    result.success = true;
    const QByteArray acceptRanges = reply->rawHeader("Accept-Ranges").toLower();
    const QByteArray contentRange = reply->rawHeader("Content-Range");
    const qint64 rangeTotal = parseTotalFromContentRange(contentRange);

    if (result.httpStatusCode == 206 || acceptRanges.contains("bytes") || rangeTotal > 0) {
        result.supportsRanges = true;
    }

    if (rangeTotal > 0) {
        result.contentLength = rangeTotal;
    } else {
        const QVariant lenVar = reply->header(QNetworkRequest::ContentLengthHeader);
        if (lenVar.isValid()) {
            const qint64 reportedLen = lenVar.toLongLong();
            if (!isFallbackGet || result.httpStatusCode != 206) {
                result.contentLength = reportedLen;
            }
        }
    }

    result.suggestedFileName = extractFileName(
        result.finalUrl,
        reply->rawHeader("Content-Disposition")
    );

    emit probeCompleted(result);
}

QString UrlProber::extractFileName(
    const QUrl& url,
    const QByteArray& contentDispositionHeader
) {
    QString candidate;
    const QString disp = QString::fromUtf8(contentDispositionHeader);

    if (!disp.isEmpty()) {
        static const QRegularExpression kRfc5987Rx(
            QStringLiteral(R"(filename\*\s*=\s*(?:UTF-8''|utf-8'')([^;\r\n]+))")
        );
        const QRegularExpressionMatch mStar = kRfc5987Rx.match(disp);
        if (mStar.hasMatch()) {
            candidate = QUrl::fromPercentEncoding(mStar.captured(1).trimmed().toUtf8());
        }

        if (candidate.isEmpty()) {
            static const QRegularExpression kQuotedRx(
                QStringLiteral(R"(filename\s*=\s*"([^"]+)")")
            );
            const QRegularExpressionMatch mQuoted = kQuotedRx.match(disp);
            if (mQuoted.hasMatch()) {
                candidate = mQuoted.captured(1).trimmed();
            }
        }

        if (candidate.isEmpty()) {
            static const QRegularExpression kPlainRx(
                QStringLiteral(R"(filename\s*=\s*([^;\s\r\n]+))")
            );
            const QRegularExpressionMatch mPlain = kPlainRx.match(disp);
            if (mPlain.hasMatch()) {
                candidate = mPlain.captured(1).trimmed();
            }
        }
    }

    if (candidate.isEmpty()) {
        const QString path = QUrl::fromPercentEncoding(url.path().toUtf8());
        candidate = QFileInfo(path).fileName();
    }

    static const QRegularExpression kIllegalChars(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])"));
    candidate.replace(kIllegalChars, QStringLiteral("_"));
    candidate = candidate.trimmed();

    if (candidate.isEmpty() || candidate == QStringLiteral(".")) {
        candidate = QStringLiteral("download.bin");
    }
    return candidate;
}

qint64 UrlProber::parseTotalFromContentRange(const QByteArray& contentRangeHeader) {
    if (contentRangeHeader.isEmpty()) {
        return -1;
    }
    const QString text = QString::fromLatin1(contentRangeHeader);
    static const QRegularExpression kRangeRx(QStringLiteral(R"(bytes\s+\d+-\d+/(\d+))"));
    const QRegularExpressionMatch match = kRangeRx.match(text);
    if (match.hasMatch()) {
        bool ok = false;
        const qint64 total = match.captured(1).toLongLong(&ok);
        return ok ? total : -1;
    }
    return -1;
}

} // namespace idm::network
