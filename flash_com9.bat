@echo off
set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set BINARY=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bin
set BOOTLOADER=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bootloader.bin
set PARTITIONS=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.partitions.bin

echo Flashing COM9...
echo [1/2] Erasing...
"%ESPTOOL%" --chip esp32s3 --port COM9 --before default-reset --after hard-reset erase-flash

echo [2/2] Writing V0.8.1...
"%ESPTOOL%" --chip esp32s3 --port COM9 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 8MB ^
    0x0 "%BOOTLOADER%" ^
    0x8000 "%PARTITIONS%" ^
    0x10000 "%BINARY%"

echo.
if %ERRORLEVEL% equ 0 (
    echo SUCCESS!
) else (
    echo FAILED - check if device is plugged in
)
pause
