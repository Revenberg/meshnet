@echo off
REM MeshNet V0.8.1 - Auto-detect and Flash Single Device

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - Auto-detect Device
echo ================================================
echo.

REM Verify binary
if not exist "%MERGED_BIN%" (
    echo ERROR: Binary not found!
    pause
    exit /b 1
)

REM Try each CP210x device (COM5, COM8, COM9, COM11)
set FOUND=0
for %%P in (COM5 COM8 COM9 COM11) do (
    if !FOUND! equ 0 (
        echo Trying %%P...
        "%ESPTOOL%" --chip esp32 --port %%P --baud 921600 read_mac >nul 2>&1
        if !errorlevel! equ 0 (
            echo FOUND device on %%P!
            set DEVICE_PORT=%%P
            set FOUND=1
        )
    )
)

if !FOUND! equ 0 (
    echo.
    echo ERROR: No device found!
    echo.
    echo Try these steps:
    echo 1. Unplug device and replug
    echo 2. Check Device Manager ^(Win+X^)
    echo 3. Look for "Silicon Labs CP210x" device
    echo 4. Run this script again
    echo.
    pause
    exit /b 1
)

echo.
echo Device found on !DEVICE_PORT!
echo.
pause

REM Close serial monitors
taskkill /F /IM putty.exe 2>nul
taskkill /F /IM miniterm.py 2>nul
timeout /t 2 /nobreak

echo.
echo BOOTLOADER MODE:
echo Hold LEFT button ^(GPIO0^), press RIGHT button ^(RST^), release both
echo.
pause

echo.
echo [1/2] Erasing flash on !DEVICE_PORT!...
"%ESPTOOL%" --chip esp32 --port !DEVICE_PORT! --baud 921600 erase-flash

if !errorlevel! neq 0 (
    echo.
    echo ERROR: Erase failed - try bootloader mode again
    echo.
    pause
    exit /b 1
)

timeout /t 2 /nobreak

echo.
echo [2/2] Writing V0.8.1 to !DEVICE_PORT!...
"%ESPTOOL%" --chip esp32 --port !DEVICE_PORT! --baud 921600 write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"

if !errorlevel! equ 0 (
    echo.
    echo ================================================
    echo   SUCCESS! Device on !DEVICE_PORT! flashed with V0.8.1
    echo ================================================
    echo.
    echo Check WiFi for: LoRA_XXXXXXXX_V0.8.1
    echo.
) else (
    echo.
    echo ERROR: Flash failed - try bootloader mode again
    echo.
)

pause
