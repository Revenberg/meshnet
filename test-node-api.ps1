# PowerShell Test Script for MeshNet Node Web Hosting API
# Usage: .\test-node-api.ps1

Write-Host "🧪 MeshNet Node Web Hosting API Test Suite" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Node IDs (hardcoded from setup)
$NODE_1 = "72d67530-dac6-4666-885c-160cb36579ee"
$NODE_2 = "26b80c3a-a7e2-4634-957a-51f7b777de72"
$NODE_3 = "d1ec1f02-0e0b-4763-94d5-984e93c11bde"

$API_BASE = "http://localhost:3001/api/host"

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
            Write-Host "✓ PASS" -ForegroundColor Green -NoNewline
            Write-Host " (HTTP $httpCode)"
            $script:pass_count += 1
            return $response
        }
        else {
            Write-Host "✗ FAIL" -ForegroundColor Red -NoNewline
            Write-Host " (Expected $expectedCode, got $httpCode)"
            $script:fail_count += 1
            return $null
        }
    }
    catch {
        Write-Host "✗ FAIL" -ForegroundColor Red -NoNewline
        Write-Host " (Error: $($_.Exception.Message))"
        $script:fail_count += 1
        return $null
    }
}

# Test: List pages for nodes
Write-Host "📋 Test: Get Pages for Nodes" -ForegroundColor Cyan
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
Write-Host "📄 Test: Get Page Content" -ForegroundColor Cyan
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
Write-Host "📡 Test: Node Information" -ForegroundColor Cyan
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

# Test: Heartbeat
Write-Host "💓 Test: Heartbeat" -ForegroundColor Cyan
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
Write-Host "❌ Test: Error Cases" -ForegroundColor Cyan
Write-Host ""

Run-Test "Non-existent node pages (empty result)" `
    "$API_BASE/node/invalid-node/pages" 200
Write-Host ""

# Summary
Write-Host "==========================================" -ForegroundColor Cyan
if ($fail_count -eq 0) {
    Write-Host "✓ All tests passed!" -ForegroundColor Green
}
Write-Host "Test Results: " -NoNewline
Write-Host "$pass_count passed" -ForegroundColor Green -NoNewline
Write-Host ", " -NoNewline
Write-Host "$fail_count failed" -ForegroundColor Red -NoNewline
Write-Host ", $test_count total" -ForegroundColor Gray
Write-Host "==========================================" -ForegroundColor Cyan
