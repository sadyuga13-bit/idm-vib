#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QWidget>

namespace idm::ui::settings {

class CompletionAndAvPage final : public QWidget {
    Q_OBJECT

public:
    explicit CompletionAndAvPage(QWidget* parent = nullptr);
    ~CompletionAndAvPage() override = default;

    void applyToConfig();

private:
    QCheckBox* m_chkShowStartConfirm{nullptr};
    QCheckBox* m_chkShowLiveProgress{nullptr};
    QCheckBox* m_chkShowCompleteDialog{nullptr};

    QCheckBox* m_chkEnableAvScan{nullptr};
    QComboBox* m_avEngineCombo{nullptr};
    QLineEdit* m_customAvPathEdit{nullptr};
    QCheckBox* m_chkWriteMotw{nullptr};

    QCheckBox* m_chkSoundAlerts{nullptr};
    QLineEdit* m_wavePathEdit{nullptr};
};

} // namespace idm::ui::settings
