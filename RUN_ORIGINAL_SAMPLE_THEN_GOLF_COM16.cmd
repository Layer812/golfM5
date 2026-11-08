@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "PORT=COM16"
if not "%~1"=="" set "PORT=%~1"
echo BALL'S EYE GOLF - ONLY AFTER ORIGINAL board.cpp:82 drawString
echo Original sample code/settings preserved. COM port: %PORT%
where git >nul 2>nul
if errorlevel 1 (echo STOP: git not installed; NOTHING FLASHED. & exit /b 1)
where pio >nul 2>nul
if errorlevel 1 (echo STOP: pio not installed; NOTHING FLASHED. & exit /b 1)
where python >nul 2>nul
if errorlevel 1 (echo STOP: python not installed; NOTHING FLASHED. & exit /b 1)
python "%~dp0stage_original_hook.py"
if errorlevel 1 (echo SAFE STOP: staging failed; NOTHING FLASHED. & exit /b 2)
echo BUILD: original 00_smoke platformio.ini + two-line hook + existing golf sources
pio run -d "%~dp0golf_build_stage" -e 00_smoke
if errorlevel 1 (echo STOP: build failed; NOTHING FLASHED. & exit /b 3)
set "BIN=%~dp0golf_build_stage\.pio\build\00_smoke\firmware.bin"
if not exist "%BIN%" (echo STOP: BIN missing; NOTHING FLASHED. & exit /b 3)
echo APP-ONLY: same verified flash_app_only.py as visible original sample
echo StopWatch must be in ROM download mode, with serial monitors closed.
python "%~dp0flash_app_only.py" "%PORT%" "%BIN%"
if errorlevel 1 (echo STOP: preflight or flash failed; NO OTHER WRITES. & exit /b 4)
echo PASS: only existing selected APP updated. No other flash partitions changed.
echo Close serial monitors; short-press power button to boot.
exit /b 0
