#!/usr/bin/env python3
"""
Serial Monitor for ESP32-S3 Debug Output
"""
import serial
import sys
import time

def monitor_serial(port, baudrate=115200, timeout=1):
    """Monitor serial port and print output"""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"\n[SERIAL] Listening on {port} @ {baudrate} baud")
        print("[SERIAL] Press CTRL+C to exit\n")
        print("-" * 60)
        
        try:
            while True:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"[{port}] {line}")
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n[SERIAL] Exiting...")
    except serial.SerialException as e:
        print(f"[ERROR] Could not open {port}: {e}")
        sys.exit(1)

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
    monitor_serial(port)
