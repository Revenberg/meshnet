# PowerShell Test Script for MeshNet Node Web Hosting API
# Usage: .\test-node-api.ps1

Write-Host "MeshNet Node Web Hosting API Test Suite" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# Node IDs (hardcoded from setup)
$NODE_1 = "72d67530-dac6-4666-885c-160cb36579ee"
$NODE_2 = "26b80c3a-a7e2-4634-957a-51f7b777de72"
$NODE_3 = "d1ec1f02-0e0b-4763-94d5-984e93c11bde"

$apiHost = $env:MESHNET_API_HOST
if (-not $apiHost) {
    $apiHost = "http://localhost:3001"
}

try {
    $health = Invoke-WebRequest -UseBasicParsing -Uri "$apiHost/health" -TimeoutSec 2
    if ($health.StatusCode -ne 200) {
        throw "Health check failed"
    }
} catch {
    $apiHost = "http://ghostnet:3001"
}

$API_BASE = "$apiHost/api/host"
$API_CORE = "$apiHost/api"

$test_count = 0
$pass_count = 0
$fail_count = 0

function Run-Test {
    param(
        [string]$name,
        [string]$url,
        [int]$expectedCode = 200,
        [string]$method = "GET",
        [string]$body = $null
    )

    $script:test_count += 1
    Write-Host -NoNewline "Test $($script:test_count): $name ... "

    try {
        $params = @{
            Uri = $url
            Method = $method
            UseBasicParsing = $true
            ErrorAction = "SilentlyContinue"
        }

        if ($body) {
            $params['Body'] = $body
            $params['ContentType'] = "application/json"
        }

        $response = Invoke-WebRequest @params
        $httpCode = $response.StatusCode

        if ($httpCode -eq $expectedCode) {
            Write-Host "PASS" -ForegroundColor Green -NoNewline
            Write-Host " (HTTP $httpCode)"
            $script:pass_count += 1
            return $response
        }
        else {
            Write-Host "FAIL" -ForegroundColor Red -NoNewline
            Write-Host " (Expected $expectedCode, got $httpCode)"
            $script:fail_count += 1
            return $null
        }
    }
    catch {
        Write-Host "FAIL" -ForegroundColor Red -NoNewline
        Write-Host " (Error: $($_.Exception.Message))"
        $script:fail_count += 1
        return $null
    }
}

function Invoke-ApiPost {
    param(
        [string]$url,
        [object]$body
    )

    return Invoke-RestMethod -Uri $url -Method Post -ContentType "application/json" -Body ($body | ConvertTo-Json -Depth 6)
}

function Get-ApiJson {
    param(
        [string]$url
    )

    return Invoke-RestMethod -Uri $url -Method Get
}

# =====================
# Test Data Setup
# =====================
Write-Host "Setup: Create 10 teams, users, virtual nodes, and pages" -ForegroundColor Cyan
Write-Host ""

$teamNames = 1..10 | ForEach-Object { "Team $($_.ToString('00'))" }
$createdTeams = @()

# Preload existing data to avoid duplicate creates
$existingGroups = @{}
$existingUsers = @{}
$existingNodes = @{}
try {
    $groupList = Get-ApiJson "$API_CORE/groups"
    foreach ($g in $groupList) { $existingGroups[$g.name] = $true }
} catch { }
try {
    $userList = Get-ApiJson "$API_CORE/users"
    foreach ($u in $userList) { $existingUsers[$u.username] = $true }
} catch { }
try {
    $nodeList = Get-ApiJson "$API_CORE/nodes"
    foreach ($n in $nodeList) { $existingNodes[$n.nodeId] = $true }
} catch { }

foreach ($team in $teamNames) {
    if ($existingGroups.ContainsKey($team)) {
        Write-Host "  SKIP Team exists: $team" -ForegroundColor DarkGray
        continue
    }
    try {
        Invoke-ApiPost "$API_CORE/groups" @{ name = $team; description = "Auto test team $team"; permissions = @() } | Out-Null
        $createdTeams += $team
        Write-Host "  OK Team created: $team" -ForegroundColor Green
    } catch {
        Write-Host "  WARN Team create failed (may already exist): $team" -ForegroundColor Yellow
    }
}

