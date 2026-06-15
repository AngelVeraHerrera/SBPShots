@echo off
REM =====================================================================================
REM  TopasShots - Release deploy helper.
REM
REM  Configures, builds and installs the Release preset. The CMake install step runs Qt's
REM  deployment (windeployqt) automatically and produces a self-contained folder under:
REM
REM      deploys\<triplet>\<version>\release\
REM
REM  Run this from an "x64 Native Tools Command Prompt for VS 2022" with the Qt MSVC bin
REM  directory on PATH.
REM =====================================================================================

setlocal

set PRESET=msvc2022-release
set SOURCE_DIR=%~dp0..

echo Configuring (%PRESET%)...
cmake --preset %PRESET% -S "%SOURCE_DIR%"
if errorlevel 1 (
    echo ERROR: CMake configure failed.
    exit /B 1
)

echo Building (%PRESET%)...
cmake --build --preset %PRESET%
if errorlevel 1 (
    echo ERROR: Build failed.
    exit /B 1
)

echo Installing / deploying (%PRESET%)...
cmake --build --preset %PRESET% --target install
if errorlevel 1 (
    echo ERROR: Install/deploy failed.
    exit /B 1
)

echo.
echo Deploy completed successfully. See the deploys\ directory.
echo.
endlocal
