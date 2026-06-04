@echo off
cd /d "%~dp0"
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release
pause