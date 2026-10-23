#pragma once

#include "core/Types.h"
#include "network/RequestProfile.h"
#include "network/UrlProber.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace idm::ui::dialogs {

class AddDownloadDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AddDownloadDialog(
        const QString& initialUrl = QString(),
        const network::RequestProfile& initialProfile = network::RequestProfile{},
        QWidget* parent = nullptr
    );
    ~AddDownloadDialog() override = default;

    [[nodiscard]] core::TaskMetrics configuredMetrics() const;
    [[nodiscard]] network::RequestProfile configuredProfile() const;
    [[nodiscard]] bool startImmediately() const noexcept { return m_startImmediately; }

private:
    void triggerProbe();
    void onProbeCompleted(const network::ProbeResult& result);
    void onBrowseFolder();

    network::UrlProber m_prober;
    qint64 m_probedBytes{0};
    bool m_probedResume{false};
    bool m_startImmediately{true};

    QLineEdit* m_urlEdit{nullptr};
    QPushButton* m_btnProbe{nullptr};
    QLabel* m_lblProbeStatus{nullptr};

    QLineEdit* m_fileNameEdit{nullptr};
    QLineEdit* m_saveDirEdit{nullptr};
    QComboBox* m_queueCombo{nullptr};
    QSpinBox* m_segmentsSpin{nullptr};
    QLineEdit* m_hashEdit{nullptr};

    QLineEdit* m_refererEdit{nullptr};
    QLineEdit* m_cookieEdit{nullptr};
    QLineEdit* m_userAgentEdit{nullptr};
    QLineEdit* m_authUserEdit{nullptr};
    QLineEdit* m_authPassEdit{nullptr};
    QCheckBox* m_proxyCheck{nullptr};
    QLineEdit* m_proxyHostEdit{nullptr};
    QSpinBox* m_proxyPortSpin{nullptr};
};

} // namespace idm::ui::dialogs
