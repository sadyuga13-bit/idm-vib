#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace idm::ipc {

class ClipboardWatcher final : public QObject {
    Q_OBJECT

public:
    explicit ClipboardWatcher(QObject* parent = nullptr);
    ~ClipboardWatcher() override = default;

    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const noexcept { return m_enabled; }

    void setMonitoredExtensions(const QStringList& extensions);
    [[nodiscard]] const QStringList& monitoredExtensions() const noexcept { return m_extensions; }

signals:
    void downloadUrlDetected(const QString& url);

private:
    void onClipboardChanged();

    bool m_enabled{true};
    QStringList m_extensions;
    QString m_lastCapturedUrl;
};

} // namespace idm::ipc
