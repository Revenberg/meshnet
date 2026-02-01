# Test script voor het editen van groepen

$ErrorActionPreference = "Continue"

# 1. Login
Write-Host "Stap 1: Inloggen..." -ForegroundColor Cyan
$loginData = @{
    username = "admin"
    password = "admin123"
} | ConvertTo-Json

$loginResponse = Invoke-WebRequest -Uri "http://localhost/login" `
    -Method Post `
    -Body $loginData `
    -ContentType "application/json" `
    -SessionVariable session `
    -UseBasicParsing

Write-Host "Login response status: $($loginResponse.StatusCode)"

# 2. Visit groups page
Start-Sleep -Seconds 1
Write-Host "Stap 2: Groups pagina ophalen..." -ForegroundColor Cyan
$groupsPage = Invoke-WebRequest -Uri "http://localhost/groups" `
    -WebSession $session `
    -UseBasicParsing

Write-Host "Groups page status: $($groupsPage.StatusCode)"

# 3. Check for editGroup function and modal
Write-Host "Stap 3: Controleren op editGroup functie..." -ForegroundColor Cyan
if ($groupsPage.Content -like "*function editGroup*") {
    Write-Host "OK: editGroup functie gevonden" -ForegroundColor Green
} else {
    Write-Host "FAIL: editGroup functie NIET gevonden" -ForegroundColor Red
}

# 4. Check for modal
if ($groupsPage.Content -like "*id=*groupModal*") {
    Write-Host "OK: groupModal element gevonden" -ForegroundColor Green
} else {
    Write-Host "FAIL: groupModal NIET gevonden" -ForegroundColor Red
}

# 5. Check for edit buttons with onclick
$editCount = ($groupsPage.Content | Select-String -Pattern 'onclick.*editGroup' -AllMatches).Matches.Count
if ($editCount -gt 0) {
    Write-Host "OK: $editCount Edit button(s) onclick handler gevonden" -ForegroundColor Green
} else {
    Write-Host "FAIL: Geen edit button onclick handlers gevonden" -ForegroundColor Red
}

Write-Host ""
Write-Host "Test voltooid!" -ForegroundColor Green
