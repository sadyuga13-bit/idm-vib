#pragma once

#include <QApplication>
#include <QString>

namespace idm::ui::theme {

class AppStyleSheet final {
public:
    AppStyleSheet() = delete;

    [[nodiscard]] static QString buildGlobalStyleSheet();
    static void applyToApplication(QApplication& app);
    static void setDayMode(bool dayMode);
};

} // namespace idm::ui::theme
