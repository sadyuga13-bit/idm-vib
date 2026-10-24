#include "ui/theme/IconRegistry.h"
#include "ui/theme/ThemeTokens.h"

#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>

namespace idm::ui::theme {

QColor IconRegistry::colorForState(IconState state) noexcept {
    switch (state) {
        case IconState::Normal:   return ThemeTokens::textSecondary();
        case IconState::Hover:    return ThemeTokens::textPrimary();
        case IconState::Active:   return ThemeTokens::statusActive();
        case IconState::Disabled: return ThemeTokens::textTechnical();
        case IconState::Success:  return ThemeTokens::statusDone();
        case IconState::Warning:  return ThemeTokens::statusPaused();
        case IconState::Danger:   return ThemeTokens::statusError();
    }
    return ThemeTokens::textSecondary();
}

IconId IconRegistry::iconForCategory(core::FileCategory category) noexcept {
    switch (category) {
        case core::FileCategory::All:         return IconId::CategoryAll;
        case core::FileCategory::Archive:     return IconId::CategoryArchive;
        case core::FileCategory::Video:       return IconId::CategoryVideo;
        case core::FileCategory::Audio:       return IconId::CategoryAudio;
        case core::FileCategory::Document:    return IconId::CategoryDocument;
        case core::FileCategory::Application: return IconId::CategoryApp;
        case core::FileCategory::Image:       return IconId::CategoryImage;
        case core::FileCategory::Other:       return IconId::CategoryGeneric;
    }
    return IconId::CategoryGeneric;
}

QColor IconRegistry::semanticColorForIcon(IconId id) noexcept {
    switch (id) {
        case IconId::AddUrl:           return QColor(0x10, 0xB9, 0x81); // Emerald
        case IconId::BatchSequence:    return QColor(0x8B, 0x5C, 0xF6); // Violet
        case IconId::StartPlay:        return QColor(0x22, 0xC5, 0x5E); // Green
        case IconId::Pause:            return QColor(0xF5, 0x9E, 0x0B); // Amber
        case IconId::StopSquare:
        case IconId::StopAll:
        case IconId::TrashDelete:
        case IconId::YoutubePlay:
        case IconId::CategoryVideo:    return QColor(0xEF, 0x44, 0x44); // Crimson
        case IconId::SchedulerClock:
        case IconId::FolderOpen:
        case IconId::CategoryArchive:
        case IconId::ThemeSun:         return QColor(0xF5, 0x9E, 0x0B); // Gold/Amber
        case IconId::SpeedGauge:
        case IconId::ShieldVerify:
        case IconId::BadgeVerified:
        case IconId::CategoryAudio:    return QColor(0x06, 0xB6, 0xD4); // Cyan
        case IconId::SearchLens:
        case IconId::NetworkGlobe:
        case IconId::SettingsGear:
        case IconId::CategoryAll:
        case IconId::CategoryDocument:
        case IconId::DeviceLaptop:     return QColor(0x3B, 0x82, 0xF6); // Royal Blue
        case IconId::TerminalLog:
        case IconId::CpuThreads:
        case IconId::CpuChip:
        case IconId::CategoryApp:
        case IconId::BrowserExt:
        case IconId::QueueNight:
        case IconId::ThemeMoon:        return QColor(0x63, 0x66, 0xF1); // Indigo
        case IconId::HardDrive:
        case IconId::QueueMain:        return QColor(0x14, 0xB8, 0xA6); // Teal
        case IconId::CategoryImage:    return QColor(0xEC, 0x48, 0x99); // Pink
        case IconId::LockClosed:
        case IconId::KeySerial:        return QColor(0xEA, 0xB3, 0x08); // Yellow Gold
        case IconId::CategoryGeneric:  return QColor(0x64, 0x74, 0x8B); // Slate
    }
    return ThemeTokens::accentPrimary();
}

QPixmap IconRegistry::pixmap(IconId id, int sizePx, IconState state) {
    if (state == IconState::Normal) {
        return pixmapColored(id, sizePx, semanticColorForIcon(id));
    }
    if (state == IconState::Hover) {
        return pixmapColored(id, sizePx, semanticColorForIcon(id).lighter(120));
    }
    return pixmapColored(id, sizePx, colorForState(state));
}

QPixmap IconRegistry::pixmapColored(IconId id, int sizePx, const QColor& strokeColor) {
    static QHash<QString, QPixmap> s_cache;

    qreal dpr = 1.0;
    if (const QScreen* screen = QGuiApplication::primaryScreen()) {
        dpr = screen->devicePixelRatio();
    }

    const QString cacheKey = QStringLiteral("%1_%2_%3_%4")
        .arg(static_cast<int>(id))
        .arg(sizePx)
        .arg(strokeColor.name(QColor::HexArgb))
        .arg(dpr, 0, 'f', 2);

    if (const auto it = s_cache.constFind(cacheKey); it != s_cache.cend()) {
        return it.value();
    }

    const QString svgMarkup = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' "
        "fill='none' stroke='%1' stroke-width='1.85' "
        "stroke-linecap='round' stroke-linejoin='round'>%2</svg>"
    ).arg(strokeColor.name(QColor::HexRgb), svgBodyForIcon(id));

