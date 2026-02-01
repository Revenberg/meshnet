@echo off
REM Read serial debug output from device on COM5

echo Reading debug output from COM5...
echo Press CTRL+C to stop.
echo.

python -m serial.tools.miniterm COM5 115200

pause
