#pragma once

#include "core/Types.h"
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace idm::ui::theme {

enum class IconId : int {
    AddUrl = 0,
    BatchSequence,
    StartPlay,
    Pause,
    StopSquare,
    StopAll,
    TrashDelete,
    SchedulerClock,
    SpeedGauge,
    FolderOpen,
    SearchLens,
    ShieldVerify,
    TerminalLog,
    NetworkGlobe,
    CpuThreads,
    HardDrive,
    QueueMain,
    QueueNight,
    CategoryAll,
    CategoryArchive,
    CategoryVideo,
    CategoryAudio,
    CategoryDocument,
    CategoryApp,
    CategoryImage,
    CategoryGeneric,
    LockClosed,
    KeySerial,
    CpuChip,
    BadgeVerified,
    DeviceLaptop,
    SettingsGear,
    ThemeSun,
    ThemeMoon,
    YoutubePlay,
    BrowserExt
};

enum class IconState : int {
    Normal = 0,
    Hover,
    Active,
    Disabled,
    Success,
    Warning,
    Danger
};

class IconRegistry final {
public:
    IconRegistry() = delete;

    [[nodiscard]] static QPixmap pixmap(
        IconId id,
        int sizePx = 16,
        IconState state = IconState::Normal
    );

    [[nodiscard]] static QPixmap pixmapColored(
        IconId id,
        int sizePx,
        const QColor& strokeColor
    );

    [[nodiscard]] static QIcon icon(IconId id, int sizePx = 16);
    [[nodiscard]] static IconId iconForCategory(core::FileCategory category) noexcept;
    [[nodiscard]] static QColor colorForState(IconState state) noexcept;
    [[nodiscard]] static QColor semanticColorForIcon(IconId id) noexcept;

private:
    [[nodiscard]] static QString svgBodyForIcon(IconId id);
};

} // namespace idm::ui::theme
