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
$liboggDir = Join-Path $root "Engine\Vendor\libogg"

# Check if libogg directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $liboggDir "CMakeLists.txt"))) {
    Write-Host "libogg not found. Cloning libogg..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libogg if it doesn't exist
    if (-not (Test-Path $liboggDir)) {
        Push-Location $vendorDir
        try {
            # Clone with full history to get tags
            git clone https://github.com/xiph/ogg.git libogg
            # Checkout v1.3.6 tag (latest stable release)
            Push-Location libogg
            try {
                git fetch --tags
                git checkout v1.3.6
                Write-Host "Checked out libogg tag: v1.3.6" -ForegroundColor Green
            }
            finally {
                Pop-Location
            }
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists, try to checkout a release tag if we're on main/master
        Push-Location $liboggDir
        try {
            $currentBranch = git rev-parse --abbrev-ref HEAD 2>$null
            if ($currentBranch -eq "main" -or $currentBranch -eq "master") {
                git fetch --tags
                git checkout v1.3.6 2>$null
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "Checked out libogg tag: v1.3.6" -ForegroundColor Green
                }
            }
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $liboggDir "CMakeLists.txt"))) {
        throw "libogg CMakeLists.txt not found after cloning. libogg directory: $liboggDir"
    }
}

# Ensure include/ogg directory exists (CMake needs it for config_types.h.in)
$includeOggDir = Join-Path $liboggDir "include\ogg"
if (-not (Test-Path $includeOggDir)) {
    New-Item -ItemType Directory -Force -Path $includeOggDir | Out-Null
    Write-Host "Created include/ogg directory for CMake configuration" -ForegroundColor Yellow
}

# Check if config_types.h.in exists, if not create it based on CMakeLists.txt requirements
$configTypesIn = Join-Path $includeOggDir "config_types.h.in"
if (-not (Test-Path $configTypesIn)) {
    Write-Host "config_types.h.in not found, creating template..." -ForegroundColor Yellow
    # This template matches what CMake's configure_file expects (typedef format, not #define)
    $configTypesContent = @"
#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

typedef @SIZE16@ ogg_int16_t;
typedef @USIZE16@ ogg_uint16_t;
typedef @SIZE32@ ogg_int32_t;
typedef @USIZE32@ ogg_uint32_t;
typedef @SIZE64@ ogg_int64_t;
typedef @USIZE64@ ogg_uint64_t;

#endif
"@
    Set-Content -Path $configTypesIn -Value $configTypesContent -Encoding UTF8
    Write-Host "Created config_types.h.in template" -ForegroundColor Green
}

# Check if ogg.h and os_types.h exist, if not search for them
$oggH = Join-Path $includeOggDir "ogg.h"
$osTypesH = Join-Path $includeOggDir "os_types.h"

if (-not (Test-Path $oggH)) {
    # Search for ogg.h in the repository (excluding build directories)
    $foundOggH = Get-ChildItem -Recurse -Path $liboggDir -Filter "ogg.h" -ErrorAction SilentlyContinue | 
        Where-Object { $_.FullName -notmatch "build-" -and $_.FullName -notmatch "\.git" } | 
        Select-Object -First 1
    if ($foundOggH) {
        Copy-Item $foundOggH.FullName $oggH -Force
        Write-Host "Found and copied ogg.h" -ForegroundColor Green
    } else {
        Write-Warning "ogg.h not found in repository. CMake build may fail."
    }
}

