@echo off
echo [DEPRECATED] Gebruik FLASH_ALL_CONNECTED.ps1 of scripts\flash_with_timing.ps1
exit /b 1
REM MeshNet V0.8.1 Flash Script - FIXED VERSION
setlocal enabledelayedexpansion

echo.
echo ================================================
echo   MeshNet V0.8.1 - ESP32 Flash Tool
echo ================================================
echo.

set BUILD_DIR=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3
set MERGED_BIN=%BUILD_DIR%\lora_node.ino.merged.bin
set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe

if not exist "%MERGED_BIN%" (
    echo ERROR: Binary not found
    pause
    exit /b 1
)

echo Binary verified: OK
echo.

for %%P in (COM3 COM4 COM5 COM9) do (
    echo.
    echo ===== Flashing %%P =====
    echo [1/2] Erasing...
    "%ESPTOOL%" --chip esp32 --port %%P erase_flash
    echo [2/2] Writing V0.8.1...
    "%ESPTOOL%" --chip esp32 --port %%P --baud 921600 write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 "%MERGED_BIN%"
    echo Done: %%P - UNPLUG and REPLUG this device!
    timeout /t 2 /nobreak
)

echo.
echo ================================================
echo   ALL FLASHING COMPLETE
echo ================================================
echo.
echo REQUIRED: Physically unplug each device, wait 5s, replug
echo Then check WiFi for: LoRA_XXXXXXX_V0.8.1
echo.
pause
