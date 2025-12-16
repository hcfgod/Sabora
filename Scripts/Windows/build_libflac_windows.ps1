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
$libflacDir = Join-Path $root "Engine\Vendor\libflac"
$liboggDir = Join-Path $root "Engine\Vendor\libogg"

# Check if libogg is built first (optional dependency for OGG FLAC)
$liboggLibDir = Join-Path $liboggDir "lib"
$hasLibogg = (Test-Path (Join-Path $liboggLibDir "ogg-debug.lib")) -or (Test-Path (Join-Path $liboggLibDir "ogg-release.lib"))

# Check if libflac directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $libflacDir "CMakeLists.txt"))) {
    Write-Host "libFLAC not found. Cloning libFLAC..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libFLAC if it doesn't exist
    if (-not (Test-Path $libflacDir)) {
        Push-Location $vendorDir
        try {
            # Clone with full history to access tags
            git clone https://github.com/xiph/flac.git libflac
            # Checkout latest stable tag
            Push-Location libflac
            try {
                git fetch --tags
                git checkout 1.4.3
                Write-Host "Checked out libFLAC tag: 1.4.3" -ForegroundColor Green
            }
            finally {
                Pop-Location
            }
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists but headers are missing, try to checkout tag
        $flacHeaderDir = Join-Path $libflacDir "include\FLAC"
        if (-not (Test-Path $flacHeaderDir)) {
            Write-Host "Headers not found, checking out 1.4.3 tag..." -ForegroundColor Yellow
            Push-Location $libflacDir
            try {
                git fetch --tags
                git checkout 1.4.3 2>$null
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "Checked out libFLAC tag: 1.4.3" -ForegroundColor Green
                }
            }
            finally {
                Pop-Location
            }
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $libflacDir "CMakeLists.txt"))) {
        throw "libFLAC CMakeLists.txt not found after cloning. libFLAC directory: $libflacDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $libflacDir "build-debug"
$buildDirRelease = Join-Path $libflacDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building libFLAC with CMake..."

# Configure libogg paths if available
$oggIncludeDir = Join-Path $liboggDir "include"
if (-not (Test-Path $oggIncludeDir)) {
    $oggIncludeDir = Join-Path $liboggDir "src"
}

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring libFLAC (Debug) with static runtime..."
$cmakeArgsDebug = @(
    "-S", "$libflacDir",
    "-B", "$buildDirDebug",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DINSTALL_MANPAGES=OFF",
    "-DINSTALL_CMAKE_CONFIG_MODULE=OFF",
    "-DWITH_STACK_PROTECTOR=OFF"
)

# Add libogg support if available
if ($hasLibogg) {
    $oggDebugLib = Join-Path $liboggLibDir "ogg-debug.lib"
    if (Test-Path $oggDebugLib) {
        $cmakeArgsDebug += "-DOGG_ROOT=$liboggDir"
        $cmakeArgsDebug += "-DOGG_INCLUDE_DIR=$oggIncludeDir"
        $cmakeArgsDebug += "-DOGG_LIBRARY=$oggDebugLib"
        Write-Host "  Configuring with libogg support (OGG FLAC enabled)" -ForegroundColor Green
    }
}

& cmake $cmakeArgsDebug

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

Write-Host "Building libFLAC (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libFLAC Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring libFLAC (Release) with static runtime..."
$cmakeArgsRelease = @(
    "-S", "$libflacDir",
    "-B", "$buildDirRelease",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DINSTALL_MANPAGES=OFF",
    "-DINSTALL_CMAKE_CONFIG_MODULE=OFF",
    "-DWITH_STACK_PROTECTOR=OFF"
)

# Add libogg support if available
if ($hasLibogg) {
    $oggReleaseLib = Join-Path $liboggLibDir "ogg-release.lib"
    if (Test-Path $oggReleaseLib) {
        $cmakeArgsRelease += "-DOGG_ROOT=$liboggDir"
        $cmakeArgsRelease += "-DOGG_INCLUDE_DIR=$oggIncludeDir"
        $cmakeArgsRelease += "-DOGG_LIBRARY=$oggReleaseLib"
        Write-Host "  Configuring with libogg support (OGG FLAC enabled)" -ForegroundColor Green
    }
}

& cmake $cmakeArgsRelease

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

Write-Host "Building libFLAC (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libFLAC Release build failed with exit code $LASTEXITCODE"
}

Write-Host "libFLAC built successfully!" -ForegroundColor Green

# Copy the built libraries and headers
$libDir = Join-Path $root "Engine\Vendor\libflac\lib"
$includeDir = Join-Path $root "Engine\Vendor\libflac\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Find and copy Debug libraries (FLAC and FLAC++ if built)
$flacDebugLib = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "FLAC*.lib" | Where-Object { $_.Name -like "*Debug*" -or $_.Name -like "*debug*" } | Select-Object -First 1
$flacDebugLibMain = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "FLAC.lib" | Where-Object { $_.Name -notlike "*++*" } | Select-Object -First 1

if ($flacDebugLibMain -and (Test-Path $flacDebugLibMain.FullName)) {
    Copy-Item $flacDebugLibMain.FullName (Join-Path $libDir "flac-debug.lib") -Force
    Write-Host "Copied libFLAC (Debug) to $libDir\flac-debug.lib" -ForegroundColor Green
} elseif ($flacDebugLib -and (Test-Path $flacDebugLib.FullName)) {
    Copy-Item $flacDebugLib.FullName (Join-Path $libDir "flac-debug.lib") -Force
    Write-Host "Copied libFLAC (Debug) to $libDir\flac-debug.lib" -ForegroundColor Green
} else {
    throw "libFLAC Debug library not found in build directory: $buildDirDebug"
}

# Find and copy Release libraries
$flacReleaseLib = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "FLAC*.lib" | Where-Object { $_.Name -like "*Release*" -or $_.Name -like "*release*" } | Select-Object -First 1
$flacReleaseLibMain = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "FLAC.lib" | Where-Object { $_.Name -notlike "*++*" } | Select-Object -First 1

if ($flacReleaseLibMain -and (Test-Path $flacReleaseLibMain.FullName)) {
    Copy-Item $flacReleaseLibMain.FullName (Join-Path $libDir "flac-release.lib") -Force
    Write-Host "Copied libFLAC (Release) to $libDir\flac-release.lib" -ForegroundColor Green
} elseif ($flacReleaseLib -and (Test-Path $flacReleaseLib.FullName)) {
    Copy-Item $flacReleaseLib.FullName (Join-Path $libDir "flac-release.lib") -Force
    Write-Host "Copied libFLAC (Release) to $libDir\flac-release.lib" -ForegroundColor Green
} else {
    throw "libFLAC Release library not found in build directory: $buildDirRelease"
}

# Copy headers
$headerSourceDir = Join-Path $libflacDir "include\FLAC"
if (Test-Path $headerSourceDir) {
    $headerDestDir = Join-Path $includeDir "FLAC"
    if (-not (Test-Path $headerDestDir)) {
        Copy-Item -Path $headerSourceDir -Destination $headerDestDir -Recurse -Force
        Write-Host "Copied FLAC headers to $headerDestDir" -ForegroundColor Green
    } else {
        Write-Host "FLAC headers already in correct location: $headerDestDir" -ForegroundColor Yellow
    }
} else {
    Write-Warning "FLAC header directory not found at: $headerSourceDir"
}

Write-Host "libFLAC build complete!" -ForegroundColor Green
