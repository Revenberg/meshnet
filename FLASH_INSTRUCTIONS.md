# MeshNet V1.0.1 Firmware Flash Instructions

## Overview
This document provides step-by-step instructions for flashing the V1.0.1 firmware to all 4 Heltec ESP32 LoRa V3 nodes.

## What's New in V1.0.1
✅ **Dynamic AP SSID**: Each node broadcasts `MeshNode-<MAC>_V1.0.1`  
✅ **Version Change Restart**: Auto-restart on new firmware version  
✅ **Serial Monitoring**: Multi-port monitor script supports version detection  

## Prerequisites
- **Arduino CLI** (v1.3.1+): `arduino-cli --version`
- **USB Cables** (4x): For programming each node
- **Computer with USB ports**
- **Node firmware source**: Located at `C:\Users\reven\Documents\Arduino\MeshNet\node\`

## Step-by-Step Instructions

### 1. Compile the Firmware

Navigate to the MeshNet project directory:
```powershell
cd C:\Users\reven\Documents\Arduino\MeshNet
```

Compile for Heltec WiFi LoRa 32 V3:
```powershell
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 node --export-binaries
```

Expected output:
```
Sketch uses XXX bytes (YY%) of program storage space
Global variables use ZZZ bytes (AA%) of dynamic memory
```

The compiled binary will be saved to:
```
C:\Users\reven\Documents\Arduino\MeshNet\node\build\esp32.esp32.heltec_wifi_lora_32_V3\node.ino.bin
```

### 2. Flash the First Node

Connect the first USB cable to the first Heltec ESP32 LoRa V3 node.

Identify the COM port:
```powershell
arduino-cli board list
```

Look for output like:
```
/dev/ttyUSB0  esp32:esp32:heltec_wifi_lora_32_V3  Heltec WiFi LoRa 32(V3)
```

Note the COM port (e.g., `/dev/ttyUSB0` or `COM3` on Windows).

Flash the firmware:
```powershell
arduino-cli upload -b esp32:esp32:heltec_wifi_lora_32_V3 -p COM3 node
```

Wait for confirmation:
```
Flashing into Flash Memory
Flash done
```

### 3. Verify Node is Running

After flashing, the node will:
1. Boot up automatically
2. Initialize LoRa radio
3. Start WiFi AP with unique SSID

To verify:
- **Check Serial Output**: Open Arduino IDE Serial Monitor at 115200 baud
- **Check WiFi**: Look for new WiFi network named `MeshNode-<MAC>_V1.0.1`

Example SSID: `MeshNode-A4C13BD12345_V1.0.1`

### 4. Repeat for Nodes 2, 3, and 4

Disconnect the first node and repeat steps 2-3 for each remaining node.

**Expected SSIDs after flashing all 4 nodes:**
```
MeshNode-<MAC1>_V1.0.1
MeshNode-<MAC2>_V1.0.1
MeshNode-<MAC3>_V1.0.1
MeshNode-<MAC4>_V1.0.1
```

## Testing the Mobile Interface

After flashing, test the mobile interface:

### Connect to Node's WiFi:
1. On your smartphone/tablet
2. WiFi Settings → Find `MeshNode-<MAC>_V1.0.1`
3. Connect (no password)

### Open Web Interface:
1. Open browser
2. Navigate to `http://192.168.3.1/`
3. You should see login page with:
   - ✅ Mobile-responsive design
   - ✅ 48px touch buttons
   - ✅ Proper viewport settings
   - ✅ Good spacing

### Test Responsiveness:
- **Portrait**: Vertical stacked layout
- **Landscape**: Multi-column layout
- **Small screens (320px)**: Still readable
- **Large screens (1024px+)**: Full width optimal

### Test Interactions:
- Tap buttons (should have visual feedback)
- Input focus (should show blue border)
- Login/Register forms (should work)
- Message sending (if users exist)

## Troubleshooting

### Issue: "Board not found" error
**Solution**: Run `arduino-cli board install esp32:esp32` to install the ESP32 board definitions.

### Issue: Compilation fails
**Solution**: 
1. Ensure all required libraries are installed
2. Check for syntax errors in `.cpp` and `.h` files
3. Verify board definition is correct: `esp32:esp32:heltec_wifi_lora_32_V3`

### Issue: Flash fails
**Solution**:
1. Check USB connection
2. Try a different USB cable
3. Put the board in bootloader mode (press BOOT + RST buttons)
4. Verify COM port is correct

### Issue: Node doesn't appear after flashing
**Solution**:
1. Check Serial Monitor for errors (115200 baud)
2. Verify power supply is adequate
3. Check WiFi AP is broadcasting (should appear in WiFi list)

## Reverting to Previous Version

If you need to rollback to V0.7.5:

```powershell
cd C:\Users\reven\Documents\Arduino
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 heltec_esp32_lora_v3_SR/heltec_lora_game/lora_node --export-binaries
arduino-cli upload -b esp32:esp32:heltec_wifi_lora_32_V3 -p COM3 heltec_esp32_lora_v3_SR/heltec_lora_game/lora_node
```

## Phase C Next Steps

After successfully flashing V1.0.1 to all 4 nodes:

1. **Test Node Connection Tracking**: Verify logins are recorded in Docker backend
2. **Implement DNS Routing**: Add DNS forwarding for easier access
3. **Implement SSO Integration**: Single Sign-On for user management

## File Locations

- **Source Code**: `C:\Users\reven\Documents\Arduino\MeshNet\node\`
- **Compiled Binary**: `C:\Users\reven\Documents\Arduino\MeshNet\node\build\esp32.esp32.heltec_wifi_lora_32_V3\node.ino.bin`
- **Board Config**: Automatically installed by Arduino CLI

## Support

For issues or questions, check:
1. [Arduino CLI Documentation](https://arduino.github.io/arduino-cli/)
2. [Heltec ESP32 Documentation](https://docs.heltec.org/)
3. Console output in Serial Monitor (115200 baud)

---

**Version**: V1.0.1  
**Board**: Heltec WiFi LoRa 32 V3  
**Created**: January 29, 2026  
**Status**: Ready for deployment
