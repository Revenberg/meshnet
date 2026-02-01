@echo off
REM MeshNet V0.8.1 - Flash all 4 ESP32-S3 devices

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set BINARY=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bin
set BOOTLOADER=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.bootloader.bin
set PARTITIONS=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.partitions.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - ESP32-S3 Multi-Device Flash
echo ================================================
echo.

REM Verify binaries
if not exist "%BINARY%" (
    echo ERROR: Binary not found: %BINARY%
    pause
    exit /b 1
)
echo Binary verified: OK
echo   - lora_node.ino.bin ^(main firmware^)
echo   - bootloader ^(for ESP32-S3^)
echo   - partitions ^(4MB layout^)
echo.

REM Flash each device  
for %%P in (COM5 COM8 COM9 COM11) do (
    echo.
    echo ===== Flashing %%P =====
    
    REM Erase
    echo [1/3] Erasing flash...
    "%ESPTOOL%" --chip esp32s3 --port %%P --before default-reset --after hard-reset erase-flash
    
    if !errorlevel! neq 0 (
        echo ERROR: Failed to erase %%P
        goto :skip_write
    )
    
    REM Write bootloader
    echo [2/3] Writing bootloader...
    "%ESPTOOL%" --chip esp32s3 --port %%P --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 8MB ^
        0x0 "%BOOTLOADER%" ^
        0x8000 "%PARTITIONS%" ^
        0x10000 "%BINARY%"
    
    if !errorlevel! equ 0 (
        echo SUCCESS: %%P flashed!
    ) else (
        echo FAILED: %%P
    )
    
    :skip_write
    timeout /t 2 /nobreak
)

echo.
echo ================================================
echo All devices flashed. Unplug and replug to verify.
echo ================================================
echo.
pause
