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
$libsndfileDir = Join-Path $root "Engine\Vendor\libsndfile"
$buildDir = Join-Path $libsndfileDir "build"

# Check if libsndfile directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $libsndfileDir "CMakeLists.txt"))) {
    Write-Host "libsndfile not found. Cloning libsndfile..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libsndfile if it doesn't exist
    if (-not (Test-Path $libsndfileDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/libsndfile/libsndfile.git libsndfile
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $libsndfileDir "CMakeLists.txt"))) {
        throw "libsndfile CMakeLists.txt not found after cloning. libsndfile directory: $libsndfileDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $libsndfileDir "build-debug"
$buildDirRelease = Join-Path $libsndfileDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building libsndfile with CMake..."

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring libsndfile (Debug) with static runtime..."
& cmake -S "$libsndfileDir" -B "$buildDirDebug" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DBUILD_SHARED_LIBS=OFF `
    -DBUILD_STATIC_LIBS=ON `
    -DBUILD_PROGRAMS=OFF `
    -DBUILD_EXAMPLES=OFF `
    -DBUILD_TESTING=OFF `
    -DENABLE_EXTERNAL_LIBS=OFF `
    -DINSTALL_PKGCONFIG_MODULE=OFF `
    -DINSTALL_MANPAGES=OFF

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

Write-Host "Building libsndfile (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libsndfile Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring libsndfile (Release) with static runtime..."
& cmake -S "$libsndfileDir" -B "$buildDirRelease" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DBUILD_SHARED_LIBS=OFF `
    -DBUILD_STATIC_LIBS=ON `
    -DBUILD_PROGRAMS=OFF `
    -DBUILD_EXAMPLES=OFF `
    -DBUILD_TESTING=OFF `
    -DENABLE_EXTERNAL_LIBS=OFF `
    -DINSTALL_PKGCONFIG_MODULE=OFF `
    -DINSTALL_MANPAGES=OFF

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

Write-Host "Building libsndfile (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libsndfile Release build failed with exit code $LASTEXITCODE"
}

Write-Host "libsndfile built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\libsndfile\lib"
$includeDir = Join-Path $root "Engine\Vendor\libsndfile\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy libsndfile library
# Note: libsndfile typically builds as sndfile.lib on Windows
$sndfileLibRelease = Join-Path $buildDirRelease "Release\sndfile.lib"
$sndfileLibDebug = Join-Path $buildDirDebug "Debug\sndfile.lib"

# Try alternative names
if (-not (Test-Path $sndfileLibRelease)) {
    $sndfileLibRelease = Join-Path $buildDirRelease "Release\libsndfile.lib"
}
if (-not (Test-Path $sndfileLibDebug)) {
    $sndfileLibDebug = Join-Path $buildDirDebug "Debug\sndfiled.lib"
    if (-not (Test-Path $sndfileLibDebug)) {
        $sndfileLibDebug = Join-Path $buildDirDebug "Debug\libsndfiled.lib"
        if (-not (Test-Path $sndfileLibDebug)) {
            $sndfileLibDebug = Join-Path $buildDirDebug "Debug\sndfile.lib"
        }
    }
}

if (Test-Path $sndfileLibRelease) {
    Copy-Item $sndfileLibRelease (Join-Path $libDir "sndfile-release.lib") -Force
    Write-Host "Copied libsndfile library (Release) to $libDir"
} else {
    Write-Warning "Release library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirRelease -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $sndfileLibDebug) {
    Copy-Item $sndfileLibDebug (Join-Path $libDir "sndfile-debug.lib") -Force
    Write-Host "Copied libsndfile library (Debug) to $libDir"
} else {
    Write-Warning "Debug library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirDebug -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

# Copy headers (libsndfile headers are typically in include/sndfile.h)
$headerSourceDir = Join-Path $libsndfileDir "src"
if (Test-Path (Join-Path $libsndfileDir "include\sndfile.h")) {
    $headerSourceDir = Join-Path $libsndfileDir "include"
}

if (Test-Path $headerSourceDir) {
    # Copy sndfile.h
    $headerSourcePath = Join-Path $headerSourceDir "sndfile.h"
    $headerDestPath = Join-Path $includeDir "sndfile.h"
    
    if (Test-Path $headerSourcePath) {
        # Only copy if source and destination are different
        $resolvedSource = (Resolve-Path $headerSourcePath).Path
        $resolvedDest = (Resolve-Path $headerDestPath -ErrorAction SilentlyContinue).Path
        if ($resolvedSource -ne $resolvedDest) {
            Copy-Item $headerSourcePath $headerDestPath -Force -ErrorAction SilentlyContinue
            Write-Host "Copied sndfile.h to $includeDir"
        } else {
            Write-Host "Header already in place at $includeDir"
        }
    }
    # Also check for sndfile.h in src directory (if different from include)
    $srcHeaderPath = Join-Path $libsndfileDir "src\sndfile.h"
    if (Test-Path $srcHeaderPath) {
        $resolvedSrc = (Resolve-Path $srcHeaderPath).Path
        $resolvedDest = (Resolve-Path $headerDestPath -ErrorAction SilentlyContinue).Path
        if ($resolvedSrc -ne $resolvedDest) {
            Copy-Item $srcHeaderPath $headerDestPath -Force -ErrorAction SilentlyContinue
            Write-Host "Copied sndfile.h (from src) to $includeDir"
        }
    }
} else {
    Write-Warning "Header file not found. Searching..."
    Get-ChildItem -Recurse -Path $libsndfileDir -Filter "sndfile.h" | ForEach-Object { 
        $resolvedSource = (Resolve-Path $_.FullName).Path
        $resolvedDest = (Resolve-Path $headerDestPath -ErrorAction SilentlyContinue).Path
        if ($resolvedSource -ne $resolvedDest) {
            Write-Host "Found: $($_.FullName)"
            Copy-Item $_.FullName $headerDestPath -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "libsndfile build complete!" -ForegroundColor Green