# Refresh groups to get IDs
$groups = Get-ApiJson "$API_CORE/groups"
$groupMap = @{}
foreach ($g in $groups) {
    $groupMap[$g.name] = $g.id
}

# Create 3-8 users per team
$totalNewUsers = 0
foreach ($team in $teamNames) {
    if (-not $groupMap.ContainsKey($team)) { continue }
    $count = Get-Random -Minimum 3 -Maximum 9
    for ($i = 1; $i -le $count; $i++) {
        $username = ("{0}_user{1:00}" -f ($team -replace '\s','').ToLower(), $i)
        if ($existingUsers.ContainsKey($username)) {
            Write-Host "  SKIP User exists: $username" -ForegroundColor DarkGray
            continue
        }
        try {
            Invoke-ApiPost "$API_CORE/users" @{ username = $username; password = "test123"; groupId = $groupMap[$team] } | Out-Null
            $totalNewUsers++
            Write-Host "  OK User created: $username" -ForegroundColor Green
        } catch {
            Write-Host "  WARN User create failed (may exist): $username" -ForegroundColor Yellow
        }
    }
}

# Create 10 virtual nodes
for ($i = 1; $i -le 10; $i++) {
    $nodeId = "VIRTUAL_NODE_{0:00}" -f $i
    $mac = "00:00:00:00:10:{0}" -f ($i.ToString('X2'))
    if ($existingNodes.ContainsKey($nodeId)) {
        Write-Host "  SKIP Virtual node exists: $nodeId" -ForegroundColor DarkGray
        continue
    }
    try {
        Invoke-ApiPost "$API_CORE/nodes" @{ nodeId = $nodeId; macAddress = $mac; functionalName = "Virtual Node $i"; version = "virtual" } | Out-Null
        Write-Host "  OK Virtual node created: $nodeId" -ForegroundColor Green
    } catch {
        Write-Host "  WARN Virtual node create failed (may exist): $nodeId" -ForegroundColor Yellow
    }
}

# Ensure pages for all groups/nodes
try {
    Invoke-ApiPost "$API_CORE/pages/ensure-defaults" @{} | Out-Null
    Write-Host "  OK Default pages ensured" -ForegroundColor Green
} catch {
    Write-Host "  FAIL Failed to ensure default pages" -ForegroundColor Red
}

Write-Host ""

# Test: List pages for nodes
Write-Host "Test: Get Pages for Nodes" -ForegroundColor Cyan
Write-Host ""

$response = Run-Test "List pages for MeshNode-1" `
    "$API_BASE/node/$NODE_1/pages" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Pages found: $($json.pages.Count)" -ForegroundColor Gray
}
Write-Host ""

$response = Run-Test "List pages for MeshNode-2" `
    "$API_BASE/node/$NODE_2/pages" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Pages found: $($json.pages.Count)" -ForegroundColor Gray
}
Write-Host ""

$response = Run-Test "List pages for MeshNode-3" `
    "$API_BASE/node/$NODE_3/pages" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Pages found: $($json.pages.Count)" -ForegroundColor Gray
}
Write-Host ""

# Test: Get page content
Write-Host "Test: Get Page Content" -ForegroundColor Cyan
Write-Host ""

# Get first page ID from Node 1
$listResponse = Invoke-WebRequest -Uri "$API_BASE/node/$NODE_1/pages" -UseBasicParsing
$json = $listResponse.Content | ConvertFrom-Json
if ($json.pages.Count -gt 0) {
    $PAGE_ID = $json.pages[0].pageId

    $response = Run-Test "Get page HTML content" `
        "$API_BASE/node/$NODE_1/pages/$PAGE_ID" 200
    if ($response) {
        Write-Host "  Content length: $($response.Content.Length) bytes" -ForegroundColor Gray
    }
    Write-Host ""

    Run-Test "Get page as JSON" `
        "$API_BASE/node/$NODE_1/pages/$PAGE_ID/json" 200
}
Write-Host ""

# Test: Node information
Write-Host "Test: Node Information" -ForegroundColor Cyan
Write-Host ""

$response = Run-Test "Get MeshNode-1 info" `
    "$API_BASE/node/$NODE_1/info" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Name: $($json.functionalName)" -ForegroundColor Gray
    Write-Host "  MAC: $($json.macAddress)" -ForegroundColor Gray
    Write-Host "  Status: $(if($json.isActive) { 'Online' } else { 'Offline' })" -ForegroundColor Gray
}
Write-Host ""

