# ESP32 V0.8.1 Flash Options

## Problem
Devices still showing V0.7.5 instead of V0.8.1

## Solutions (in order of ease)

### Option 1: Batch File (EASIEST) ✓
```cmd
cd C:\Users\reven\Documents\Arduino\MeshNet
flash_v0.8.1.bat
```
- Windows Batch script
- No PowerShell issues
- Automatically flashes all 4 devices
- **RECOMMENDED**

### Option 2: Manual Commands
```powershell
$esptool = "C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.7\esptool.py"
$bin = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin"

# For each port
python $esptool --chip esp32 --port COM3 erase_flash
python $esptool --chip esp32 --port COM3 --baud 921600 write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 $bin

# Repeat for COM4, COM5, COM9
```

### Option 3: Arduino IDE GUI
1. File > Open > `C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node`
2. Tools > Board > esp32 > Heltec WiFi LoRa 32(V3)
3. Tools > Port > COM3
4. Click Sketch > Upload Using Programmer
5. Repeat for each COM port

### Option 4: Manual Instructions
See: `MANUAL_FLASH_V0.8.1.md`

---

## After Flashing

1. **Physically unplug USB cable** from device
2. Wait 5 seconds
3. **Replug USB cable**
4. Look for WiFi SSID: `LoRA_<12HexChars>_V0.8.1`
5. Connect to SSID
6. Open: `http://192.168.3.1/`
7. Verify V0.8.1 displayed in web interface

---

## Success Indicators

✓ Device shows boot message in serial monitor with V0.8.1
✓ WiFi SSID is unique (different for each of 4 devices)
✓ Web interface loads at 192.168.3.1
✓ Version "V0.8.1" visible in page

---

## Files Available

- `flash_v0.8.1.bat` - Windows Batch (BEST)
- `force_flash_v0.8.1.ps1` - PowerShell script
- `flash_v0.8.1.py` - Python script
- `MANUAL_FLASH_V0.8.1.md` - Manual instructions

---

**RECOMMENDATION**: Run the batch file!
```cmd
flash_v0.8.1.bat
```

Then physically unplug/replug each device and verify new version shows up.
