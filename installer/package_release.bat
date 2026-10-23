@echo off
setlocal enabledelayedexpansion
echo ============================================================================
echo   IDM Vibe v2.0.0 - C++20 / Qt6 Release Builder ^& Windeployqt Packager
echo ============================================================================

cmake -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1

where windeployqt >nul 2>nul
if %errorlevel% equ 0 (
    echo Running windeployqt on build\Release\IdmVibe.exe...
    windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler build\Release\IdmVibe.exe
)

xcopy /E /I /Y chrome-extension build\Release\chrome-extension >nul

where iscc >nul 2>nul
if %errorlevel% equ 0 (
    echo Compiling Inno Setup 6 Installer...
    iscc installer\setup.iss
) else (
    echo [INFO] Inno Setup (iscc.exe) not in PATH; portable package is ready in build\Release
)

echo [SUCCESS] Release build and packaging complete.
