# MeshNet V0.8.1 - Deployment Status

## ✅ Build Status: SUCCESS

**Firmware Version:** MeshNet V0.8.1  
**Build Date:** January 30, 2026  
**Build Path:** `C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\`

### Build Artifacts
```
lora_node.ino.bootloader.bin     20 KB  (bootloader)
lora_node.ino.partitions.bin      3 KB  (partition table)
lora_node.ino.bin            1,310 KB  (application firmware)
```

### Code Changes Applied
✅ **version.h** - Version macros added:
```cpp
````markdown
# MeshNet V0.9.4 - Deployment Status

## ✅ Build Status: SUCCESS

**Firmware Version:** MeshNet V0.9.4  
**Build Date:** January 31, 2026  
**Build Path:** `C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\`

### Build Artifacts
```
lora_node.ino.bootloader.bin     20 KB  (bootloader)
lora_node.ino.partitions.bin      3 KB  (partition table)
lora_node.ino.bin            1,360 KB  (application firmware)
```

---

## 🔌 Hardware Status

### Flashed Devices (0.9.4)
| Port | Device | Status |
|------|--------|--------|
| COM5 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:e9:a1:74) | ✅ Flashed OK |
| COM7 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:ed:11:94) | ✅ Flashed OK |

### Skipped Devices
| Port | Device | Status |
|------|--------|--------|
| COM3 | Bluetooth Serial | N/A (not ESP32) |
| COM4 | Bluetooth Serial | N/A (not ESP32) |

---

## 🚀 Deployment Options

### Option 1: Auto Flash (All Connected Devices)
```
cd C:\Users\reven\Documents\Arduino\MeshNet
powershell -ExecutionPolicy Bypass -File FLASH_ALL_CONNECTED.ps1
```

This script will:
1. ✅ Auto-detect ESP32-S3 devices on all COM ports
2. ✅ Erase flash memory
3. ✅ Write MeshNet V0.9.4 firmware
4. ✅ Verify boot sequence

### Option 2: Manual Flash (Single Device)
```
$port = "COM5"  # Replace with your port
$buildDir = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build"

# Erase
python -m esptool --chip esp32s3 --port $port --baud 57600 erase_flash

# Write
python -m esptool --chip esp32s3 --port $port --baud 57600 `
    write_flash --flash_mode dio --flash_size 8MB `
    0x0 "$buildDir\lora_node.ino.bootloader.bin" `
    0x8000 "$buildDir\lora_node.ino.partitions.bin" `
    0x10000 "$buildDir\lora_node.ino.bin"
```

---

## 🌐 Verification After Flash

### Expected Boot Output
```
╔════════════════════════════════════════╗
║  🚀 MeshNet V0.9.4 - STARTING UP      ║
╚════════════════════════════════════════╝

[VERSION] Firmware Details:
[VERSION]   Name: MeshNet
[VERSION]   Version: 0.9.4
[VERSION] ✓ MeshNet V0.9.4 confirmed

[DEBUG] Soft AP started: MeshNet V0.9.4
[INFO] Async Webserver gestart
[LoRa] Init OK, node = LoRA_<MAC>_0.9.4
```

### WiFi Connection
- **SSID:** `MeshNet V0.9.4`
- **Password:** (empty/open)
- **Web UI:** `http://192.168.3.1`

---

## 🧪 Test Checklist
Use the current checklist in TEST_CHECKLIST.md.

### Automated Tests
- lora-gateway: `npm test` (serialConfig tests passed)

---

## ⚠️ Current Issues
- lora-gateway reports: "Serial not connected, skipping transmission" or "Operation not permitted".
    - Ensure the gateway Heltec is connected to the RPI USB port.
    - On RPI/Linux, verify the device is visible as /dev/ttyUSB* and that the container has /dev mounted read/write and explicit device access (devices mapping).
    - Restart lora-gateway after connecting the device.
    - Optional override: set SERIAL_PORT and SERIAL_BAUD in the lora-gateway environment.
    - docker-compose exposes SERIAL_PORT/SERIAL_BAUD for lora-gateway.

---

## 🖥️ Laptop Serial Logging (Monitoring)
Use the laptop to read serial logs from the Heltec nodes while the RPI acts as LoRa gateway.

### Single Port (COM5/COM7)
```
python serial_monitor.py --port COM5 --baud 115200 --log-file logs\serial_COM5.log
```

### Two Ports (separate terminals)
```
python serial_monitor.py --port COM5 --baud 115200 --log-file logs\serial_COM5.log
python serial_monitor.py --port COM7 --baud 115200 --log-file logs\serial_COM7.log
```

---

## 📊 Build Compilation Details

**Compiler:** Arduino CLI v1.0.0  
**Board:** Heltec WiFi LoRa 32 (V3) - esp32:esp32:heltec_wifi_lora_32_V3  
**MCU:** ESP32-S3 (240 MHz, 8 MB Flash)  
**Sketch Size:** 1,360,144 bytes (41% of 3.3 MB)  
**Global Variables:** 61,000 bytes (19% of 327,680 bytes)

---

## 🔧 Build Commands

### Recompile if needed
```
cd C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3
```

### Clean rebuild
```
rm -r build -Force -ErrorAction SilentlyContinue
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3
```

---

**Last Updated:** Jan 31, 2026  
**Status:** 0.9.4 flashed to COM5 and COM7  
**Notes:** Continue with end-to-end tests and log any failures.

````
4. Verify via serial monitor or WiFi connection
