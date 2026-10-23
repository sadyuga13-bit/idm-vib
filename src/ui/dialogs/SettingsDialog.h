#pragma once

#include "network/YoutubeExtractor.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace idm::ui::dialogs {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

signals:
    void settingsApplied();

private:
    QWidget* buildGeneralTab();
    QWidget* buildEngineTab();
    QWidget* buildYoutubeTab();
    QWidget* buildBrowserNetworkTab();

    void onSaveSettings();
    void onInstallYtDlp();
    void onOpenChromeExtFolder();

    network::YoutubeExtractor m_ytExtractor;

    QComboBox* m_themeCombo{nullptr};
    QCheckBox* m_clipboardCheck{nullptr};
    QLineEdit* m_defaultDirEdit{nullptr};

    QSpinBox* m_defaultSegmentsSpin{nullptr};
    QSpinBox* m_maxConcurrentSpin{nullptr};
    QSpinBox* m_speedLimitSpin{nullptr};

    QComboBox* m_ytQualityCombo{nullptr};
    QLabel* m_lblYtDlpStatus{nullptr};
    QPushButton* m_btnInstallYtDlp{nullptr};

    QSpinBox* m_ipcPortSpin{nullptr};
    QLineEdit* m_userAgentEdit{nullptr};
    QCheckBox* m_proxyCheck{nullptr};
    QLineEdit* m_proxyHostEdit{nullptr};
    QSpinBox* m_proxyPortSpin{nullptr};
};

} // namespace idm::ui::dialogs
