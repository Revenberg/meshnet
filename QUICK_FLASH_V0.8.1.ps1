# MeshNet V0.8.1 Quick Flashing Script
# Automatically detects and flashes all available ESP32 devices

$buildDir = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build"
$baud = 57600

Write-Host "╔════════════════════════════════════════════════╗"
Write-Host "║  🚀 MeshNet V0.8.1 - MULTI-DEVICE FLASHING   ║"
Write-Host "╚════════════════════════════════════════════════╝"
Write-Host ""

# Verify build exists
if (-not (Test-Path "$buildDir\lora_node.ino.bin")) {
    Write-Host "❌ ERROR: Build not found at $buildDir"
    Write-Host "   Run: arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3"
    exit 1
}

Write-Host "Build: Found"
Write-Host "  - bootloader.bin: $(Get-Item "$buildDir\lora_node.ino.bootloader.bin" -ErrorAction SilentlyContinue | % Length) bytes"
Write-Host "  - partitions.bin: $(Get-Item "$buildDir\lora_node.ino.partitions.bin" -ErrorAction SilentlyContinue | % Length) bytes"
Write-Host "  - app.bin: $(Get-Item "$buildDir\lora_node.ino.bin" -ErrorAction SilentlyContinue | % Length) bytes"
Write-Host ""

# Test each port
$flashedDevices = @()
for ($portNum = 1; $portNum -le 15; $portNum++) {
    $port = "COM$portNum"
    
    Write-Host -NoNewline "Checking $port... "
    
    # Read full chip_id output
    $chipOutput = python -m esptool --chip auto --port $port --baud $baud chip_id 2>&1 | Out-String
    
    # Parse response
    if ($chipOutput -match "Chip is (ESP32[^\r\n]*)" -and $chipOutput -notmatch "A fatal error") {
        $chipName = $matches[1]
        Write-Host "✅ FOUND: $chipName"
        
        Write-Host ""
        Write-Host "┌────────────────────────────────────────┐"
        Write-Host "│ Device Information                     │"
        Write-Host "├────────────────────────────────────────┤"
        
        # Extract and show device details
        if ($chipOutput -match "Chip is ([^\r\n]+)") {
            Write-Host "│ Chip: $($matches[1])" 
        }
        if ($chipOutput -match "MAC: (.+)") {
            Write-Host "│ MAC:  $($matches[1])"
        }
        if ($chipOutput -match "Features:([^\r\n]+)") {
            Write-Host "│ Features: $($matches[1].Trim())"
        }
        if ($chipOutput -match "Flash: ([0-9]+)") {
            Write-Host "│ Flash: $($matches[1]) MB"
        }
        if ($chipOutput -match "Revision: (v[0-9.]+)") {
            Write-Host "│ Revision: $($matches[1])"
        }
        
        Write-Host "└────────────────────────────────────────┘"
        Write-Host ""
        
        # Confirm before flashing
        Write-Host "⚠️  Ready to flash MeshNet V0.8.1 to $port"
        Write-Host "   This will erase the current firmware."
        $confirm = Read-Host "   Proceed? (y/n)"
        
        if ($confirm -ne "y" -and $confirm -ne "Y") {
            Write-Host "   ⏭️  Skipped."
            Write-Host ""
            continue
        }
        
        Write-Host ""
        Write-Host "   [1/3] Erasing flash..."
        $eraseOutput = python -m esptool --chip auto --port $port --baud $baud erase_flash 2>&1 | Out-String
        if ($eraseOutput -match "Chip erase completed") {
            Write-Host "   ✅ Erase successful"
        } else {
            Write-Host "   ❌ Erase failed!"
            Write-Host "   $eraseOutput"
            Write-Host ""
            continue
        }
        
        Write-Host "   [2/3] Writing firmware..."
        $writeOutput = python -m esptool --chip auto --port $port --baud $baud `
            write_flash --flash_mode dio --flash_size keep `
            0x0 "$buildDir\lora_node.ino.bootloader.bin" `
            0x8000 "$buildDir\lora_node.ino.partitions.bin" `
            0x10000 "$buildDir\lora_node.ino.bin" 2>&1 | Out-String
        
        if ($writeOutput -match "Hash of data verified") {
            Write-Host "   ✅ Write successful"
        } else {
            Write-Host "   ❌ Write may have failed!"
            Write-Host "   Check output above for errors"
            Write-Host ""
            continue
        }
        
        Write-Host "   [3/3] Verifying boot..."
        Start-Sleep -Seconds 3
        
        try {
            $p = New-Object System.IO.Ports.SerialPort($port, 115200)
            $p.Open()
            $p.DiscardInBuffer()
            $boot = ""
            $sw = [Diagnostics.Stopwatch]::StartNew()
            
            while ($sw.ElapsedMilliseconds -lt 10000) {
                if ($p.BytesToRead -gt 0) {
                    $c = [char]$p.ReadChar()
                    $boot += $c
                    if ($boot -match "Setup complete") { break }
                }
                Start-Sleep -Milliseconds 5
            }
            $p.Close()
            
            if ($boot -match "MeshNet V0.8.1 confirmed") {
                Write-Host "   ✅ Boot verified - MeshNet V0.8.1 running!"
                $flashedDevices += $port
            } elseif ($boot -match "STARTING UP") {
                Write-Host "   ✅ Boot detected (MeshNet starting)"
                $flashedDevices += $port
            } else {
                Write-Host "   ⚠️  Boot output detected but version unclear"
                Write-Host "   Device may still be OK - check serial output"
            }
        } catch {
            Write-Host "   ⚠️  Could not read boot output (port busy)"
            Write-Host "   Flash likely succeeded"
            $flashedDevices += $port
        }
        
        Write-Host ""
    } else {
        Write-Host "✗ (not ESP32 or no device)"
    }
}

Write-Host "╔════════════════════════════════════════════════╗"
Write-Host "║               FLASH COMPLETE                  ║"
Write-Host "╚════════════════════════════════════════════════╝"
Write-Host ""

if ($flashedDevices.Count -eq 0) {
    Write-Host "❌ No ESP32 devices found or flashing failed"
    Write-Host ""
    Write-Host "Troubleshooting:"
    Write-Host "  1. Check USB cable connection"
    Write-Host "  2. Check Device Manager for COM port"
    Write-Host "  3. Try different USB port on computer"
    Write-Host "  4. Restart device (power cycle)"
} else {
    Write-Host "✅ Successfully flashed:"
    foreach ($dev in $flashedDevices) {
        Write-Host "   ✓ $dev"
    }
    Write-Host ""
    Write-Host "Next: Connect WiFi SSID 'MeshNode-<MAC>_V1.0.1'"
    Write-Host "      Login: test / test"
    Write-Host "      Web UI: 192.168.4.1"
}

Write-Host ""
