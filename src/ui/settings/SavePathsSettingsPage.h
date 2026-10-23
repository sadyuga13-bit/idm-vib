#pragma once

#include "core/Types.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QWidget>

namespace idm::ui::settings {

class SavePathsSettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit SavePathsSettingsPage(QWidget* parent = nullptr);
    ~SavePathsSettingsPage() override = default;

    void applyToConfig();

private:
    void onCategoryIndexChanged(int idx);
    void refreshFreeDiskLabel(const QString& folderPath);

    QComboBox* m_categoryCombo{nullptr};
    QLineEdit* m_catPathEdit{nullptr};
    QLabel* m_lblFreeSpace{nullptr};
    QLineEdit* m_tempPartEdit{nullptr};
    QCheckBox* m_chkPreallocate{nullptr};

    QMap<core::FileCategory, QString> m_pendingPaths;
};

} // namespace idm::ui::settings
