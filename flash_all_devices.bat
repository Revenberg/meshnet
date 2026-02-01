@echo off
REM MeshNet V0.8.1 - Batch Flash All Heltec V3 Devices
REM Flashes each device with bootloader reset instructions

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin
set DEVICE_COUNT=0
set FLASHED_COUNT=0

echo.
echo ================================================
echo   MeshNet V0.8.1 - Multi-Device Flash
echo ================================================
echo.

REM Verify binary
if not exist "%MERGED_BIN%" (
    echo ERROR: Binary not found at:
    echo %MERGED_BIN%
    pause
    exit /b 1
)
echo Binary verified: OK
echo.

REM List available CP210x ports (Heltec devices)
echo Available Heltec devices:
for %%P in (COM5 COM8 COM9 COM11) do (
    "%ESPTOOL%" --chip esp32 --port %%P --baud 921600 read_mac >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [!DEVICE_COUNT!] %%P - READY
        set DEVICE[!DEVICE_COUNT!]=%%P
        set /A DEVICE_COUNT+=1
    )
)

echo.
echo Found !DEVICE_COUNT! device(s)
echo.

if !DEVICE_COUNT! equ 0 (
    echo ERROR: No devices found!
    echo.
    echo Troubleshooting:
    echo 1. Plug in one Heltec device
    echo 2. Wait 2 seconds for driver
    echo 3. Check Device Manager for "Silicon Labs CP210x"
    echo 4. Run this script again
    echo.
    pause
    exit /b 1
)

echo.
echo IMPORTANT: For each device, you MUST enter bootloader mode:
echo   1. HOLD the LEFT button (GPIO0)
    echo   2. Press RIGHT button (RST) briefly while holding LEFT
    echo   3. Release both buttons
    echo   4. Press ENTER in this window
    echo.

REM Close any open serial monitors
taskkill /F /IM putty.exe 2>nul
taskkill /F /IM miniterm.py 2>nul
timeout /t 2 /nobreak

REM Flash each device
for /L %%i in (0,1,!DEVICE_COUNT!) do (
    if defined DEVICE[%%i] (
        set PORT=!DEVICE[%%i]!
        cls
        echo.
        echo ================================================
        echo   DEVICE %%i of !DEVICE_COUNT! - PORT !PORT!
        echo ================================================
        echo.
        echo ACTION: Enter bootloader mode on device
        echo   1. HOLD LEFT button (GPIO0)
        echo   2. Press RIGHT button (RST)
        echo   3. Release both buttons
        echo.
        echo Press ENTER when device LED blinks or changes...
        echo.
        pause

        echo.
        echo [1/2] Erasing flash on !PORT!...
        "%ESPTOOL%" --chip esp32 --port !PORT! --baud 921600 erase-flash
        
        if !errorlevel! neq 0 (
            echo ERROR: Could not erase !PORT! - skipping
            timeout /t 2 /nobreak
            goto :skip_write
        )

        timeout /t 1 /nobreak

        echo.
        echo [2/2] Writing V0.8.1 to !PORT!...
        "%ESPTOOL%" --chip esp32 --port !PORT! --baud 921600 write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"
        
        if !errorlevel! equ 0 (
            echo.
            echo SUCCESS: !PORT! flashed with V0.8.1
            set /A FLASHED_COUNT+=1
        ) else (
            echo.
            echo FAILED: !PORT! - try again with bootloader mode
        )

        :skip_write
        timeout /t 2 /nobreak
    )
)

cls
echo.
echo ================================================
echo   FLASHING COMPLETE
echo ================================================
echo.
echo Flashed !FLASHED_COUNT! of !DEVICE_COUNT! devices
echo.
echo NEXT STEPS:
echo   1. Unplug all devices
echo   2. Wait 5 seconds
echo   3. Plug them back in one by one
echo   4. Check WiFi networks for: LoRA_XXXXXXXX_V0.8.1
echo   5. Connect to each and verify version in web UI
echo.
pause
