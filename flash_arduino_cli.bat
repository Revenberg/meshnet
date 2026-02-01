@echo off
REM MeshNet V0.8.1 - Arduino CLI Flash (more reliable)
REM Uses Arduino CLI which auto-handles bootloader mode

setlocal enabledelayedexpansion

set ARDUINO_CLI="%ProgramFiles%\Arduino\arduino-cli.exe"
set BOARD=esp32:esp32:heltec_wifi_lora_32_V3
set BINARY=C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin

echo.
echo ================================================
echo   MeshNet V0.8.1 - Arduino CLI Flash
echo ================================================
echo.

REM Check if Arduino CLI exists
if not exist %ARDUINO_CLI% (
    echo ERROR: Arduino CLI not found at %ARDUINO_CLI%
    echo.
    echo Install from: https://arduino.github.io/arduino-cli/latest/installation/
    echo Or use: winget install ArduinoProject.ArduinoCLI
    echo.
    pause
    exit /b 1
)

REM List available ports
echo Available COM ports:
%ARDUINO_CLI% board list

echo.
echo.
echo Which port has your device? (COM5, COM8, COM9, COM11)
set /p PORT="Enter port (e.g., COM5): "

echo.
echo Flashing %BOARD% on %PORT% with %BINARY%
echo.
%ARDUINO_CLI% upload -p %PORT% --fqbn %BOARD% --input-file "%BINARY%" -v

if !errorlevel! equ 0 (
    echo.
    echo SUCCESS: Device flashed!
    echo.
    echo Check WiFi for: LoRA_XXXXXXXX_V0.8.1
) else (
    echo.
    echo ERROR: Flash failed
    echo.
    echo Troubleshooting:
    echo 1. Make sure port is correct
    echo 2. Unplug/replug device
    echo 3. Try again
)

echo.
pause
