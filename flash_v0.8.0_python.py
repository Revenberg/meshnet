#!/usr/bin/env python3
import subprocess
import time
import sys

ESPTOOL = r"C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.1.0\esptool.exe"
BINARY = r"C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3\lora_node.ino.merged.bin"

def check_device(port):
    """Check if device is accessible on port"""
    try:
        result = subprocess.run(
            [ESPTOOL, "--chip", "esp32", "--port", port, "--baud", "921600", "read_mac"],
            capture_output=True,
            timeout=5,
            text=True
        )
        return result.returncode == 0
    except:
        return False

def erase_and_flash(port):
    """Erase and flash device on given port"""
    print(f"\n[{port}] Erasing flash...")
    result1 = subprocess.run(
        [ESPTOOL, "--chip", "esp32", "--port", port, "--baud", "921600", "erase-flash"],
        capture_output=True,
        timeout=30,
        text=True
    )
    
    if result1.returncode != 0:
        print(f"[{port}] ERROR: {result1.stderr}")
        return False
    
    print(f"[{port}] Writing V0.8.1...")
    result2 = subprocess.run(
        [ESPTOOL, "--chip", "esp32", "--port", port, "--baud", "921600",
         "write-flash", "--flash-mode", "dio", "--flash-freq", "80m", "--flash-size", "4MB",
         "0x0", BINARY],
        capture_output=True,
        timeout=60,
        text=True
    )
    
    if result2.returncode != 0:
        print(f"[{port}] ERROR: {result2.stderr}")
        return False
    
    print(f"[{port}] SUCCESS!")
    return True

def main():
    print("\n" + "="*50)
    print("MeshNet V0.8.1 - Python Flash Tool")
    print("="*50 + "\n")
    
    # Check available devices
    print("Checking ports...")
    devices = []
    for port in ["COM5", "COM8", "COM9", "COM11"]:
        print(f"  {port}...", end=" ", flush=True)
        if check_device(port):
            print("FOUND")
            devices.append(port)
        else:
            print("not found")
    
    if not devices:
        print("\nERROR: No devices found!")
        print("\nTroubleshooting:")
        print("  1. Plug in devices")
        print("  2. Wait 3 seconds for drivers")
        print("  3. Devices should appear in Device Manager")
        print("  4. If not, install CP210x drivers from Silicon Labs")
        input("\nPress ENTER to exit...")
        return
    
    print(f"\nFound {len(devices)} device(s): {devices}")
    print("\nFor each device, you must enter bootloader mode:")
    print("  1. HOLD LEFT button (GPIO0)")
    print("  2. Press RIGHT button (RST)")
    print("  3. Release both")
    input("\nPress ENTER to start flashing...")
    
    # Flash each device
    flashed = 0
    for i, port in enumerate(devices, 1):
        print(f"\n{'='*50}")
        print(f"Device {i} of {len(devices)}: {port}")
        print("="*50)
        print("\nEnter bootloader mode NOW:")
        print("  1. HOLD LEFT button")
        print("  2. Press RIGHT button")
        print("  3. Release both")
        input("\nPress ENTER when ready...")
        
        if erase_and_flash(port):
            flashed += 1
            print(f"\n✓ {port} flashed successfully")
        else:
            print(f"\n✗ {port} failed - try again")
        
        time.sleep(1)
    
    print(f"\n{'='*50}")
    print(f"Flashed {flashed}/{len(devices)} devices")
    print("="*50)
    print("\nNext: Unplug all devices, wait 5s, replug one by one")
    print("Check WiFi for: LoRA_XXXXXXXX_V0.8.1")
    input("\nPress ENTER to exit...")

if __name__ == "__main__":
    main()
