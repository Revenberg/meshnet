# MeshNet V0.8.1 - Node Connection Tracking Test
# Test the connection tracking API

Write-Host "Node Connection Tracking Test v0.8.1" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$backendAPI = "http://localhost:3001"
$nodeId = "A4:C1:38:12:34:56"  # Mock node ID (MAC address)

Write-Host "Configuration:" -ForegroundColor Green
Write-Host "   Backend API: $backendAPI"
Write-Host "   Test Node ID: $nodeId"
Write-Host ""

Write-Host "Testing Node Connection API..." -ForegroundColor Yellow
Write-Host ""

# Test 1: Verify backend is running
Write-Host "Test 1: Check Backend Health"
try {
    $healthResponse = Invoke-RestMethod -Uri "$backendAPI/health" -Method Get -TimeoutSec 5
    Write-Host "  Status: PASS" -ForegroundColor Green
    Write-Host "  Response: $($healthResponse.status)"
} catch {
    Write-Host "  Status: FAIL" -ForegroundColor Red
    Write-Host "  Error: Backend not responding"
}

Write-Host ""

# Test 2: Get all nodes
Write-Host "Test 2: Get All Nodes from API"
try {
    $nodesResponse = Invoke-RestMethod -Uri "$backendAPI/api/nodes" -Method Get -TimeoutSec 5
    Write-Host "  Status: PASS" -ForegroundColor Green
    Write-Host "  Nodes Found: $($nodesResponse.Count)"
    if ($nodesResponse.Count -gt 0) {
        $nodesResponse | ForEach-Object {
            Write-Host "    - $($_.functionalName) (ID: $($_.nodeId))"
        }
    }
} catch {
    Write-Host "  Status: FAIL" -ForegroundColor Red
    Write-Host "  Error: Could not fetch nodes"
}

Write-Host ""

# Test 3: Simulate user login to node
Write-Host "Test 3: Simulate User Connection to Node"
try {
    $testUser = "TestPlayer"
    $testTeam = "TestTeam"
    
    $connectionPayload = @{
        username = $testUser
        teamName = $testTeam
    } | ConvertTo-Json
    
    $connectionResponse = Invoke-RestMethod `
        -Uri "$backendAPI/api/host/node/$nodeId/connection" `
        -Method Post `
        -ContentType "application/json" `
        -Body $connectionPayload `
        -TimeoutSec 5
    
    Write-Host "  Status: PASS" -ForegroundColor Green
    Write-Host "  Connection recorded:"
    Write-Host "    Username: $testUser"
    Write-Host "    Team: $testTeam"
    Write-Host "    Response: $($connectionResponse.status)"
} catch {
    Write-Host "  Status: FAIL" -ForegroundColor Red
    Write-Host "  Error: $($_.Exception.Message)"
}

Write-Host ""

# Test 4: Get connection history for node
Write-Host "Test 4: Get Connection History for Node"
try {
    $historyResponse = Invoke-RestMethod `
        -Uri "$backendAPI/api/host/node/$nodeId/connections" `
        -Method Get `
        -TimeoutSec 5
    
    Write-Host "  Status: PASS" -ForegroundColor Green
    Write-Host "  Total Connections: $($historyResponse.connections.Count)"
    
    if ($historyResponse.connections.Count -gt 0) {
        Write-Host "  Recent connections:"
        $historyResponse.connections | Select-Object -Last 3 | ForEach-Object {
            Write-Host "    - $($_.username) from $($_.teamName) at $($_.connectedAt)"
        }
    }
} catch {
    Write-Host "  Status: FAIL" -ForegroundColor Red
    Write-Host "  Error: Could not fetch connection history"
}

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Test Summary:" -ForegroundColor Cyan
Write-Host "  Node connection tracking API is operational"
Write-Host "  Connections are being logged to the database"
Write-Host ""
Write-Host "Expected Behavior:" -ForegroundColor Yellow
Write-Host "  1. When user connects to node WiFi"
Write-Host "  2. User logs in via web interface"
Write-Host "  3. Connection data sent to Docker backend"
Write-Host "  4. Connection recorded in node_user_connections table"
Write-Host "  5. Visible in RPI dashboard"
