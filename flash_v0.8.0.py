#!/usr/bin/env python3
"""
MeshNet V0.8.1 Flash Script with NVRAM Wipe
Uses esptool to erase flash and properly flash new firmware
"""

import subprocess
import sys
import os
import time

BUILD_DIR = r"C:\Users\reven\Documents\Arduino\MeshNet\node\lora_node\build\esp32.esp32.heltec_wifi_lora_32_V3"
MERGED_BIN = os.path.join(BUILD_DIR, "lora_node.ino.merged.bin")
BOOTLOADER_BIN = os.path.join(BUILD_DIR, "lora_node.ino.bootloader.bin")
PARTITIONS_BIN = os.path.join(BUILD_DIR, "lora_node.ino.partitions.bin")

PORTS = ["COM3", "COM4", "COM5", "COM9"]

def find_esptool():
    """Find esptool.py in Arduino installation"""
    pattern = r"C:\Users\reven\AppData\Local\Arduino15\packages\esp32\tools\esptool_py"
    for root, dirs, files in os.walk(pattern.replace("*", "")):
        if "esptool.py" in files:
            return os.path.join(root, "esptool.py")
    raise FileNotFoundError("esptool.py not found")

def flash_device(port, esptool_path):
    """Flash a device with complete erase cycle"""
    print(f"\n{'='*60}")
    print(f"Flashing {port}")
    print('='*60)
    
    try:
        # Step 1: Full erase
        print("[1/4] Erasing entire flash memory...")
        cmd = ["python", esptool_path, "--chip", "esp32", "--port", port, "erase_flash"]
        subprocess.run(cmd, check=True, capture_output=True)
        print("      ✓ Flash erased")
        
        # Step 2: Write bootloader
        print("[2/4] Writing bootloader...")
        cmd = ["python", esptool_path, "--chip", "esp32", "--port", port, 
               "--baud", "921600", "write_flash", "-z", 
               "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "detect",
               "0x0", BOOTLOADER_BIN]
        subprocess.run(cmd, check=True, capture_output=True)
        print("      ✓ Bootloader written")
        
        # Step 3: Write partitions
        print("[3/4] Writing partition table...")
        cmd = ["python", esptool_path, "--chip", "esp32", "--port", port,
               "--baud", "921600", "write_flash", "-z",
               "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "detect",
               "0x8000", PARTITIONS_BIN]
        subprocess.run(cmd, check=True, capture_output=True)
        print("      ✓ Partitions written")
        
        # Step 4: Write application
        print("[4/4] Writing application firmware V0.8.1...")
        cmd = ["python", esptool_path, "--chip", "esp32", "--port", port,
               "--baud", "921600", "write_flash", "-z",
               "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "detect",
               "0x10000", MERGED_BIN]
        subprocess.run(cmd, check=True, capture_output=True)
        print("      ✓ Firmware written")
        
        print(f"\n✓✓✓ {port} FLASHED SUCCESSFULLY ✓✓✓")
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"\n✗✗✗ {port} FAILED: {e} ✗✗✗")
        return False

def main():
    print("\n" + "="*60)
    print("  MeshNet V0.8.1 - Forced Flash Script")
    print("="*60 + "\n")
    
    # Find esptool
    try:
        esptool = find_esptool()
        print(f"Found esptool: {esptool}\n")
    except FileNotFoundError as e:
        print(f"ERROR: {e}")
        sys.exit(1)
    
    # Verify binary files
    for f in [MERGED_BIN, BOOTLOADER_BIN, PARTITIONS_BIN]:
        if not os.path.exists(f):
            print(f"ERROR: Missing {f}")
            sys.exit(1)
    
    print("Binary files verified ✓\n")
    
    # Flash all ports
    results = {}
    for port in PORTS:
        results[port] = flash_device(port, esptool)
        time.sleep(1)  # Wait between devices
    
    # Summary
    print("\n" + "="*60)
    print("FLASH SUMMARY")
    print("="*60)
    success = sum(1 for v in results.values() if v)
    for port, result in results.items():
        status = "✓ OK" if result else "✗ FAIL"
        print(f"  {port}: {status}")
    
    print(f"\nSuccess: {success}/{len(PORTS)}")
    print("\n" + "="*60)
    print("IMPORTANT: Physically unplug and replug each device!")
    print("This resets the bootloader and forces new version load.")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
