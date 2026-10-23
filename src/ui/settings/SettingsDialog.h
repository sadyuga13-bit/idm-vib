#pragma once

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>

namespace idm::ui::settings {

class GeneralSettingsPage;
class FileTypesSettingsPage;
class SavePathsSettingsPage;
class ConnectionSettingsPage;
class ProxyAndLoginsPage;
class CompletionAndAvPage;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

signals:
    void settingsApplied();

private:
    void onSaveAndApply();

    QListWidget* m_navList{nullptr};
    QStackedWidget* m_stack{nullptr};

    GeneralSettingsPage* m_generalPage{nullptr};
    FileTypesSettingsPage* m_fileTypesPage{nullptr};
    SavePathsSettingsPage* m_savePathsPage{nullptr};
    ConnectionSettingsPage* m_connectionPage{nullptr};
    ProxyAndLoginsPage* m_proxyPage{nullptr};
    CompletionAndAvPage* m_completionPage{nullptr};
};

} // namespace idm::ui::settings

namespace idm::ui {
using SettingsDialog = settings::SettingsDialog;
} // namespace idm::ui
