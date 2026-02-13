#!/usr/bin/env python3
"""
Monitor all available serial ports and highlight firmware version logs.
"""
import argparse
import sys
import threading
import time
from datetime import datetime

import serial
from serial.tools import list_ports


def format_line(port, line):
    timestamp = datetime.now().isoformat(timespec="seconds")
    return f"[{timestamp}] [{port}] {line}"


def is_version_line(line, expected_version):
    if not expected_version:
        return False
    lower = line.lower()
    return expected_version.lower() in lower and ("version" in lower or "firmware" in lower)


def monitor_port(port, baudrate, timeout, log_handle, expected_version, stop_event):
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"\n[SERIAL] Listening on {port} @ {baudrate} baud")
        while not stop_event.is_set():
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    formatted = format_line(port, line)
                    print(formatted)
                    if is_version_line(line, expected_version):
                        print(format_line(port, f"[VERSION] Detected {expected_version}"))
                    if log_handle:
                        log_handle.write(formatted + "\n")
                        if is_version_line(line, expected_version):
                            log_handle.write(format_line(port, f"[VERSION] Detected {expected_version}") + "\n")
                        log_handle.flush()
            time.sleep(0.01)
    except serial.SerialException as e:
        print(f"[ERROR] Could not open {port}: {e}")
    finally:
        try:
            if ser and ser.is_open:
                ser.close()
        except Exception:
            pass


def main():
    parser = argparse.ArgumentParser(description="Monitor all ESP32 serial ports")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--timeout", type=int, default=1, help="Serial timeout")
    parser.add_argument("--log-file", default=None, help="Optional log file path")
    parser.add_argument("--expected-version", default=None, help="Expected firmware version string")
    parser.add_argument(
        "--max-seconds",
        type=int,
        default=120,
        help="Maximum runtime in seconds before stopping (default: 120)",
    )
    args = parser.parse_args()

    ports = [p.device for p in list_ports.comports()]
    if not ports:
        print("[ERROR] No serial ports found.")
        sys.exit(1)

    print("[SERIAL] Ports found:")
    for port in ports:
        print(f"  - {port}")

    log_handle = open(args.log_file, "a", encoding="utf-8") if args.log_file else None
    stop_event = threading.Event()

    threads = []
    try:
        for port in ports:
            t = threading.Thread(
                target=monitor_port,
                args=(port, args.baud, args.timeout, log_handle, args.expected_version, stop_event),
                daemon=True,
            )
            t.start()
            threads.append(t)

        print("\n[SERIAL] Press CTRL+C to exit\n")
        start_time = time.monotonic()
        while not stop_event.is_set():
            if args.max_seconds > 0 and (time.monotonic() - start_time) >= args.max_seconds:
                print(f"\n[SERIAL] Max runtime reached ({args.max_seconds}s). Stopping...")
                stop_event.set()
                break
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[SERIAL] Exiting...")
        stop_event.set()
        time.sleep(0.2)
    finally:
        stop_event.set()
        if log_handle:
            log_handle.close()
    sys.exit(0)


if __name__ == "__main__":
    main()
