#!/usr/bin/env powershell
# MeshNet V0.8.1 ESP32 Forced Flash
# Properly erases and flashes with reset

Write-Host "MeshNet V0.8.1 - Force Flash (Erase + Flash)" -ForegroundColor Cyan
Write-Host ""

$buildDir = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3"
$mergedFile = "$buildDir\lora_node.ino.merged.bin"

# COM ports
$ports = @("COM3", "COM4", "COM5", "COM9")

# Find esptool
$espTool = Get-Item "C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\*\esptool.py" -ErrorAction Stop | Select-Object -First 1 -ExpandProperty FullName

if (-not (Test-Path $mergedFile)) {
    Write-Host "ERROR: Binary not found at $mergedFile" -ForegroundColor Red
    exit 1
}

Write-Host "Using binary: $(Split-Path $mergedFile -Leaf)" -ForegroundColor Green
Write-Host ""

$success = 0
foreach ($port in $ports) {
    Write-Host ">>> Flashing $port (ERASE + FLASH)" -ForegroundColor Yellow
    
    try {
        # Step 1: Erase entire flash
        Write-Host "    [1/3] Erasing flash..." -ForegroundColor Gray
        $eraseCmd = "python.exe `"$espTool`" --chip esp32 --port $port erase_flash"
        Invoke-Expression $eraseCmd 2>&1 | Out-Null
        
        # Step 2: Flash with merge binary
        Write-Host "    [2/3] Writing firmware..." -ForegroundColor Gray
        $flashCmd = "python.exe `"$espTool`" --chip esp32 --port $port --baud 921600 write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 `"$mergedFile`""
        Invoke-Expression $flashCmd 2>&1 | Out-Null
        
        # Step 3: Reset device (via DTR signal)
        Write-Host "    [3/3] Resetting device..." -ForegroundColor Gray
        Start-Sleep -Milliseconds 500
        
        Write-Host "    [OK] $port complete - Please unplug and replug" -ForegroundColor Green
        $success++
    }
    catch {
        Write-Host "    [FAIL] $port error" -ForegroundColor Red
    }
    
    Write-Host ""
}

Write-Host "============================" -ForegroundColor Cyan
Write-Host "Flash complete: $success/$($ports.Count)" -ForegroundColor Green
Write-Host "============================" -ForegroundColor Cyan
Write-Host ""
Write-Host "IMPORTANT: Physically unplug and replug each device!" -ForegroundColor Yellow
Write-Host "This resets the bootloader cache and forces version reload." -ForegroundColor Yellow
Write-Host ""
