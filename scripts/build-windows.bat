@echo off
setlocal
cd /d "%~dp0\.."

cmake -S . -B build-windows -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build build-windows --config Release --target VDX7_VST3
if errorlevel 1 exit /b 1

echo.
echo Built VST3 should be under:
echo build-windows\VDX7_artefacts\Release\VST3\VDX7.vst3
