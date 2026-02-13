#!/usr/bin/env powershell
$ErrorActionPreference = "Stop"

$board = "esp32:esp32:heltec_wifi_lora_32_V3"
$root = "C:/Users/reven/Documents/Arduino/MeshNet"
$node = "$root/node"
$lora = "$root/node/lora_node"
$logDir = "$root/logs"
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
}
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logPath = Join-Path $logDir "flash_with_timing_$timestamp.log"
Start-Transcript -Path $logPath -Append | Out-Null
Write-Host "[LOG] $logPath" -ForegroundColor DarkGray

function Step {
    param(
        [string]$Name,
        [scriptblock]$Command
    )

    Write-Host "" 
    Write-Host "=== $Name ===" -ForegroundColor Cyan
    $start = Get-Date
    Write-Host ("Start: {0}" -f $start.ToString("yyyy-MM-dd HH:mm:ss"))
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $Command
    $sw.Stop()
    $end = Get-Date
    Write-Host ("End:   {0}" -f $end.ToString("yyyy-MM-dd HH:mm:ss"))
    Write-Host ("Duration: {0}s" -f [Math]::Round($sw.Elapsed.TotalSeconds, 2))
}

Step "Build lora_node" { arduino-cli compile -b $board $lora }
Step "Build node" { arduino-cli compile -b $board $node }
Step "Upload COM6 lora_node" { arduino-cli upload -b $board -p COM6 $lora }
Step "Upload COM5 node" { arduino-cli upload -b $board -p COM5 $node }

Stop-Transcript | Out-Null
Write-Host "[LOG] Completed. Log saved to $logPath" -ForegroundColor DarkGray
