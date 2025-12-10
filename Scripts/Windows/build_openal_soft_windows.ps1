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
$openalSoftDir = Join-Path $root "Engine\Vendor\openal-soft"
$buildDir = Join-Path $openalSoftDir "build"

# Check if openal-soft directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $openalSoftDir "CMakeLists.txt"))) {
    Write-Host "openal-soft not found. Cloning openal-soft..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone openal-soft if it doesn't exist
    if (-not (Test-Path $openalSoftDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/kcat/openal-soft.git openal-soft
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $openalSoftDir "CMakeLists.txt"))) {
        throw "openal-soft CMakeLists.txt not found after cloning. openal-soft directory: $openalSoftDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $openalSoftDir "build-debug"
$buildDirRelease = Join-Path $openalSoftDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building openal-soft with CMake..."

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring openal-soft (Debug) with static runtime..."
& cmake -S "$openalSoftDir" -B "$buildDirDebug" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DLIBTYPE=STATIC `
    -DALSOFT_UTILS=OFF `
    -DALSOFT_EXAMPLES=OFF `
    -DALSOFT_TESTS=OFF `
    -DALSOFT_INSTALL=OFF

if ($LASTEXITCODE -ne 0) {
    throw "CMake Debug configuration failed with exit code $LASTEXITCODE"
}

# Force static runtime library in generated project files
Write-Host "Fixing runtime library settings in Debug project files..."
$vcxprojFiles = Get-ChildItem -Path $buildDirDebug -Filter "*.vcxproj" -Recurse
foreach ($projFile in $vcxprojFiles) {
    [xml]$xml = Get-Content $projFile.FullName
    $modified = $false
    
    foreach ($propGroup in $xml.Project.PropertyGroup) {
        if ($propGroup.RuntimeLibrary) {
            if ($propGroup.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or 
                $propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
    }
    
    foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
        if ($itemDefGroup.ClCompile.RuntimeLibrary) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or 
                $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL") {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
    }
    
    if ($modified) {
        $xml.Save($projFile.FullName)
        Write-Host "  Fixed: $($projFile.Name)"
    }
}

Write-Host "Building openal-soft (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "openal-soft Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring openal-soft (Release) with static runtime..."
& cmake -S "$openalSoftDir" -B "$buildDirRelease" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DLIBTYPE=STATIC `
    -DALSOFT_UTILS=OFF `
    -DALSOFT_EXAMPLES=OFF `
    -DALSOFT_TESTS=OFF `
    -DALSOFT_INSTALL=OFF

if ($LASTEXITCODE -ne 0) {
    throw "CMake Release configuration failed with exit code $LASTEXITCODE"
}

# Force static runtime library in generated project files
Write-Host "Fixing runtime library settings in Release project files..."
$vcxprojFiles = Get-ChildItem -Path $buildDirRelease -Filter "*.vcxproj" -Recurse
foreach ($projFile in $vcxprojFiles) {
    [xml]$xml = Get-Content $projFile.FullName
    $modified = $false
    
    foreach ($propGroup in $xml.Project.PropertyGroup) {
        if ($propGroup.RuntimeLibrary) {
            if ($propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
    }
    
    foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
        if ($itemDefGroup.ClCompile.RuntimeLibrary) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL") {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
    }
    
    if ($modified) {
        $xml.Save($projFile.FullName)
        Write-Host "  Fixed: $($projFile.Name)"
    }
}

Write-Host "Building openal-soft (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "openal-soft Release build failed with exit code $LASTEXITCODE"
}

Write-Host "openal-soft built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\openal-soft\lib"
$includeDir = Join-Path $root "Engine\Vendor\openal-soft\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy OpenAL Soft library
# Note: OpenAL Soft typically builds as OpenAL32.lib on Windows (same name for Debug and Release)
$openalLibRelease = Join-Path $buildDirRelease "Release\OpenAL32.lib"
$openalLibDebug = Join-Path $buildDirDebug "Debug\OpenAL32.lib"

# Try alternative names
if (-not (Test-Path $openalLibRelease)) {
    $openalLibRelease = Join-Path $buildDirRelease "Release\openal.lib"
}
if (-not (Test-Path $openalLibDebug)) {
    # Try OpenAL32.lib (without 'd' suffix) first
    $openalLibDebug = Join-Path $buildDirDebug "Debug\OpenAL32.lib"
    if (-not (Test-Path $openalLibDebug)) {
        $openalLibDebug = Join-Path $buildDirDebug "Debug\openald.lib"
        if (-not (Test-Path $openalLibDebug)) {
            $openalLibDebug = Join-Path $buildDirDebug "Debug\openal.lib"
        }
    }
}

if (Test-Path $openalLibRelease) {
    Copy-Item $openalLibRelease (Join-Path $libDir "OpenAL32-release.lib") -Force
    Write-Host "Copied openal-soft library (Release) to $libDir"
} else {
    Write-Warning "Release library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirRelease -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $openalLibDebug) {
    Copy-Item $openalLibDebug (Join-Path $libDir "OpenAL32-debug.lib") -Force
    Write-Host "Copied openal-soft library (Debug) to $libDir"
} else {
    Write-Warning "Debug library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirDebug -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

# Copy headers (OpenAL headers are typically in include/AL/)
$headerSourceDir = Join-Path $openalSoftDir "include"
if (Test-Path $headerSourceDir) {
    # Only copy if source and destination are different
    $resolvedSource = (Resolve-Path $headerSourceDir).Path
    $resolvedDest = (Resolve-Path $includeDir -ErrorAction SilentlyContinue).Path
    if ($resolvedSource -ne $resolvedDest) {
        Copy-Item -Path "$headerSourceDir\*" -Destination $includeDir -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "Copied openal-soft headers to $includeDir"
    } else {
        Write-Host "Headers already in place at $includeDir"
    }
} else {
    # Try alternative location
    $altHeaderDir = Join-Path $openalSoftDir "include\AL"
    if (Test-Path $altHeaderDir) {
        New-Item -ItemType Directory -Force -Path (Join-Path $includeDir "AL") | Out-Null
        Copy-Item -Path "$altHeaderDir\*" -Destination (Join-Path $includeDir "AL") -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "Copied openal-soft headers to $includeDir"
    }
}

Write-Host "openal-soft build complete!" -ForegroundColor Green

