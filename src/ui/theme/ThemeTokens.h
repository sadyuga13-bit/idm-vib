#pragma once

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QString>

namespace idm::ui::theme {

struct ThemeTokens {
    inline static bool s_dayMode = false;

    [[nodiscard]] static bool isDayMode() noexcept { return s_dayMode; }
    static void setDayMode(bool enabled) noexcept { s_dayMode = enabled; }

    // Surface QColor Helpers (Adaptive Modern Obsidian-Slate Dark / Crisp Daylight)
    [[nodiscard]] static QColor bgWindow() {
        return s_dayMode ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x09, 0x0D, 0x16);
    }
    [[nodiscard]] static QColor bgPanel() {
        return s_dayMode ? QColor(0xF8, 0xFA, 0xFC) : QColor(0x0F, 0x15, 0x23);
    }
    [[nodiscard]] static QColor bgToolbar() {
        return s_dayMode ? QColor(0xF1, 0xF5, 0xF9) : QColor(0x13, 0x1B, 0x2E);
    }
    [[nodiscard]] static QColor bgRowAlt() {
        return s_dayMode ? QColor(0xF8, 0xFA, 0xFC) : QColor(0x0D, 0x13, 0x20);
    }
    [[nodiscard]] static QColor bgHover() {
        return s_dayMode ? QColor(0xE2, 0xE8, 0xF0) : QColor(0x17, 0x21, 0x36);
    }
    [[nodiscard]] static QColor bgSelected() {
        return s_dayMode ? QColor(0xDB, 0xEA, 0xFE) : QColor(0x1B, 0x2A, 0x4A);
    }
    [[nodiscard]] static QColor borderHairline() {
        return s_dayMode ? QColor(0xCB, 0xD5, 0xE1) : QColor(0x1E, 0x29, 0x3B);
    }

    // Typography QColor Helpers
    [[nodiscard]] static QColor textPrimary() {
        return s_dayMode ? QColor(0x0F, 0x17, 0x2A) : QColor(0xF1, 0xF5, 0xF9);
    }
    [[nodiscard]] static QColor textSecondary() {
        return s_dayMode ? QColor(0x47, 0x55, 0x69) : QColor(0x94, 0xA3, 0xB8);
    }
    [[nodiscard]] static QColor textTechnical() {
        return s_dayMode ? QColor(0x64, 0x74, 0x8B) : QColor(0x64, 0x74, 0x8B);
    }

    // Semantic Status QColor Helpers
    [[nodiscard]] static QColor statusActive() {
        return s_dayMode ? QColor(0x25, 0x63, 0xEB) : QColor(0x3B, 0x82, 0xF6);
    }
    [[nodiscard]] static QColor statusDone() {
        return s_dayMode ? QColor(0x05, 0x96, 0x69) : QColor(0x10, 0xB9, 0x81);
    }
    [[nodiscard]] static QColor statusPaused() {
        return s_dayMode ? QColor(0xD9, 0x77, 0x06) : QColor(0xF5, 0x9E, 0x0B);
    }
    [[nodiscard]] static QColor statusError() {
        return s_dayMode ? QColor(0xDC, 0x26, 0x26) : QColor(0xEF, 0x44, 0x44);
    }

    // Modern Responsive Metrics
    static constexpr int kRowHeightPx       = 42;
    static constexpr int kHeaderHeightPx    = 34;
    static constexpr int kProgressBarHeight = 7;
    static constexpr int kSelectionBarWidth = 3;

    [[nodiscard]] static QFont uiFont(bool bold = false) {
        QFont font(QStringLiteral("Segoe UI Variable Text"));
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Inter"));
        }
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Segoe UI"));
        }
        font.setPixelSize(13);
        font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
        font.setStyleStrategy(QFont::PreferAntialias);
        return font;
    }

    [[nodiscard]] static QFont monoFont(bool bold = false) {
        QFont font(QStringLiteral("JetBrains Mono"));
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Cascadia Mono"));
        }
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Consolas"));
        }
        font.setPointSizeF(9.0);
        font.setFixedPitch(true);
        font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
        font.setStyleHint(QFont::Monospace);
        return font;
    }
};

} // namespace idm::ui::theme
