@echo off
REM MeshNet V0.8.1 - ESP32 Flash Tool with Manual Bootloader Reset
REM INSTRUCTIONS: For each device, when prompted:
REM   1. Hold GPIO0 button (left button on Heltec)
REM   2. Press RST button (right button on Heltec)
REM   3. Release RST, keep holding GPIO0 for 2 more seconds
REM   4. Release GPIO0
REM   Then press ENTER

setlocal enabledelayedexpansion

set ESPTOOL=C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe
set MERGED_BIN=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - ESP32 Manual Bootloader Flash
echo ================================================
echo.

REM Verify binary exists
if not exist "%MERGED_BIN%" (
    echo ERROR: Binary not found at %MERGED_BIN%
    pause
    exit /b 1
)
echo Binary verified: OK
echo.

REM Close any open COM ports
taskkill /F /IM putty.exe 2>nul
taskkill /F /IM miniterm.py 2>nul
timeout /t 2 /nobreak

REM Flash each device
for %%P in (COM3 COM4 COM5 COM9) do (
    cls
    echo.
    echo ================================================
    echo   FLASHING %%P
    echo ================================================
    echo.
    echo INSTRUCTIONS for %%P:
    echo   1. Look at Heltec board - find two buttons
    echo   2. Press and HOLD the LEFT button (GPIO0)
    echo   3. While holding LEFT, press RIGHT button briefly (RST)
    echo   4. Release RIGHT button first
    echo   5. Keep holding LEFT for 2 more seconds
    echo   6. Release LEFT button
    echo.
    echo When device shows "Connecting..." in terminal below, you've done it right!
    echo.
    pause

    echo.
    echo [Step 1/2] Erasing flash on %%P...
    "%ESPTOOL%" --chip esp32 --port %%P --baud 921600 erase-flash
    
    if !errorlevel! neq 0 (
        echo WARNING: Erase failed on %%P - trying write anyway
    )
    
    timeout /t 1 /nobreak
    
    echo.
    echo [Step 2/2] Writing V0.8.1 to %%P...
    "%ESPTOOL%" --chip esp32 --port %%P --baud 921600 write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x0 "%MERGED_BIN%"
    
    if !errorlevel! equ 0 (
        echo.
        echo SUCCESS: %%P flashed with V0.8.1
        echo Device will restart automatically...
    ) else (
        echo.
        echo FAILED: %%P did not flash successfully
        echo Try again with manual bootloader reset
    )
    
    timeout /t 2 /nobreak
)

cls
echo.
echo ================================================
echo   FLASHING COMPLETE
echo ================================================
echo.
echo Check WiFi networks on phone/computer for:
echo   - LoRA_XXXXXXXX_V0.8.1 (4 unique networks)
echo.
echo If not appearing:
echo   1. Unplug all devices
echo   2. Wait 10 seconds
echo   3. Plug in one device at a time
echo   4. Check if WiFi network appears
echo.
pause
