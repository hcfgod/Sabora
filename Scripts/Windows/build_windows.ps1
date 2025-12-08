[CmdletBinding()]
Param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Debug',
    [string[]]$Platforms = @('x64','ARM64'),
    [string]$PremakeVersion = $env:PREMAKE_VERSION
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path "$PSScriptRoot\..\..").Path

# Ensure Premake is installed locally
& "$PSScriptRoot\install_premake_windows.ps1" -Version $PremakeVersion | Write-Output

$premake = Join-Path $root "Tools\Premake\premake5.exe"
if (-not (Test-Path $premake)) { throw "Premake not found at $premake" }

Push-Location $root
try {
    & $premake vs2022
} finally {
    Pop-Location
}

$solution = Get-ChildItem -Path $root -Filter *.sln -Recurse | Select-Object -First 1
if (-not $solution) {
    Write-Warning "No .sln found after generation. Ensure your root premake5.lua defines a workspace."
    exit 0
}

# Locate MSBuild
$msbuild = $null
$vswhere = "$Env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $install = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if ($install) {
        $candidate = Join-Path $install "MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path $candidate) { $msbuild = $candidate }
    }
}
if (-not $msbuild) {
try {
    $cmd = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($cmd) { $msbuild = $cmd.Source }
} catch {}
}
if (-not $msbuild) {
    Write-Warning "MSBuild not found. Opening solution in Visual Studio instead."
    
    # Check if Visual Studio is already running
    $vsProcesses = Get-Process | Where-Object { $_.ProcessName -like "*devenv*" -or $_.ProcessName -like "*VisualStudio*" }
    if ($vsProcesses) {
        Write-Host "Visual Studio is already running. Opening solution in existing instance..."
        # Use the first running VS instance to open the solution
        $vsProcess = $vsProcesses[0]
        Start-Process $solution.FullName -WindowStyle Minimized
    } else {
        Write-Host "Starting new Visual Studio instance..."
        Start-Process $solution.FullName
    }
    exit 0
}

foreach ($platform in $Platforms) {
    Write-Host "Building $($solution.Name) Configuration=$Configuration Platform=$platform"
    & $msbuild $solution.FullName /m /p:Configuration=$Configuration /p:Platform=$platform
}

Write-Host "Build completed."

