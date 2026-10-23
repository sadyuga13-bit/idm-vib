#pragma once

#include <QComboBox>
#include <QPlainTextEdit>
#include <QWidget>

namespace idm::ui::settings {

class FileTypesSettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit FileTypesSettingsPage(QWidget* parent = nullptr);
    ~FileTypesSettingsPage() override = default;

    void applyToConfig();

private:
    QPlainTextEdit* m_extensionsEdit{nullptr};
    QPlainTextEdit* m_excludedDomainsEdit{nullptr};
    QComboBox* m_duplicatePolicyCombo{nullptr};
};

} // namespace idm::ui::settings
