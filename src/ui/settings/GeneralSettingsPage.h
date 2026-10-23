#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

namespace idm::ui::settings {

class GeneralSettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit GeneralSettingsPage(QWidget* parent = nullptr);
    ~GeneralSettingsPage() override = default;

    void applyToConfig();

private:
    QComboBox* m_langCombo{nullptr};
    QComboBox* m_themeCombo{nullptr};
    QCheckBox* m_chkStartWindows{nullptr};
    QCheckBox* m_chkCloseToTray{nullptr};
    QCheckBox* m_chkPreventSleep{nullptr};
    QCheckBox* m_chkClipboard{nullptr};
    QCheckBox* m_chkAltSuppress{nullptr};
    QCheckBox* m_chkInsertForce{nullptr};
};

} // namespace idm::ui::settings