if (-not (Test-Path $osTypesH)) {
    # Search for os_types.h in the repository (excluding build directories)
    $foundOsTypesH = Get-ChildItem -Recurse -Path $liboggDir -Filter "os_types.h" -ErrorAction SilentlyContinue | 
        Where-Object { $_.FullName -notmatch "build-" -and $_.FullName -notmatch "\.git" } | 
        Select-Object -First 1
    if ($foundOsTypesH) {
        Copy-Item $foundOsTypesH.FullName $osTypesH -Force
        Write-Host "Found and copied os_types.h" -ForegroundColor Green
    } else {
        Write-Warning "os_types.h not found in repository. CMake build may fail."
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $liboggDir "build-debug"
$buildDirRelease = Join-Path $liboggDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building libogg with CMake..."

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring libogg (Debug) with static runtime..."
& cmake -S "$liboggDir" -B "$buildDirDebug" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DBUILD_SHARED_LIBS=OFF `
    -DINSTALL_DOCS=OFF `
    -DINSTALL_PKG_CONFIG_MODULE=OFF

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

Write-Host "Building libogg (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libogg Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring libogg (Release) with static runtime..."
& cmake -S "$liboggDir" -B "$buildDirRelease" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DBUILD_SHARED_LIBS=OFF `
    -DINSTALL_DOCS=OFF `
    -DINSTALL_PKG_CONFIG_MODULE=OFF

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

Write-Host "Building libogg (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libogg Release build failed with exit code $LASTEXITCODE"
}

Write-Host "libogg built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\libogg\lib"
$includeDir = Join-Path $root "Engine\Vendor\libogg\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy libogg library
# Note: libogg typically builds as ogg.lib on Windows
$oggLibRelease = Join-Path $buildDirRelease "Release\ogg.lib"
$oggLibDebug = Join-Path $buildDirDebug "Debug\ogg.lib"

# Try alternative names
if (-not (Test-Path $oggLibRelease)) {
    $oggLibRelease = Join-Path $buildDirRelease "Release\libogg.lib"
}

# Try alternative names for Debug library
if (-not (Test-Path $oggLibDebug)) {
    $altPath = Join-Path $buildDirDebug "Debug\oggd.lib"
    if (Test-Path $altPath) {
        $oggLibDebug = $altPath
    } else {
        $altPath = Join-Path $buildDirDebug "Debug\liboggd.lib"
        if (Test-Path $altPath) {
            $oggLibDebug = $altPath
        } else {
            $oggLibDebug = Join-Path $buildDirDebug "Debug\ogg.lib"
        }
    }
}

if (Test-Path $oggLibRelease) {
    Copy-Item $oggLibRelease (Join-Path $libDir "ogg-release.lib") -Force
    Write-Host "Copied libogg library (Release) to $libDir"
} else {
    Write-Warning "Release library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirRelease -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $oggLibDebug) {
    Copy-Item $oggLibDebug (Join-Path $libDir "ogg-debug.lib") -Force
    Write-Host "Copied libogg library (Debug) to $libDir"
} else {
    Write-Warning "Debug library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirDebug -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

# Headers are already in the source include/ogg/ directory (where premake expects them)
# We only need to copy the generated config_types.h from the build directory
$sourceOggDir = Join-Path $liboggDir "include\ogg"

# Ensure the source include/ogg directory exists
if (-not (Test-Path $sourceOggDir)) {
    New-Item -ItemType Directory -Force -Path $sourceOggDir | Out-Null
    Write-Host "Created include/ogg directory" -ForegroundColor Yellow
}

# Copy config_types.h from build directory (generated by CMake) to source include/ogg/
# This is the only file that needs to be copied since it's generated during build
$configTypesDebug = Join-Path $buildDirDebug "include\ogg\config_types.h"
$configTypesRelease = Join-Path $buildDirRelease "include\ogg\config_types.h"
$configTypesDest = Join-Path $sourceOggDir "config_types.h"

if (Test-Path $configTypesDebug) {
    $resolvedSource = (Resolve-Path $configTypesDebug).Path
    $resolvedDest = (Resolve-Path $configTypesDest -ErrorAction SilentlyContinue).Path
    if ($resolvedSource -ne $resolvedDest) {
        Copy-Item $configTypesDebug $configTypesDest -Force
        Write-Host "Copied config_types.h from Debug build" -ForegroundColor Green
    }
} elseif (Test-Path $configTypesRelease) {
    $resolvedSource = (Resolve-Path $configTypesRelease).Path
    $resolvedDest = (Resolve-Path $configTypesDest -ErrorAction SilentlyContinue).Path
    if ($resolvedSource -ne $resolvedDest) {
        Copy-Item $configTypesRelease $configTypesDest -Force
        Write-Host "Copied config_types.h from Release build" -ForegroundColor Green
    }
}

# Verify essential headers exist (ogg.h and os_types.h should already be in source from git checkout)
$requiredHeaders = @("ogg.h", "os_types.h", "config_types.h")
$missingHeaders = @()
foreach ($header in $requiredHeaders) {
    if (-not (Test-Path (Join-Path $sourceOggDir $header))) {
        $missingHeaders += $header
    }
}

if ($missingHeaders.Count -gt 0) {
    Write-Warning "Missing required headers: $($missingHeaders -join ', ')"
    Write-Warning "These should be present after checking out the release tag."
    Write-Warning "Searching repository for missing headers..."
    foreach ($header in $missingHeaders) {
        $found = Get-ChildItem -Recurse -Path $liboggDir -Filter $header -ErrorAction SilentlyContinue | 
            Where-Object { $_.FullName -notmatch "build-" -and $_.FullName -notmatch "\.git" } | 
            Select-Object -First 1
        if ($found) {
            Copy-Item $found.FullName (Join-Path $sourceOggDir $header) -Force
            Write-Host "Found and copied $header from $($found.DirectoryName)" -ForegroundColor Green
        }
    }
} else {
    Write-Host "All required headers verified in $sourceOggDir" -ForegroundColor Green
}

Write-Host "libogg build complete!" -ForegroundColor Green

