@echo off
REM Flash COM5 with retry tolerance

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set BINARY=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bin
set BOOTLOADER=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bootloader.bin
set PARTITIONS=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.partitions.bin

echo.
echo Flashing COM5 (with reset tolerance)...
echo.

REM Step 1: Erase (device will reset after this)
echo [1/2] Erasing flash...
"%ESPTOOL%" --chip esp32s3 --port COM5 --before default-reset --after hard-reset erase-flash
echo.

REM Wait for device to boot back up
echo Waiting for device to restart... (10 seconds)
timeout /t 10 /nobreak

REM Step 2: Write (should work now)
echo.
echo [2/2] Writing V0.8.1...
"%ESPTOOL%" --chip esp32s3 --port COM5 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 8MB ^
    0x0 "%BOOTLOADER%" ^
    0x8000 "%PARTITIONS%" ^
    0x10000 "%BINARY%"

if !errorlevel! equ 0 (
    echo.
    echo ================================================
    echo SUCCESS! Device flashed with V0.8.1
    echo ================================================
    echo.
    echo Next: Unplug device, wait 5 seconds, plug back in
    echo Look for WiFi: LoRA_XXXXXXXX_V0.8.1
) else (
    echo.
    echo ERROR: Flash still failed after reset
    echo Try: Unplug device for 30 seconds, then retry
)

echo.
pause
