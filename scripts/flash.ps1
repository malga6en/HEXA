param(
    [Parameter(Mandatory=$true)]
    [string]$Port
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Firmware = Join-Path $ProjectRoot "output\firmware.bin"

if (-not (Test-Path $Firmware)) {
    Write-Error "firmware.bin not found. Build the firmware in the Dev Container first."
    exit 1
}

Write-Host "Flashing HEXA"
Write-Host "Port: $Port"
Write-Host "Firmware: $Firmware"

python -m esptool `
    --chip esp32s2 `
    --port $Port `
    --baud 460800 `
    --before default-reset `
    --after hard-reset `
    write-flash `
    -z `
    --flash-mode dio `
    --flash-freq 80m `
    --flash-size 4MB `
    0x10000 $Firmware

if ($LASTEXITCODE -ne 0) {
    throw "Flashing failed."
}

Write-Host ""
Write-Host "HEXA firmware flashed successfully."