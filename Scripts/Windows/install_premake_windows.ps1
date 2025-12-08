Param(
    [string]$Version = $env:PREMAKE_VERSION,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

if (-not $Version -or $Version.Trim() -eq '') { $Version = '5.0.0-beta7' }

$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$toolsDir = Join-Path $root "Tools\Premake"
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null

$exePath = Join-Path $toolsDir "premake5.exe"
if (-not $Force -and (Test-Path $exePath)) {
    Write-Host "Premake already installed at $exePath"
    exit 0
}

$url = "https://github.com/premake/premake-core/releases/download/v$Version/premake-$Version-windows.zip"
$zipPath = Join-Path $toolsDir "premake.zip"

try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

Write-Host "Downloading Premake $Version from $url ..."
Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing

# Clean any previous extracted folders
Get-ChildItem -Path $toolsDir -Filter "premake-*-windows" -Directory -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

Expand-Archive -Path $zipPath -DestinationPath $toolsDir -Force

if (Test-Path $exePath) {
    # Extracted directly to the destination folder
    Remove-Item -Force $zipPath
} else {
    # Locate the downloaded executable anywhere under tools dir
    # Try premake5.exe first, then premake.exe as fallback
    $downloadedExe = Get-ChildItem -Path $toolsDir -Recurse -Filter "premake5.exe" -File | Select-Object -First 1
    if (-not $downloadedExe) {
        $downloadedExe = Get-ChildItem -Path $toolsDir -Recurse -Filter "premake.exe" -File | Select-Object -First 1
    }
    if (-not $downloadedExe) {
        throw "premake5.exe or premake.exe not found after extraction."
    }
    Move-Item -Force $downloadedExe.FullName $exePath
    Remove-Item -Force $zipPath
    # Remove leftover extracted directories
    Get-ChildItem -Path $toolsDir -Filter "premake-*-windows" -Directory -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Premake installed at $exePath"

