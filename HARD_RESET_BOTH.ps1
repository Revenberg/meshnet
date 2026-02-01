#!/usr/bin/env powershell
# HARD RESET - Erase + Flash MeshNet V0.8.1
# For COM5 and COM11 with manual BOOT+RESET sequence

$ErrorActionPreference = "Continue"
$buildPath = "node/lora_node/build/esp32.esp32.heltec_wifi_lora_32_V3"

# Kleuren
Write-Host "`n[!] HARD RESET MeshNet V0.8.1`n" -ForegroundColor Cyan

# Check build files exist
if (-not (Test-Path "$buildPath/lora_node.ino.bootloader.bin")) {
    Write-Host "[X] Build files niet gevonden! Zorg dat je eerst gecompileerd hebt." -ForegroundColor Red
    exit 1
}

# Function to reset one device
function Reset-Device {
    param($port)
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "[*] HARD RESET: $port" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan
    
    Write-Host "`n[STAP 1] Voorbereiding" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Houd BEIDE knoppen vast op je $port device:"
    Write-Host "  * BOOT knop (rechts)"
    Write-Host "  * RESET knop (links)"
    Write-Host ""
    Read-Host "Klaar? Druk ENTER"
    
    Write-Host "`n[!] ERASING alles op $port..." -ForegroundColor Red
    python -m esptool --chip esp32s3 --port $port --baud 57600 erase_flash
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[OK] ERASE klaar! Nu:" -ForegroundColor Green
        Write-Host "  1. Laat RESET los (BOOT nog vasthouden!)"
        Write-Host "  2. Wacht op volgende instructie"
        Read-Host "Klaar? Druk ENTER"
        
        Write-Host "`n[*] FLASHING MeshNet V0.8.1 op $port..." -ForegroundColor Cyan
        python -m esptool --chip esp32s3 --port $port --baud 57600 `
          write_flash -z `
          0x0 "$buildPath/lora_node.ino.bootloader.bin" `
          0x8000 "$buildPath/lora_node.ino.partitions.bin" `
          0x10000 "$buildPath/lora_node.ino.bin"
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n[OK] FLASH klaar! Nu:" -ForegroundColor Green
            Write-Host "  * Laat BOOT los"
            Write-Host "  * Device start automatisch op"
            Read-Host "Klaar? Druk ENTER"
            
            Write-Host "[+] Device booted!" -ForegroundColor Green
            Write-Host "[+] $port HARD RESET SUCCES" -ForegroundColor Green
            return $true
        } else {
            Write-Host "[X] FLASH MISLUKT op $port" -ForegroundColor Red
            return $false
        }
    } else {
        Write-Host "[X] ERASE MISLUKT op $port" -ForegroundColor Red
        return $false
    }
}

# Reset beide devices
$results = @{}
$results["COM5"] = Reset-Device -port "COM5"
$results["COM11"] = Reset-Device -port "COM11"

# Summary
Write-Host "`n[SUMMARY] HARD RESET Results" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$results.GetEnumerator() | ForEach-Object {
    $status = if ($_.Value) { "[OK]" } else { "[X]" }
    $color = if ($_.Value) { "Green" } else { "Red" }
    Write-Host "$($_.Key): $status" -ForegroundColor $color
}

Write-Host "`n[Done]`n" -ForegroundColor Green
