@echo off
cd /d "%~dp0"
echo Building in: %CD%
if exist build rmdir /s /q build
cmake -B build -S . -G "Visual Studio 17 2022"
if %ERRORLEVEL% neq 0 (
    echo CMAKE CONFIGURE FAILED
    exit /b %ERRORLEVEL%
)
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo CMAKE BUILD FAILED
    exit /b %ERRORLEVEL%
)
echo BUILD SUCCEEDED
