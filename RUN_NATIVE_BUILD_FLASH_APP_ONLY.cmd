@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "PORT=COM16"
if not "%~1"=="" set "PORT=%~1"
echo BALL'S EYE GOLF R13 - INDEPENDENT BOOT / APP-ONLY / %PORT%
echo Standalone native application build.
echo Device must already be in ESP32-S3 ROM DOWNLOAD mode. No erase.
call "%~dp0RUN_NATIVE_BUILD_ONLY.cmd"
if errorlevel 1 (echo STOP: build failed; NOTHING FLASHED. & exit /b 2)
set "BIN=%~dp0.pio\build\golf_native\firmware.bin"
if not exist "%BIN%" (echo STOP: firmware.bin missing; NOTHING FLASHED. & exit /b 3)
where python >nul 2>nul
if errorlevel 1 (echo STOP: python not installed; NOTHING FLASHED. & exit /b 3)
python "%~dp0flash_app_only.py" "%PORT%" "%BIN%"
if errorlevel 1 (echo STOP: read-only preflight or APP-only flash failed. & exit /b 4)
echo PASS: only verified active APP slot updated. No other regions written.
echo Close serial monitor; short-press power to reboot.
exit /b 0
