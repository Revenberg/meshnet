#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_PATH="$SCRIPT_DIR/.env"

info() { echo "[INFO] $*"; }
warn() { echo "[WARN] $*"; }

serial_port=""

# Prefer stable by-id symlinks when available
if compgen -G "/dev/serial/by-id/*" > /dev/null; then
  serial_port=$(ls -1 /dev/serial/by-id/* | head -n 1)
elif compgen -G "/dev/ttyUSB*" > /dev/null; then
  serial_port=$(ls -1 /dev/ttyUSB* | head -n 1)
elif compgen -G "/dev/ttyACM*" > /dev/null; then
  serial_port=$(ls -1 /dev/ttyACM* | head -n 1)
fi

if [[ -n "$serial_port" ]]; then
  info "Detected serial port: $serial_port"
else
  warn "No USB serial device detected. Set SERIAL_PORT manually in .env if needed."
fi

{
  if [[ -n "$serial_port" ]]; then
    echo "SERIAL_PORT=$serial_port"
  else
    echo "SERIAL_PORT="
  fi
  echo "SERIAL_BAUD=115200"
} > "$ENV_PATH"

info "Starting docker compose..."
cd "$SCRIPT_DIR"
docker compose up -d --build
info "docker compose started."
