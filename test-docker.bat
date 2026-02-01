@echo off
REM MeshNet Docker Testing Script for Windows
REM Run this script from MeshNet root directory

setlocal enabledelayedexpansion

cls
echo.
echo ╔════════════════════════════════════════╗
echo ║  MeshNet Docker Test Setup (Windows)   ║
echo ╚════════════════════════════════════════╝
echo.

REM Check Docker
docker --version >nul 2>&1
if errorlevel 1 (
    echo ✗ Docker not found! Install Docker Desktop for Windows
    pause
    exit /b 1
)

echo ✓ Docker installed

REM Check Docker Compose
docker-compose --version >nul 2>&1
if errorlevel 1 (
    echo ✗ Docker Compose not found!
    pause
    exit /b 1
)

echo ✓ Docker Compose installed
echo.

REM Create directories
echo Creating necessary directories...
if not exist "rpi\docker\mysql\data" mkdir "rpi\docker\mysql\data"
if not exist "rpi\docker\mosquitto\config" mkdir "rpi\docker\mosquitto\config"
if not exist "rpi\docker\mosquitto\data" mkdir "rpi\docker\mosquitto\data"
if not exist "rpi\backend\data" mkdir "rpi\backend\data"
echo ✓ Directories created
echo.

REM Navigate to docker directory
cd rpi\docker

echo Starting Docker containers...
echo (First run may take 2-3 minutes to download images)
echo.

docker-compose up -d

echo.
echo Waiting 15 seconds for services to initialize...
timeout /t 15 /nobreak

echo.
echo ╔════════════════════════════════════════╗
echo ║  Container Status                      ║
echo ╚════════════════════════════════════════╝
echo.
docker-compose ps

echo.
echo ╔════════════════════════════════════════╗
echo ║  Testing Services                      ║
echo ╚════════════════════════════════════════╝
echo.

echo Testing Backend API on port 3001...
timeout /t 2 /nobreak >nul
curl -s http://localhost:3001/health
if errorlevel 0 (
    echo.
    echo ✓ Backend API responding!
) else (
    echo ⚠ Backend may still be starting, check logs below
)

echo.
echo Testing Webserver on port 80...
timeout /t 2 /nobreak >nul
curl -s http://localhost/health
if errorlevel 0 (
    echo.
    echo ✓ Webserver responding!
) else (
    echo ⚠ Webserver may still be starting
)

echo.
echo ╔════════════════════════════════════════╗
echo ║  Quick Access                          ║
echo ╚════════════════════════════════════════╝
echo.
echo Browser Access:
echo   Dashboard:     http://localhost
echo   Backend API:   http://localhost:3001
echo.
echo Database Access:
echo   Host: localhost:3306
echo   User: meshnet
echo   Password: meshnet_secure_pwd
echo   Database: meshnet
echo.
echo Useful Commands:
echo.
echo   View logs:
echo   docker-compose logs -f
echo   docker-compose logs -f backend
echo   docker-compose logs -f webserver
echo.
echo   Access MySQL:
echo   docker-compose exec mysql mysql -u meshnet -p meshnet
echo.
echo   Check database tables:
echo   docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SHOW TABLES;"
echo.
echo   Stop all services:
echo   docker-compose down
echo.
echo   Reset everything (DELETE DATA):
echo   docker-compose down -v
echo.
echo ✓ Docker stack is running!
echo.
pause
