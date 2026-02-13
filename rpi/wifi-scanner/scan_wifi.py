#!/usr/bin/env python3
import os
import shutil
import subprocess
import time
from datetime import datetime
from urllib.request import urlopen
from urllib.error import URLError, HTTPError

def log(message):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {message}")

def run_cmd(cmd, timeout=15):
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return result.returncode, result.stdout.strip(), result.stderr.strip()
    except Exception as exc:
        return 1, "", str(exc)

def get_interfaces():
    code, out, err = run_cmd(["iw", "dev"])
    if code != 0:
        log(f"[WARN] iw dev failed: {err or out}")
        return []
    interfaces = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Interface "):
            interfaces.append(line.split("Interface ", 1)[1].strip())
    return interfaces

def nmcli_available():
    return shutil.which("nmcli") is not None

def scan_nmcli():
    code, out, err = run_cmd(["nmcli", "-t", "-f", "SSID", "dev", "wifi", "list"], timeout=30)
    if code != 0:
        log(f"[WARN] nmcli scan failed: {err or out}")
        return []
    ssids = []
    for line in out.splitlines():
        ssid = line.strip()
        if ssid:
            ssids.append(ssid)
    return ssids

def scan_iw(iface):
    code, out, err = run_cmd(["iw", iface, "scan"], timeout=30)
    if code != 0:
        log(f"[WARN] iw scan failed on {iface}: {err or out}")
        return []
    ssids = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("SSID:"):
            ssid = line.split("SSID:", 1)[1].strip()
            if ssid:
                ssids.append(ssid)
    return ssids

def get_active_connection(iface):
    if not nmcli_available():
        return None
    code, out, err = run_cmd(["nmcli", "-t", "-f", "NAME,DEVICE", "con", "show", "--active"])
    if code != 0:
        log(f"[WARN] nmcli active connections failed: {err or out}")
        return None
    for line in out.splitlines():
        if not line.strip():
            continue
        name, device = (line.split(":", 1) + [""])[:2]
        if device == iface:
            return name
    return None

def connect_ssid(ssid, iface, password):
    if not nmcli_available():
        log("[WARN] nmcli not available; cannot connect to APs")
        return False
    cmd = ["nmcli", "--wait", "15", "dev", "wifi", "connect", ssid]
    if iface:
        cmd += ["ifname", iface]
    if password:
        cmd += ["password", password]
    code, out, err = run_cmd(cmd, timeout=20)
    if code != 0:
        log(f"[WARN] connect failed for {ssid}: {err or out}")
        return False
    log(f"[OK] connected to {ssid}")
    return True

def disconnect_iface(iface):
    if not nmcli_available() or not iface:
        return
    run_cmd(["nmcli", "dev", "disconnect", iface])

def restore_connection(conn_name):
    if not nmcli_available() or not conn_name:
        return
    run_cmd(["nmcli", "con", "up", conn_name])

def check_paths(ap_ip, paths):
    for path in paths:
        url = f"http://{ap_ip}{path}"
        try:
            with urlopen(url, timeout=6) as resp:
                log(f"[CHECK] {url} -> {resp.status}")
        except HTTPError as exc:
            log(f"[CHECK] {url} -> HTTP {exc.code}")
        except URLError as exc:
            log(f"[CHECK] {url} -> ERROR {exc.reason}")

def main():
    ap_prefix = os.getenv("AP_PREFIX", "MeshNet-")
    ap_password = os.getenv("AP_PASSWORD", "")
    ap_ip = os.getenv("AP_IP", "192.168.3.1")
    scan_interval = int(os.getenv("SCAN_INTERVAL_S", "120"))
    enable_connect = os.getenv("ENABLE_CONNECT", "true").lower() in ("1", "true", "yes")
    wifi_iface = os.getenv("WIFI_IFACE", "").strip()
    check_paths_env = os.getenv("CHECK_PATHS", "/")
    check_paths_list = [p.strip() for p in check_paths_env.split(",") if p.strip()]

    log("[START] WiFi scanner container starting")
    log(f"[CONFIG] prefix={ap_prefix} ip={ap_ip} interval={scan_interval}s connect={enable_connect}")

    while True:
        ifaces = [wifi_iface] if wifi_iface else get_interfaces()
        if not ifaces:
            log("[WARN] No WiFi interfaces found")
        ssids = []
        if nmcli_available():
            ssids = scan_nmcli()
        elif ifaces:
            ssids = scan_iw(ifaces[0])
        targets = sorted({s for s in ssids if s.startswith(ap_prefix)})
        log(f"[SCAN] Found {len(targets)} target APs: {', '.join(targets) if targets else '-'}")

        if enable_connect and targets and ifaces:
            iface = ifaces[0]
            previous_conn = get_active_connection(iface)
            for ssid in targets:
                if connect_ssid(ssid, iface, ap_password):
                    time.sleep(3)
                    check_paths(ap_ip, check_paths_list)
                    disconnect_iface(iface)
                    time.sleep(2)
            if previous_conn:
                restore_connection(previous_conn)
        time.sleep(scan_interval)

if __name__ == "__main__":
    main()