$response = Run-Test "Get MeshNode-2 info" `
    "$API_BASE/node/$NODE_2/info" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Name: $($json.functionalName)" -ForegroundColor Gray
}
Write-Host ""

$response = Run-Test "Get MeshNode-3 info" `
    "$API_BASE/node/$NODE_3/info" 200
if ($response) {
    $json = $response.Content | ConvertFrom-Json
    Write-Host "  Name: $($json.functionalName)" -ForegroundColor Gray
}
Write-Host ""

# Test: Offline nodes count
Write-Host "Test: Offline Nodes Count" -ForegroundColor Cyan
Write-Host ""
$response = Run-Test "List nodes" `
    "$API_CORE/nodes" 200
if ($response) {
    $nodes = $response.Content | ConvertFrom-Json
    $offlineCount = @($nodes | Where-Object { -not $_.isActive }).Count
    $script:test_count += 1
    Write-Host -NoNewline "Test $($script:test_count): Offline nodes count > 0 ... "
    if ($offlineCount -gt 0) {
        Write-Host "PASS" -ForegroundColor Green -NoNewline
        Write-Host " (offline=$offlineCount)"
        $script:pass_count += 1
    } else {
        Write-Host "FAIL" -ForegroundColor Red -NoNewline
        Write-Host " (offline=$offlineCount)"
        $script:fail_count += 1
    }
}
Write-Host ""

# Test: Heartbeat
Write-Host "Test: Heartbeat" -ForegroundColor Cyan
Write-Host ""

$heartbeatPayload = @{
    signalStrength = -92
    battery = 87
    connectedNodes = 3
} | ConvertTo-Json

Run-Test "Send heartbeat for MeshNode-1" `
    "$API_BASE/node/$NODE_1/heartbeat" 200 "POST" $heartbeatPayload
Write-Host ""

# Test: Error cases
Write-Host "Test: Error Cases" -ForegroundColor Cyan
Write-Host ""

Run-Test "Non-existent node pages (empty result)" `
    "$API_BASE/node/invalid-node/pages" 200
Write-Host ""

# Validate real Heltec nodes only receive their pages + all users
Write-Host "Test: Real device sync scope" -ForegroundColor Cyan
Write-Host ""

$nodes = Get-ApiJson "$API_CORE/nodes"
$realNodes = $nodes | Where-Object { $_.nodeId -like "LoRA_*" -or $_.functionalName -like "LoRA_*" }
$groupsCount = ($groups | Measure-Object).Count

foreach ($node in $realNodes) {
    $pages = Get-ApiJson "$API_CORE/sync/pages?nodeId=$($node.nodeId)"
    $pageCount = $pages.page_count
    Write-Host "  Node $($node.nodeId): pages=$pageCount" -ForegroundColor Gray
    if ($pageCount -gt $groupsCount) {
        Write-Host "  FAIL Node has more pages than groups" -ForegroundColor Red
        $fail_count++
    }
}

$usersSync = Get-ApiJson "$API_CORE/sync/users"
Write-Host "  Users synced: $($usersSync.user_count) (new users added: $totalNewUsers)" -ForegroundColor Gray
if ($usersSync.user_count -lt $totalNewUsers) {
    Write-Host "  FAIL Sync users count smaller than expected" -ForegroundColor Red
    $fail_count++
}
Write-Host ""

# Summary
Write-Host "==========================================" -ForegroundColor Cyan
if ($fail_count -eq 0) {
    Write-Host "All tests passed!" -ForegroundColor Green
}
Write-Host "Test Results: " -NoNewline
Write-Host "$pass_count passed" -ForegroundColor Green -NoNewline
Write-Host ", " -NoNewline
Write-Host "$fail_count failed" -ForegroundColor Red -NoNewline
Write-Host ", $test_count total" -ForegroundColor Gray
Write-Host "==========================================" -ForegroundColor Cyan
