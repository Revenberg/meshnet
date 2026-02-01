#!/usr/bin/env powershell
# MeshNet V0.8.1 ESP32 Flash Script

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MeshNet V0.8.1 ESP32 Flash Deploy  " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$buildDir = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3"
$binFile = "$buildDir\lora_node.ino.bin"
$bootloaderFile = "$buildDir\lora_node.ino.bootloader.bin"
$partitionsFile = "$buildDir\lora_node.ino.partitions.bin"
$mergedFile = "$buildDir\lora_node.ino.merged.bin"

$comPorts = @("COM3", "COM4", "COM5", "COM9")

Write-Host "Configuration:" -ForegroundColor Green
Write-Host "  Ports: $($comPorts -join ', ')"
Write-Host "  Binary: $(Split-Path $binFile -Leaf)"
Write-Host ""

# Verify files
$filesOk = @($binFile, $bootloaderFile, $partitionsFile, $mergedFile) | Where-Object {Test-Path $_} | Measure-Object | Select-Object -ExpandProperty Count
if ($filesOk -ne 4) {
    Write-Host "ERROR: Missing binary files" -ForegroundColor Red
    exit 1
}

Write-Host "All files verified - starting flash..." -ForegroundColor Green
Write-Host ""

# Find esptool
$espToolPath = Get-Item "C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\*\esptool.py" -ErrorAction Stop | Select-Object -First 1 -ExpandProperty FullName

$successCount = 0

foreach ($port in $comPorts) {
    Write-Host ">> Flashing $port..." -ForegroundColor Yellow
    
    try {
        # Erase
        python.exe $espToolPath --chip esp32 --port $port erase_flash 2>&1 | Out-Null
        
        # Write merged binary (easiest way)
        python.exe $espToolPath --chip esp32 --port $port --baud 460800 write_flash `
            -z --flash_mode dio --flash_freq 80m --flash_size detect `
            0x0 $mergedFile 2>&1 | Out-Null
        
        Write-Host "   [OK] $port flashed successfully" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "   [FAIL] $port - Error occurred" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Flash Summary: $successCount/4 succeeded" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($successCount -eq 4) {
    Write-Host "SUCCESS! Reconnect devices to see unique SSIDs." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some devices failed. Check USB connections." -ForegroundColor Yellow
    exit 1
}
