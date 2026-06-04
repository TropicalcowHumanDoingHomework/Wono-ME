@echo off
title [WouoUI Simulator Builder]

REM ====================================================================
REM          WouoUI Simulator - One-click build script
REM
REM  [Human]  Double-click this file to build.
REM  [AI]     From simulator\build directory, run:
REM              cmake --build . --config Release
REM
REM  Output:  simulator\build\Release\wouo_sim.exe
REM ====================================================================

setlocal
cd /d "%~dp0"
cd simulator

if not exist "build\CMakeCache.txt" (
    echo.
    echo [*] First-time CMake configuration ...
    if not exist build mkdir build
    cmake -B build -S . -G "Visual Studio 17 2022" >nul 2>&1
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configuration failed.
        pause
        exit /b 1
    )
)

echo.
echo [*] Building ...
echo.
cmake --build build --config Release
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo       BUILD SUCCESSFUL
echo.
echo   Output:
echo   simulator\build\Release\wouo_sim.exe
echo ========================================
echo.
pause
