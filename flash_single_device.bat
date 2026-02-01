@echo off
REM MeshNet V0.8.1 - Single Device Flash Tool
REM Flash one device at a time with manual bootloader entry

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin
set COM_PORT=COM3

echo.
echo ================================================
echo   MeshNet V0.8.1 - Single Device Flash
echo ================================================
echo.

REM Verify binary
if not exist "%MERGED_BIN%" (
    echo ERROR: Binary not found!
    echo Expected: %MERGED_BIN%
    pause
    exit /b 1
)
echo Binary verified: OK
echo.

REM Close any serial monitors
taskkill /F /IM putty.exe 2>nul
taskkill /F /IM miniterm.py 2>nul
timeout /t 2 /nobreak

echo.
echo BOOTLOADER MODE INSTRUCTIONS:
echo ==============================
echo 1. HOLD the LEFT button (GPIO0) on the device
echo 2. PRESS and release the RIGHT button (RST) while holding LEFT
echo 3. RELEASE both buttons
echo 4. Device LED should change or device should show "Connecting..."
echo.
pause

echo.
echo [1/2] Erasing flash on %COM_PORT%...
"%ESPTOOL%" --chip esp32 --port %COM_PORT% --baud 921600 erase-flash

if !errorlevel! neq 0 (
    echo.
    echo ERROR: Could not connect to device!
    echo Try again with:
    echo   1. Device plugged in
    echo   2. GPIO0 held during erase
    echo.
    pause
    exit /b 1
)

echo SUCCESS: Flash erased
timeout /t 2 /nobreak

echo.
echo [2/2] Writing V0.8.1 to %COM_PORT%...
"%ESPTOOL%" --chip esp32 --port %COM_PORT% --baud 921600 write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"

if !errorlevel! equ 0 (
    echo.
    echo ================================================
    echo   SUCCESS! Device flashed with V0.8.1
    echo ================================================
    echo.
    echo Device will restart automatically
    echo Look for WiFi: LoRA_XXXXXXXX_V0.8.1
    echo.
) else (
    echo.
    echo ERROR: Flash failed!
    echo Try again - make sure GPIO0 is held during the process
    echo.
)

pause