    const int pxDim = qRound(sizePx * dpr);
    QPixmap pm(pxDim, pxDim);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(svgMarkup.toUtf8());
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(0, 0, pxDim, pxDim));
    painter.end();

    pm.setDevicePixelRatio(dpr);
    s_cache.insert(cacheKey, pm);
    return pm;
}

QIcon IconRegistry::icon(IconId id, int sizePx) {
    QIcon ico;
    ico.addPixmap(pixmap(id, sizePx, IconState::Normal), QIcon::Normal, QIcon::Off);
    ico.addPixmap(pixmap(id, sizePx, IconState::Hover), QIcon::Active, QIcon::Off);
    ico.addPixmap(pixmap(id, sizePx, IconState::Active), QIcon::Selected, QIcon::Off);
    ico.addPixmap(pixmap(id, sizePx, IconState::Disabled), QIcon::Disabled, QIcon::Off);
    return ico;
}

QString IconRegistry::svgBodyForIcon(IconId id) {
    switch (id) {
        case IconId::AddUrl:
            return QStringLiteral("<circle cx='12' cy='12' r='9'/><line x1='12' y1='8' x2='12' y2='16'/><line x1='8' y1='12' x2='16' y2='12'/>");
        case IconId::BatchSequence:
            return QStringLiteral("<rect x='3' y='4' width='18' height='5' rx='1'/><rect x='3' y='11' width='14' height='5' rx='1'/><line x1='19' y1='16' x2='19' y2='22'/><line x1='16' y1='19' x2='22' y2='19'/>");
        case IconId::StartPlay:
            return QStringLiteral("<polygon points='6 4 20 12 6 20 6 4'/>");
        case IconId::Pause:
            return QStringLiteral("<rect x='6' y='4' width='4' height='16' rx='1'/><rect x='14' y='4' width='4' height='16' rx='1'/>");
        case IconId::StopSquare:
            return QStringLiteral("<rect x='5' y='5' width='14' height='14' rx='2'/>");
        case IconId::StopAll:
            return QStringLiteral("<rect x='3' y='3' width='8' height='8' rx='1'/><rect x='13' y='3' width='8' height='8' rx='1'/><rect x='3' y='13' width='8' height='8' rx='1'/><rect x='13' y='13' width='8' height='8' rx='1'/>");
        case IconId::TrashDelete:
            return QStringLiteral("<polyline points='3 6 5 6 21 6'/><path d='M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6'/><path d='M10 11v6'/><path d='M14 11v6'/><path d='M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2'/>");
        case IconId::SchedulerClock:
            return QStringLiteral("<circle cx='12' cy='12' r='9'/><polyline points='12 7 12 12 15.5 14'/>");
        case IconId::SpeedGauge:
            return QStringLiteral("<path d='M12 15l3.5-3.5'/><path d='M20.3 18A9 9 0 1 0 3.7 18Z'/>");
        case IconId::FolderOpen:
            return QStringLiteral("<path d='M3 7v11a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-7l-2-2H5a2 2 0 0 0-2 2Z'/>");
        case IconId::SearchLens:
            return QStringLiteral("<circle cx='11' cy='11' r='7'/><line x1='21' y1='21' x2='16.35' y2='16.35'/>");
        case IconId::ShieldVerify:
            return QStringLiteral("<path d='M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z'/><polyline points='9 12 11 14 15 10'/>");
        case IconId::TerminalLog:
            return QStringLiteral("<polyline points='4 17 10 11 4 5'/><line x1='12' y1='19' x2='20' y2='19'/>");
        case IconId::NetworkGlobe:
            return QStringLiteral("<circle cx='12' cy='12' r='9'/><line x1='3' y1='12' x2='21' y2='12'/><path d='M12 3a14.5 14.5 0 0 0 0 18 14.5 14.5 0 0 0 0-18'/>");
        case IconId::CpuThreads:
        case IconId::CpuChip:
            return QStringLiteral("<rect x='6' y='6' width='12' height='12' rx='2'/><rect x='9' y='9' width='6' height='6'/><line x1='9' y1='2' x2='9' y2='6'/><line x1='15' y1='2' x2='15' y2='6'/><line x1='9' y1='18' x2='9' y2='22'/><line x1='15' y1='18' x2='15' y2='22'/><line x1='2' y1='9' x2='6' y2='9'/><line x1='2' y1='15' x2='6' y2='15'/><line x1='18' y1='9' x2='22' y2='9'/><line x1='18' y1='15' x2='22' y2='15'/>");
        case IconId::HardDrive:
            return QStringLiteral("<line x1='22' y1='12' x2='2' y2='12'/><path d='M5.45 5.11L2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z'/><line x1='6' y1='16' x2='6.01' y2='16'/><line x1='10' y1='16' x2='10.01' y2='16'/>");
        case IconId::QueueMain:
            return QStringLiteral("<line x1='8' y1='6' x2='21' y2='6'/><line x1='8' y1='12' x2='21' y2='12'/><line x1='8' y1='18' x2='21' y2='18'/><line x1='3' y1='6' x2='3.01' y2='6'/><line x1='3' y1='12' x2='3.01' y2='12'/><line x1='3' y1='18' x2='3.01' y2='18'/>");
        case IconId::QueueNight:
        case IconId::ThemeMoon:
            return QStringLiteral("<path d='M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z'/>");
        case IconId::CategoryAll:
            return QStringLiteral("<path d='M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4'/><polyline points='7 10 12 15 17 10'/><line x1='12' y1='15' x2='12' y2='3'/>");
        case IconId::CategoryArchive:
            return QStringLiteral("<polyline points='21 8 21 21 3 21 3 8'/><rect x='1' y='3' width='22' height='5'/><line x1='10' y1='12' x2='14' y2='12'/>");
        case IconId::CategoryVideo:
        case IconId::YoutubePlay:
            return QStringLiteral("<rect x='2' y='4' width='20' height='16' rx='3'/><polygon points='10 9 16 12 10 15 10 9'/>");
        case IconId::CategoryAudio:
            return QStringLiteral("<path d='M9 18V5l12-2v13'/><circle cx='6' cy='18' r='3'/><circle cx='18' cy='16' r='3'/>");
        case IconId::CategoryDocument:
            return QStringLiteral("<path d='M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z'/><polyline points='14 2 14 8 20 8'/><line x1='8' y1='13' x2='16' y2='13'/><line x1='8' y1='17' x2='16' y2='17'/>");
        case IconId::CategoryApp:
        case IconId::BrowserExt:
            return QStringLiteral("<path d='M16.5 9.4l-9-5.19'/><path d='M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z'/><polyline points='3.27 6.96 12 12.01 20.73 6.96'/><line x1='12' y1='22.08' x2='12' y2='12'/>");
        case IconId::CategoryImage:
            return QStringLiteral("<rect x='3' y='3' width='18' height='18' rx='2'/><circle cx='8.5' cy='8.5' r='1.5'/><polyline points='21 15 16 10 5 21'/>");
        case IconId::CategoryGeneric:
            return QStringLiteral("<path d='M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z'/><polyline points='13 2 13 9 20 9'/>");
        case IconId::LockClosed:
            return QStringLiteral("<rect x='3' y='11' width='18' height='11' rx='2' ry='2'/><path d='M7 11V7a5 5 0 0 1 10 0v4'/>");
        case IconId::KeySerial:
            return QStringLiteral("<circle cx='7.5' cy='15.5' r='5.5'/><path d='M21 2l-9.6 9.6'/><path d='M15.5 7.5l3 3L22 7l-3-3'/>");
        case IconId::BadgeVerified:
            return QStringLiteral("<path d='M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z'/>");
        case IconId::DeviceLaptop:
            return QStringLiteral("<rect x='3' y='4' width='18' height='12' rx='2'/><line x1='2' y1='20' x2='22' y2='20'/>");
        case IconId::SettingsGear:
            return QStringLiteral("<circle cx='12' cy='12' r='3'/><path d='M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z'/>");
        case IconId::ThemeSun:
            return QStringLiteral("<circle cx='12' cy='12' r='5'/><line x1='12' y1='1' x2='12' y2='3'/><line x1='12' y1='21' x2='12' y2='23'/><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'/><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'/><line x1='1' y1='12' x2='3' y2='12'/><line x1='21' y1='12' x2='23' y2='12'/><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'/><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'/>");
    }
    return QStringLiteral("<circle cx='12' cy='12' r='8'/>");
}

} // namespace idm::ui::theme
