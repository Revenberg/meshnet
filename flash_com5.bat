@echo off
REM Flash COM5 (currently connected device)

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set BINARY=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bin
set BOOTLOADER=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bootloader.bin
set PARTITIONS=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.partitions.bin

echo.
echo Flashing COM5...
echo [1/2] Erasing...
"%ESPTOOL%" --chip esp32s3 --port COM5 --before default-reset --after hard-reset erase-flash

if !errorlevel! neq 0 (
    echo ERROR: Could not connect to COM5
    pause
    exit /b 1
)

echo.
echo [2/2] Writing V0.8.1...
"%ESPTOOL%" --chip esp32s3 --port COM5 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 8MB ^
    0x0 "%BOOTLOADER%" ^
    0x8000 "%PARTITIONS%" ^
    0x10000 "%BINARY%"

if !errorlevel! equ 0 (
    echo.
    echo SUCCESS! Device flashed.
    echo Unplug, wait 5 seconds, plug back in.
    echo Check WiFi for: LoRA_XXXXXXXX_V0.8.1
) else (
    echo.
    echo ERROR: Flash failed
)

pause
