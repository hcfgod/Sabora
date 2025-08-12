Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Function to check if a command is available
function Test-Command($command) {
    return [bool](Get-Command $command -ErrorAction SilentlyContinue)
}

# Function to install Git using winget
function Install-Git {
    Write-Host "Git not found. Installing Git using winget..." -ForegroundColor Yellow
    
    if (Test-Command winget) {
        Write-Host "Installing Git..." -ForegroundColor Green
        winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Git installed successfully!" -ForegroundColor Green
            Write-Host "Please restart your terminal/PowerShell and run this script again." -ForegroundColor Yellow
            Write-Host "This ensures the updated PATH is available." -ForegroundColor Yellow
            exit 0
        } else {
            throw "Failed to install Git using winget. Please install Git manually from https://git-scm.com/"
        }
    } else {
        throw "winget not available. Please install Git manually from https://git-scm.com/"
    }
}

# Function to install CMake using winget
function Install-CMake {
    Write-Host "CMake not found. Installing CMake using winget..." -ForegroundColor Yellow
    
    if (Test-Command winget) {
        Write-Host "Installing CMake..." -ForegroundColor Green
        winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -eq 0) {
            Write-Host "CMake installed successfully!" -ForegroundColor Green
            Write-Host "Please restart your terminal/PowerShell and run this script again." -ForegroundColor Yellow
            Write-Host "This ensures the updated PATH is available." -ForegroundColor Yellow
            exit 0
        } else {
            throw "Failed to install CMake using winget. Please install CMake manually from https://cmake.org/"
        }
    } else {
        throw "winget not available. Please install CMake manually from https://cmake.org/"
    }
}

# Check and install Git if needed
if (-not (Test-Command git)) {
    Install-Git
}

# Check and install CMake if needed
if (-not (Test-Command cmake)) {
    Install-CMake
}

Write-Host "All required tools are available!" -ForegroundColor Green

$root = (Resolve-Path "$PSScriptRoot\..").Path
$vendor = Join-Path $root "Engine\Vendor"
New-Item -ItemType Directory -Force -Path $vendor | Out-Null

function Clone-Or-Update($url, $targetDir) {
    if (Test-Path (Join-Path $targetDir ".git")) {
        if ($Force) {
            Write-Host "Resetting $targetDir ..."
            git -C $targetDir fetch --depth 1 origin
            git -C $targetDir reset --hard FETCH_HEAD
        } else {
            Write-Host "Updating $targetDir ..."
            git -C $targetDir pull --ff-only
        }
    } else {
        Write-Host "Cloning $url -> $targetDir ..."
        git clone --recursive --depth 1 $url $targetDir
    }
}

function Install-SDL3($vendorDir) {
    $sdlDir = Join-Path $vendorDir "SDL"
    
    # Remove existing SDL directory if it exists
    if (Test-Path $sdlDir) {
        Write-Host "Removing existing SDL directory..."
        Remove-Item -Recurse -Force $sdlDir
    }
    
    # Clone SDL3
    Write-Host "Cloning SDL3..."
    git clone --recursive --depth 1 "https://github.com/libsdl-org/SDL.git" $sdlDir
    
    # Build SDL3 with CMake to generate build config files
    Write-Host "Building SDL3 with CMake to generate build configuration..."
    & (Join-Path $PSScriptRoot "build_sdl3_windows.ps1")
}

Clone-Or-Update "https://github.com/gabime/spdlog.git"    (Join-Path $vendor "spdlog")
Clone-Or-Update "https://github.com/doctest/doctest.git"   (Join-Path $vendor "doctest")
Clone-Or-Update "https://github.com/g-truc/glm.git"        (Join-Path $vendor "glm")
Clone-Or-Update "https://github.com/nlohmann/json.git"     (Join-Path $vendor "json")

# Install SDL3 with custom premake5.lua
Install-SDL3 $vendor

Write-Host "Dependencies are ready under $vendor"