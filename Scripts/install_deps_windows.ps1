Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Ensure git is available
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required to download dependencies. Please install git and retry."
}

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