#!/usr/bin/env powershell
# HARD RESET - Erase + Flash MeshNet V0.9.4 op COM5

$ErrorActionPreference = "Continue"
$buildPath = "node/lora_node/build/esp32.esp32.heltec_wifi_lora_32_V3"
$port = "COM5"

Write-Host "`n[!] HARD RESET MeshNet V0.9.4 - COM5`n" -ForegroundColor Cyan

# Check build files
if (-not (Test-Path "$buildPath/lora_node.ino.bootloader.bin")) {
    Write-Host "[X] Build files niet gevonden!" -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "[*] HARD RESET: $port" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan

Write-Host "`n[STAP 1] Voorbereiding" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Houd BEIDE knoppen vast op je $port device:"
Write-Host "  * BOOT knop (rechts)"
Write-Host "  * RESET knop (links)"
Write-Host ""
Write-Host "[+] Wachtend 2 seconden..." -ForegroundColor Yellow
Start-Sleep -Seconds 2

Write-Host "`n[!] ERASING alles op $port..." -ForegroundColor Red
python -m esptool --chip esp32s3 --port $port --baud 57600 erase_flash

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[OK] ERASE klaar!" -ForegroundColor Green
    Write-Host "  1. Laat RESET los (BOOT nog vasthouden!)"
    Write-Host "  2. Wacht op volgende instructie"
    Write-Host "[+] Wachtend 2 seconden voor ERASE..." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    
    Write-Host "`n[*] FLASHING MeshNet V0.9.4 op $port..." -ForegroundColor Cyan
    python -m esptool --chip esp32s3 --port $port --baud 57600 `
      write_flash -z `
      0x0 "$buildPath/lora_node.ino.bootloader.bin" `
      0x8000 "$buildPath/lora_node.ino.partitions.bin" `
      0x10000 "$buildPath/lora_node.ino.bin"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[OK] FLASH klaar!" -ForegroundColor Green
        Write-Host "  * Laat BOOT los"
        Write-Host "  * Device start automatisch op"
        Write-Host "[+] Wachtend 1 seconde voor BOOT..." -ForegroundColor Yellow
        Start-Sleep -Seconds 1
        
        Write-Host "[+] Device booted!" -ForegroundColor Green
        Write-Host "[+] COM5 HARD RESET SUCCES" -ForegroundColor Green
    } else {
        Write-Host "[X] FLASH MISLUKT op $port" -ForegroundColor Red
    }
} else {
    Write-Host "[X] ERASE MISLUKT op $port" -ForegroundColor Red
}

Write-Host "`n[Done]`n" -ForegroundColor Green
