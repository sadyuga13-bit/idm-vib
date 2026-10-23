#pragma once

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QString>

namespace idm::ui::theme {

struct ThemeTokens {
    // Surface Hex Tokens
    static constexpr const char* kBgWindowHex       = "#0B0C0E";
    static constexpr const char* kBgPanelHex        = "#111317";
    static constexpr const char* kBgToolbarHex      = "#15181E";
    static constexpr const char* kBgRowAltHex       = "#13161B";
    static constexpr const char* kBgHoverHex        = "#1A1E26";
    static constexpr const char* kBgSelectedHex     = "#1E2638";
    static constexpr const char* kBorderHairlineHex = "#222630";

    // Typography Hex Tokens
    static constexpr const char* kTextPrimaryHex    = "#E6E8EC";
    static constexpr const char* kTextSecondaryHex  = "#949BA6";
    static constexpr const char* kTextTechnicalHex  = "#636A75";

    // Semantic Status Hex Tokens
    static constexpr const char* kStatusActiveHex   = "#3B82F6";
    static constexpr const char* kStatusDoneHex     = "#10B981";
    static constexpr const char* kStatusPausedHex   = "#F59E0B";
    static constexpr const char* kStatusErrorHex    = "#EF4444";

    // QColor Helpers
    [[nodiscard]] static QColor bgWindow()       { return QColor(0x0B, 0x0C, 0x0E); }
    [[nodiscard]] static QColor bgPanel()        { return QColor(0x11, 0x13, 0x17); }
    [[nodiscard]] static QColor bgToolbar()      { return QColor(0x15, 0x18, 0x1E); }
    [[nodiscard]] static QColor bgRowAlt()       { return QColor(0x13, 0x16, 0x1B); }
    [[nodiscard]] static QColor bgHover()        { return QColor(0x1A, 0x1E, 0x26); }
    [[nodiscard]] static QColor bgSelected()     { return QColor(0x1E, 0x26, 0x38); }
    [[nodiscard]] static QColor borderHairline() { return QColor(0x22, 0x26, 0x30); }

    [[nodiscard]] static QColor textPrimary()    { return QColor(0xE6, 0xE8, 0xEC); }
    [[nodiscard]] static QColor textSecondary()  { return QColor(0x94, 0x9B, 0xA6); }
    [[nodiscard]] static QColor textTechnical()  { return QColor(0x63, 0x6A, 0x75); }

    [[nodiscard]] static QColor statusActive()   { return QColor(0x3B, 0x82, 0xF6); }
    [[nodiscard]] static QColor statusDone()     { return QColor(0x10, 0xB9, 0x81); }
    [[nodiscard]] static QColor statusPaused()   { return QColor(0xF5, 0x9E, 0x0B); }
    [[nodiscard]] static QColor statusError()    { return QColor(0xEF, 0x44, 0x44); }

    // Metrics
    static constexpr int kRowHeightPx       = 32;
    static constexpr int kHeaderHeightPx    = 28;
    static constexpr int kProgressBarHeight = 4;
    static constexpr int kSelectionBarWidth = 2;

    [[nodiscard]] static QFont uiFont(bool bold = false) {
        QFont font(QStringLiteral("Segoe UI Variable Text"));
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Inter"));
        }
        if (!QFontDatabase::hasFamily(font.family())) {
            font.setFamily(QStringLiteral("Segoe UI"));
        }
        font.setPixelSize(12);
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
        font.setPointSizeF(8.75); // ~11.5px at 96 DPI with crisp tabular alignment
        font.setFixedPitch(true);
        font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
        font.setStyleHint(QFont::Monospace);
        return font;
    }
};

} // namespace idm::ui::theme
