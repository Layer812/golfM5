@echo off
setlocal EnableExtensions
cd /d "%~dp0"
echo BALL'S EYE GOLF R13 - INDEPENDENT BOOT / BUILD ONLY
where pio >nul 2>nul
if errorlevel 1 (echo STOP: PlatformIO CLI pio not installed. & exit /b 1)
pio run -e golf_native
if errorlevel 1 (echo STOP: native build failed. NOTHING FLASHED. & exit /b 2)
echo PASS: native golf firmware built. Nothing was flashed.
exit /b 0
