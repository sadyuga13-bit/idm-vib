cmake_minimum_required(VERSION 3.22)

project(IdmVibe
    VERSION 1.0.0
    DESCRIPTION "High-Performance Native C++20 Multi-Segment Download Manager with Cryptographic Anti-Crack Subsystem"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Svg
)

find_package(OpenSSL QUIET)

set(PROJECT_SOURCES
    src/main.cpp

    # Core
    src/core/Types.h
    src/core/SpeedLimiter.h
    src/core/SpeedLimiter.cpp
    src/core/HashVerifier.h
    src/core/HashVerifier.cpp
    src/core/BatchExpander.h
    src/core/BatchExpander.cpp

    # Storage
    src/storage/DatabaseManager.h
    src/storage/DatabaseManager.cpp
    src/storage/DiskWriter.h
    src/storage/DiskWriter.cpp
    src/storage/CategoryRouter.h
    src/storage/CategoryRouter.cpp

    # Security & Anti-Crack Subsystem
    src/security/SecurityTypes.h
    src/security/ObfuscatedConstants.h
    src/security/CryptoEngine.h
    src/security/CryptoEngine.cpp
    src/security/HwidEngine.h
    src/security/HwidEngine.cpp
    src/security/SecureVault.h
    src/security/SecureVault.cpp
    src/security/IntegrityGuard.h
    src/security/IntegrityGuard.cpp
    src/security/SecureAuthClient.h
    src/security/SecureAuthClient.cpp
    src/security/LicenseCoordinator.h
    src/security/LicenseCoordinator.cpp

    # Network
    src/network/RequestProfile.h
    src/network/UrlProber.h
    src/network/UrlProber.cpp
    src/network/SegmentWorker.h
    src/network/SegmentWorker.cpp
    src/network/DownloadTask.h
    src/network/DownloadTask.cpp
    src/network/QueueScheduler.h
    src/network/QueueScheduler.cpp

    # IPC
    src/ipc/BrowserIpcServer.h
    src/ipc/BrowserIpcServer.cpp
    src/ipc/ClipboardWatcher.h
    src/ipc/ClipboardWatcher.cpp

    # UI - Theme
    src/ui/theme/ThemeTokens.h
    src/ui/theme/AppStyleSheet.h
    src/ui/theme/AppStyleSheet.cpp
    src/ui/theme/IconRegistry.h
    src/ui/theme/IconRegistry.cpp

    # UI - Models
    src/ui/models/DownloadTableModel.h
    src/ui/models/DownloadTableModel.cpp
    src/ui/models/SegmentTableModel.h
    src/ui/models/SegmentTableModel.cpp

    # UI - Delegates
    src/ui/delegates/DownloadItemDelegate.h
    src/ui/delegates/DownloadItemDelegate.cpp

    # UI - Widgets
    src/ui/widgets/TopToolbar.h
    src/ui/widgets/TopToolbar.cpp
    src/ui/widgets/SidebarTree.h
    src/ui/widgets/SidebarTree.cpp
    src/ui/widgets/InspectorDock.h
    src/ui/widgets/InspectorDock.cpp
    src/ui/widgets/SegmentMapWidget.h
    src/ui/widgets/SegmentMapWidget.cpp
    src/ui/widgets/SpeedGraphWidget.h
    src/ui/widgets/SpeedGraphWidget.cpp

    # UI - Dialogs
    src/ui/dialogs/AddDownloadDialog.h
    src/ui/dialogs/AddDownloadDialog.cpp
    src/ui/dialogs/BatchDownloadDialog.h
    src/ui/dialogs/BatchDownloadDialog.cpp

    # UI - Security & Licensing
    src/ui/security/SerialKeyInputWidget.h
    src/ui/security/SerialKeyInputWidget.cpp
    src/ui/security/LicenseStatusBadge.h
    src/ui/security/LicenseStatusBadge.cpp
    src/ui/security/SeatManagerWidget.h
    src/ui/security/SeatManagerWidget.cpp
    src/ui/security/LicenseDialog.h
    src/ui/security/LicenseDialog.cpp

    # UI - Main Window
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

qt_add_executable(IdmVibe
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(IdmVibe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(IdmVibe PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(IdmVibe PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(IdmVibe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(IdmVibe PRIVATE
        crypt32
        bcrypt
        advapi32
    )
endif()

if(OpenSSL_FOUND)
    target_link_libraries(IdmVibe PRIVATE OpenSSL::Crypto)
endif()

set_target_properties(IdmVibe PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

qt_finalize_executable(IdmVibe)
