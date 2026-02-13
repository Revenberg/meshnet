#!/usr/bin/env powershell
# Flash MeshNet firmware to all detected ESP32-S3 devices

$ErrorActionPreference = "Continue"
$buildPath = "node/lora_node/build/esp32.esp32.heltec_wifi_lora_32_V3"
$ports = @("COM3","COM4","COM5","COM6","COM7","COM8","COM9","COM10","COM11")
$logDir = "logs"
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
}
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logPath = Join-Path $logDir "flash_all_$timestamp.log"
Start-Transcript -Path $logPath -Append | Out-Null
Write-Host "[LOG] $logPath" -ForegroundColor DarkGray

if (-not (Test-Path "$buildPath/lora_node.ino.bootloader.bin")) {
    Write-Host "[X] Build files niet gevonden: $buildPath" -ForegroundColor Red
    exit 1
}

$totalPorts = $ports.Count
$index = 0
foreach ($port in $ports) {
    $index++
    Write-Progress -Activity "Flashing ESP32-S3 devices" -Status "$port ($index/$totalPorts)" -PercentComplete ([Math]::Round(($index / $totalPorts) * 100, 0))
    Write-Host "`n=== $port ===" -ForegroundColor Cyan
    try {
        $chip = python -m esptool --chip esp32s3 --port $port --baud 57600 chip_id 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[SKIP] Geen ESP32-S3 op $port" -ForegroundColor Yellow
            continue
        }
        Write-Host "[OK] ESP32-S3 gevonden op $port" -ForegroundColor Green
    } catch {
        Write-Host "[SKIP] $port niet bereikbaar" -ForegroundColor Yellow
        continue
    }

    Write-Progress -Activity "Flashing ESP32-S3 devices" -Status "Erasing $port" -PercentComplete ([Math]::Round((($index - 0.5) / $totalPorts) * 100, 0))
    Write-Host "[ERASE] $port" -ForegroundColor Yellow
    python -m esptool --chip esp32s3 --port $port --baud 57600 erase_flash
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] Erase mislukt op $port" -ForegroundColor Red
        continue
    }

    Write-Progress -Activity "Flashing ESP32-S3 devices" -Status "Flashing $port" -PercentComplete ([Math]::Round((($index - 0.25) / $totalPorts) * 100, 0))
    Write-Host "[FLASH] $port" -ForegroundColor Yellow
    python -m esptool --chip esp32s3 --port $port --baud 57600 `
      write_flash -z `
      0x0 "$buildPath/lora_node.ino.bootloader.bin" `
      0x8000 "$buildPath/lora_node.ino.partitions.bin" `
      0x10000 "$buildPath/lora_node.ino.bin"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "[DONE] Flash OK op $port" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Flash mislukt op $port" -ForegroundColor Red
    }
}

Write-Progress -Activity "Flashing ESP32-S3 devices" -Completed
Stop-Transcript | Out-Null
Write-Host "[LOG] Completed. Log saved to $logPath" -ForegroundColor DarkGray
