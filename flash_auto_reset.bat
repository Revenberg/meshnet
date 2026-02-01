@echo off
REM MeshNet V0.8.1 - esptool with Auto Bootloader (RTS/DTR)

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - Flash with Auto-Bootloader
echo ================================================
echo.
echo BEFORE RUNNING:
echo   1. CLOSE Arduino IDE completely
echo   2. CLOSE Serial Monitor
echo   3. CLOSE any terminal with open COM port
echo.
pause

echo Checking available devices...
for %%P in (COM5 COM8 COM9 COM11) do (
    echo   %%P...
    "%ESPTOOL%" --chip esp32 --port %%P --before=default_reset --after=hard_reset read_mac 2>nul
    if !errorlevel! equ 0 echo     ^> FOUND
)

echo.
set /p PORT="Enter port number (e.g., COM5): "

if "%PORT%"=="" (
    echo ERROR: No port entered
    pause
    exit /b 1
)

echo.
echo Flashing %PORT%...
echo   Step 1: Erase flash
"%ESPTOOL%" --chip esp32 --port %PORT% --before=default_reset --after=hard_reset erase-flash

echo.
echo   Step 2: Write V0.8.1
"%ESPTOOL%" --chip esp32 --port %PORT% --before=default_reset --after=hard_reset write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"

if !errorlevel! equ 0 (
    echo.
    echo ================================================
    echo   SUCCESS!
    echo ================================================
    echo.
    echo Device on %PORT% has been flashed with V0.8.1
    echo.
    echo Wait 5-10 seconds, then check WiFi networks for:
    echo   LoRA_XXXXXXXX_V0.8.1
    echo.
) else (
    echo.
    echo ERROR: Flash failed
    echo.
    echo If you see "Connecting..." then "timeout":
    echo   - Device is in normal mode, not responding to bootloader
    echo   - Hold GPIO0 button (left) while plugging in device
    echo   - Or use Arduino IDE to flash first, then try again
)

echo.
pause
