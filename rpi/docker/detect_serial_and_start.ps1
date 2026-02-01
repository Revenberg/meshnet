$ErrorActionPreference = 'Stop'

function Write-Info($msg) { Write-Host "[INFO] $msg" }
function Write-Warn($msg) { Write-Host "[WARN] $msg" }
function Write-Err($msg) { Write-Host "[ERROR] $msg" }

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$envPath = Join-Path $scriptDir '.env'

# Try to detect a serial port on Windows (COM*)
$serialPort = $null
if ($IsLinux) {
  try {
    $usbPorts = @(Get-ChildItem -Path /dev/ttyUSB* -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName)
    $acmPorts = @(Get-ChildItem -Path /dev/ttyACM* -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName)
    $byIdPorts = @(Get-ChildItem -Path /dev/serial/by-id/* -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName)
    $serialPort = ($usbPorts + $acmPorts + $byIdPorts | Select-Object -First 1)
  } catch {
    Write-Warn "Failed to enumerate Linux serial ports: $($_.Exception.Message)"
  }
} elseif ($IsWindows) {
  try {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    if ($ports.Count -gt 0) {
      # Prefer higher COM numbers (often USB)
      $serialPort = ($ports | Sort-Object { [int]($_ -replace '[^0-9]','') } -Descending)[0]
      Write-Warn "Detected Windows COM port '$serialPort'. Linux containers cannot open COM ports; set SERIAL_PORT on the RPI to /dev/ttyUSB* instead."
    }
  } catch {
    Write-Warn "Failed to enumerate serial ports: $($_.Exception.Message)"
  }
}

if (-not $serialPort) {
  Write-Warn "No COM ports found. Set SERIAL_PORT manually in .env if needed."
}

# Write .env file for docker compose
$lines = @()
if ($serialPort -and $IsLinux) {
  $lines += "SERIAL_PORT=$serialPort"
} else {
  $lines += "SERIAL_PORT="
}
$lines += "SERIAL_BAUD=115200"

Set-Content -Path $envPath -Value $lines -Encoding UTF8

if ($serialPort -and $IsLinux) {
  Write-Info "Detected serial port: $serialPort"
} else {
  Write-Warn "SERIAL_PORT not set (no Linux USB serial detected)."
}

# Start docker compose
Push-Location $scriptDir
try {
  Write-Info "Starting docker compose..."
  docker compose up -d --build
  Write-Info "docker compose started."
} finally {
  Pop-Location
}
