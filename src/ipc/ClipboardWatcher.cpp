#include "ipc/ClipboardWatcher.h"
#include "storage/CategoryRouter.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QUrl>

namespace idm::ipc {

ClipboardWatcher::ClipboardWatcher(QObject* parent)
    : QObject(parent)
    , m_extensions(storage::CategoryRouter::defaultCapturedExtensions()) {
    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        connect(clipboard, &QClipboard::dataChanged, this, &ClipboardWatcher::onClipboardChanged);
    }
}

void ClipboardWatcher::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void ClipboardWatcher::setMonitoredExtensions(const QStringList& extensions) {
    m_extensions = extensions;
}

void ClipboardWatcher::onClipboardChanged() {
    if (!m_enabled) {
        return;
    }

    const QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }

    const QMimeData* mime = clipboard->mimeData();
    if (mime == nullptr || !mime->hasText()) {
        return;
    }

    const QString text = mime->text().trimmed();
    if (text.isEmpty() || text.size() > 2048 || text.contains(QLatin1Char('\n'))) {
        return;
    }

    if (text == m_lastCapturedUrl) {
        return;
    }

    const QUrl url(text);
    if (!url.isValid()) {
        return;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("http") &&
        scheme != QStringLiteral("https") &&
        scheme != QStringLiteral("ftp")) {
        return;
    }

    const QString urlPath = url.path();
    if (!storage::CategoryRouter::matchesExtensionList(urlPath, m_extensions)) {
        return;
    }

    m_lastCapturedUrl = text;
    emit downloadUrlDetected(text);
}

} // namespace idm::ipc
