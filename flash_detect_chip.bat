@echo off
REM MeshNet V0.8.1 - Flash with Chip Auto-Detect

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - Auto-Detect Chip
echo ================================================
echo.

set /p PORT="Enter COM port (COM5/COM8/COM9/COM11): "

if "%PORT%"=="" (
    echo ERROR: No port entered
    pause
    exit /b 1
)

echo.
echo Detecting chip on %PORT%...
"%ESPTOOL%" --port %PORT% chip_id >nul 2>&1

if !errorlevel! equ 0 (
    echo   ^> Found ESP32 (original)
    set CHIP=esp32
) else (
    echo   ^> Not standard ESP32, trying ESP32-S3...
    "%ESPTOOL%" --chip esp32s3 --port %PORT% chip_id >nul 2>&1
    if !errorlevel! equ 0 (
        echo   ^> Found ESP32-S3
        set CHIP=esp32s3
    ) else (
        echo ERROR: Could not detect chip
        pause
        exit /b 1
    )
)

echo.
echo Chip detected: !CHIP!
echo.

REM Flash based on detected chip
if "!CHIP!"=="esp32s3" (
    echo Note: Device is ESP32-S3, not standard ESP32
    echo This binary may not be compatible!
    echo.
    echo Continuing anyway...
    echo.
    "%ESPTOOL%" --chip esp32s3 --port %PORT% --before default-reset --after hard-reset erase-flash
    "%ESPTOOL%" --chip esp32s3 --port %PORT% --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"
) else (
    echo Erasing %PORT%...
    "%ESPTOOL%" --chip esp32 --port %PORT% --before default-reset --after hard-reset erase-flash
    
    echo.
    echo Writing V0.8.1 to %PORT%...
    "%ESPTOOL%" --chip esp32 --port %PORT% --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"
)

if !errorlevel! equ 0 (
    echo.
    echo SUCCESS!
) else (
    echo.
    echo ERROR: Flash failed
)

echo.
pause
