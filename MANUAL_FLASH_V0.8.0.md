# MeshNet V0.8.1 Flash Instructions - Manual Method
# Use this if script/terminal issues occur

## Quick Method: Drag & Drop via Arduino IDE

1. Open Arduino IDE
2. Go to: Tools > Board > esp32 > Heltec WiFi LoRa 32(V3)
3. Set Configuration:
   - Upload Speed: 921600
   - Core Debug Level: None
   - USB CDC On Boot: Enabled
   - Partition Scheme: Default with SPIFFS

4. For each COM port (COM3, COM4, COM5, COM9):
   a. Select Tools > Port > [PORT]
   b. Click Sketch > Upload Using Programmer
   c. Wait for "Leaving... hard resetting via RTS pin"
   d. Device will show "LoRA_<MAC>_V0.8.1" WiFi SSID after reset

## Advanced Method: Command Line (if Terminal works)

```powershell
# Find esptool path
$esptool = Get-Item "C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\*\esptool.py" | Select-Object -First 1 -ExpandProperty FullName

# Binary location
$merged = "C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin"

# For each port (COM3, COM4, COM5, COM9):
$port = "COM3"

# Erase
python $esptool --chip esp32 --port $port erase_flash

# Flash
python $esptool --chip esp32 --port $port --baud 921600 write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0 $merged
```

## Verification Steps

1. **Unplug and replug USB cable** on each device
2. Wait 5 seconds for boot
3. On your computer/phone, check WiFi networks
4. Look for SSIDs matching: `LoRA_XXXXXXXXXX_V0.8.1`
   - Each of 4 devices should have UNIQUE SSID
   - Format: LoRA_<12HexChars>_V0.8.1

5. Connect to one SSID
6. Open browser to: http://192.168.3.1/
7. Verify page shows V0.8.1 in title or footer

## If Still on V0.7.5

Common causes:
1. **Device cached old version in NVRAM**
   - Solution: Fully power down device for 30 seconds
   - Disconnect USB, battery (if present)
   - Reconnect and try again

2. **Bootloader not updated**
   - Solution: Use full 4-part flash (bootloader + partitions + app)
   - Not just merged binary

3. **Flash didn't complete**
   - Look for: "Leaving... hard resetting via RTS pin"
   - If missing, flash incomplete

## Binary Files Location

All files in:
`C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\`

Files:
- `lora_node.ino.merged.bin` - Full flash (easiest)
- `lora_node.ino.bootloader.bin` - Bootloader only
- `lora_node.ino.partitions.bin` - Partition table
- `lora_node.ino.bin` - Application only

## Success Indicators

✓ Device reboots (LED flashes)
✓ Serial output shows "V0.8.1"  
✓ WiFi SSID is unique format: LoRA_<MAC>_V0.8.1
✓ Web interface at 192.168.3.1 shows V0.8.1

## Contact

If issues persist:
1. Check Arduino IDE: Tools > Get Board Info
2. Verify port is correct (should show "Heltec WiFi LoRa 32(V3)")
3. Try different USB cable or port
4. Try pre-compiled .bin from web flasher

---

Date: 2026-01-29
Version: V0.8.1
Target: Heltec WiFi LoRa 32 V3
