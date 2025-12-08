Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Function to check if a command is available
function Test-Command($command) {
    return [bool](Get-Command $command -ErrorAction SilentlyContinue)
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

# Check and install CMake if needed
if (-not (Test-Command cmake)) {
    Install-CMake
}

Write-Host "CMake is available!" -ForegroundColor Green

$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$sdlDir = Join-Path $root "Engine\Vendor\SDL"
$buildDir = Join-Path $sdlDir "build"

# Check if SDL3 directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $sdlDir "CMakeLists.txt"))) {
    Write-Host "SDL3 not found. Cloning SDL3..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone SDL3 if it doesn't exist
    if (-not (Test-Path $sdlDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/libsdl-org/SDL.git SDL
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $sdlDir "CMakeLists.txt"))) {
        throw "SDL3 CMakeLists.txt not found after cloning. SDL directory: $sdlDir"
    }
}

# Create build directory
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Write-Host "Building SDL3 with CMake..."

# Configure SDL3 with CMake

Write-Host "Configuring SDL3..."
& cmake -S "$sdlDir" -B "$buildDir" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DSDL_STATIC=ON `
    -DSDL_SHARED=OFF `
    -DSDL_INSTALL=OFF `
    -DSDL_TESTS=OFF `
    -DSDL_EXAMPLES=OFF `
    -DSDL_INSTALL_TESTS=OFF `
    -DSDL_DEPS_SHARED=OFF `
    -DSDL_RPATH=OFF `
    -DSDL_AUDIO=ON `
    -DSDL_VIDEO=ON `
    -DSDL_GPU=ON `
    -DSDL_RENDER=ON `
    -DSDL_CAMERA=ON `
    -DSDL_JOYSTICK=ON `
    -DSDL_HAPTIC=ON `
    -DSDL_HIDAPI=ON `
    -DSDL_POWER=ON `
    -DSDL_SENSOR=ON `
    -DSDL_DIALOG=ON `
    -DSDL_DIRECTX=ON `
    -DSDL_RENDER_D3D=ON `
    -DSDL_RENDER_D3D11=ON `
    -DSDL_RENDER_D3D12=ON `
    -DSDL_RENDER_GPU=ON `
    -DSDL_RENDER_VULKAN=ON `
    -DSDL_WASAPI=ON `
    -DSDL_XINPUT=ON `
    -DSDL_OPENGL=ON `
    -DSDL_OPENGLES=ON `
    -DSDL_VULKAN=ON `
    -DSDL_DISKAUDIO=ON `
    -DSDL_DUMMYAUDIO=ON `
    -DSDL_DUMMYVIDEO=ON `
    -DSDL_OFFSCREEN=ON `
    -DSDL_DUMMYCAMERA=ON `
    -DSDL_VIRTUAL_JOYSTICK=ON `
    -DSDL_ASSEMBLY=ON `
    -DSDL_AVX=ON `
    -DSDL_AVX2=ON `
    -DSDL_AVX512F=ON `
    -DSDL_SSE=ON `
    -DSDL_SSE2=ON `
    -DSDL_SSE3=ON `
    -DSDL_SSE4_1=ON `
    -DSDL_SSE4_2=ON

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
}

# Build SDL3 in both configurations
Write-Host "Building SDL3 (Release)..."
& cmake --build $buildDir --config Release

if ($LASTEXITCODE -ne 0) {
    throw "SDL3 Release build failed with exit code $LASTEXITCODE"
}

Write-Host "Building SDL3 (Debug)..."
& cmake --build $buildDir --config Debug

if ($LASTEXITCODE -ne 0) {
    throw "SDL3 Debug build failed with exit code $LASTEXITCODE"
}

Write-Host "SDL3 built successfully!"

# Copy the built library and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\SDL\lib"
$includeDir = Join-Path $root "Engine\Vendor\SDL\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy the static library - CMake outputs it as SDL3-static.lib
$libFileRelease = Join-Path $buildDir "Release\SDL3-static.lib"
$libFileDebug = Join-Path $buildDir "Debug\SDL3-static.lib"

# Copy both Debug and Release libraries
if (Test-Path $libFileRelease) {
    Copy-Item $libFileRelease (Join-Path $libDir "SDL3-static-release.lib") -Force
    Write-Host "Copied SDL3-static.lib (Release) to $libDir"
}

if (Test-Path $libFileDebug) {
    Copy-Item $libFileDebug (Join-Path $libDir "SDL3-static-debug.lib") -Force
    Write-Host "Copied SDL3-static.lib (Debug) to $libDir"
}

if (-not (Test-Path $libFileRelease) -and -not (Test-Path $libFileDebug)) {
    Write-Warning "SDL3 library not found. Checking build directory structure..."
    Get-ChildItem -Recurse -Path $buildDir -Name "*.lib" | ForEach-Object { Write-Host "Found lib: $_" }
}

# Copy the generated SDL_build_config.h file - CMake puts it in include-config-release\build_config\
$buildConfigFile = Join-Path $buildDir "include-config-release\build_config\SDL_build_config.h"
$buildConfigFileDebug = Join-Path $buildDir "include-config-debug\build_config\SDL_build_config.h"

if (Test-Path $buildConfigFile) {
    Copy-Item $buildConfigFile $includeDir -Force
    Write-Host "Copied SDL_build_config.h (Release) to $includeDir"
} elseif (Test-Path $buildConfigFileDebug) {
    Copy-Item $buildConfigFileDebug $includeDir -Force
    Write-Host "Copied SDL_build_config.h (Debug) to $includeDir"
} else {
    Write-Warning "SDL_build_config.h not found. Checking build directory structure..."
    Get-ChildItem -Recurse -Path $buildDir -Name "SDL_build_config.h" | ForEach-Object { Write-Host "Found config: $_" }
}

Write-Host "SDL3 build complete!"