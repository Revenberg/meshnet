#!/usr/bin/env python3
"""
Serial Monitor for ESP32-S3 Debug Output
"""
import argparse
import serial
import sys
import time
from datetime import datetime


def format_line(port, line):
    timestamp = datetime.now().isoformat(timespec="seconds")
    return f"[{timestamp}] [{port}] {line}"


def monitor_serial(port, baudrate=115200, timeout=1, log_file=None, max_seconds=120):
    """Monitor serial port and print output"""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"\n[SERIAL] Listening on {port} @ {baudrate} baud")
        print("[SERIAL] Press CTRL+C to exit\n")
        print("-" * 60)

        log_handle = open(log_file, "a", encoding="utf-8") if log_file else None

        start_time = time.monotonic()
        try:
            while True:
                if max_seconds > 0 and (time.monotonic() - start_time) >= max_seconds:
                    print(f"\n[SERIAL] Max runtime reached ({max_seconds}s). Stopping...")
                    break
                if ser.in_waiting:
                    line = ser.readline().decode("utf-8", errors="ignore").strip()
                    if line:
                        formatted = format_line(port, line)
                        print(formatted)
                        if log_handle:
                            log_handle.write(formatted + "\n")
                            log_handle.flush()
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n[SERIAL] Exiting...")
        finally:
            if log_handle:
                log_handle.close()
        sys.exit(0)
    except serial.SerialException as e:
        print(f"[ERROR] Could not open {port}: {e}")
        sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Serial monitor for ESP32-S3 output")
    parser.add_argument("--port", default="COM5", help="Serial port (e.g. COM5)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--log-file", default=None, help="Optional log file path")
    parser.add_argument(
        "--max-seconds",
        type=int,
        default=120,
        help="Maximum runtime in seconds before stopping (default: 120)",
    )
    args = parser.parse_args()

    monitor_serial(args.port, baudrate=args.baud, log_file=args.log_file, max_seconds=args.max_seconds)
