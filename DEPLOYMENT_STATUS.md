# MeshNet V1.0.1 - Deployment Status

## ✅ Update Feb 13, 2026 (MeshNet V4.0.0)

**Firmware Version:** MeshNet V4.0.0  
**Build Date:** Feb 13, 2026  
**Build Path:** `C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\`

### Flashed Devices (V4.0.0)
| Port | Device | Status |
|------|--------|--------|
| COM5 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:9f:bd:ac) | ✅ Flashed OK |
| COM6 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:ed:11:94) | ✅ Flashed OK |

### Startup Verification (Serial)
- **COM5**: Version 4.0.0 detected, AP `MeshNet-9E139E9FBDAC_V4.0.0`
- **COM6**: Version 4.0.0 detected, AP `MeshNet-9E139EED1194_V4.0.0`

### RPI Deployment (No sudo)
```
ssh copilot@ghostnet "cd /home/copilot/meshnet; docker compose -f rpi/docker/docker-compose.yml --project-directory rpi/docker up -d"
```

### RPI USB Heltec Flash (V4.0.0)
```
ssh copilot@ghostnet "python3 -m esptool --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 /home/copilot/meshnet/rpi/lora_node_v4.0.0.merged.bin"
```

### Test Results
- **API test suite:** ✅ 13/13 passed (includes nodes count > 0)
- **Sync scope:** ✅ pages per real node = 13


## ✅ Update Feb 7, 2026 (MeshNet V1.7.1)

**Firmware Version:** MeshNet V1.7.1  
**Build Date:** Feb 7, 2026  
**Build Path:** `C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\`

### Flashed Devices (V1.7.1)
| Port | Device | Status |
|------|--------|--------|
| COM5 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:9f:bd:ac) | ✅ Flashed OK |
| COM6 | Heltec WiFi LoRa 32 V3 (MAC: 9c:13:9e:ed:11:94) | ✅ Flashed OK |

### Startup Verification (Serial)
- **COM5**: Version 1.7.1 detected, Users=0, Pages=2, Setup OK
- **COM6**: Version 1.7.1 detected, Users=1, Pages=2, Setup OK

### RPI Deployment (No sudo)
```
ssh copilot@ghostnet "cd /home/copilot/meshnet; docker compose -f rpi/docker/docker-compose.yml up -d"
```

**RPI Container Status:** All services running (backend, lora-gateway, caddy, mqtt, mysql, web)

## ✅ Build Status: SUCCESS

**Firmware Version:** MeshNet V1.0.1  
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
# MeshNet V1.0.1 - Deployment Status

## ✅ Build Status: SUCCESS

**Firmware Version:** MeshNet V1.0.1  
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

### Flashed Devices (1.0.0)
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

### Option 0: RPI Heltec Flash (SSH, no sudo)
```
ssh copilot@ghostnet "python3 -m esptool --chip esp32s3 --port /dev/ttyUSB0 --baud 57600 erase_flash"
ssh copilot@ghostnet "python3 -m esptool --chip esp32s3 --port /dev/ttyUSB0 --baud 57600 write_flash -z \
    0x0 /home/copilot/meshnet/firmware/lora_node.ino.bootloader.bin \
    0x8000 /home/copilot/meshnet/firmware/lora_node.ino.partitions.bin \
    0x10000 /home/copilot/meshnet/firmware/lora_node.ino.bin"
```

### Option 1: Auto Flash (All Connected Devices)
```
cd C:\Users\reven\Documents\Arduino\MeshNet
powershell -ExecutionPolicy Bypass -File FLASH_ALL_CONNECTED.ps1
```

This script will:
1. ✅ Auto-detect ESP32-S3 devices on all COM ports
2. ✅ Erase flash memory
3. ✅ Write MeshNet V1.0.1 firmware
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
║  🚀 MeshNet V1.0.1 - STARTING UP      ║
╚════════════════════════════════════════╝

[VERSION] Firmware Details:
[VERSION]   Name: MeshNet
[VERSION]   Version: 1.0.1
[VERSION] ✓ MeshNet V1.0.1 confirmed

[DEBUG] Soft AP started: MeshNode-<MAC>_V1.0.1
[INFO] Async Webserver gestart
[LoRa] Init OK, node = LoRA_<MAC>
```

### WiFi Connection
- **SSID:** `MeshNode-<MAC>_V1.0.1`
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

### All Ports (single terminal)
```
python serial_monitor_all.py --baud 115200 --expected-version 1.0.1 --log-file logs\serial_all.log
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
**Status:** 1.0.1 flashed to COM5 and COM7  
**Notes:** Continue with end-to-end tests and log any failures.

````
4. Verify via serial monitor or WiFi connection
