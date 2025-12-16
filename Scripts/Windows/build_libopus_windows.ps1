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
$libopusDir = Join-Path $root "Engine\Vendor\libopus"

# Check if libopus directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $libopusDir "CMakeLists.txt"))) {
    Write-Host "libopus not found. Cloning libopus..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libopus if it doesn't exist
    if (-not (Test-Path $libopusDir)) {
        Push-Location $vendorDir
        try {
            # Clone with full history to access tags
            git clone https://gitlab.xiph.org/xiph/opus.git libopus
            Write-Host "Cloned libopus repository" -ForegroundColor Green
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists but headers are missing, try to update
        $opusHeaderDir = Join-Path $libopusDir "include\opus"
        if (-not (Test-Path $opusHeaderDir)) {
            Write-Host "Headers not found, updating repository..." -ForegroundColor Yellow
            Push-Location $libopusDir
            try {
                git pull
                if ($LASTEXITCODE -ne 0) {
                    Write-Host "Warning: git pull failed, but continuing..." -ForegroundColor Yellow
                }
            }
            finally {
                Pop-Location
            }
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $libopusDir "CMakeLists.txt"))) {
        throw "libopus CMakeLists.txt not found after cloning. libopus directory: $libopusDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $libopusDir "build-debug"
$buildDirRelease = Join-Path $libopusDir "build-release"

if ($Force -or -not (Test-Path $buildDirDebug)) {
    New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
}

if ($Force -or -not (Test-Path $buildDirRelease)) {
    New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null
}

Write-Host "Building libopus with CMake..." -ForegroundColor Green

# Configure and build Debug configuration
Write-Host "Configuring libopus (Debug)..." -ForegroundColor Yellow
Push-Location $buildDirDebug
try {
    cmake -S $libopusDir `
        -B $buildDirDebug `
        -DCMAKE_BUILD_TYPE=Debug `
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
        -DBUILD_SHARED_LIBS=OFF `
        -DOPUS_BUILD_PROGRAMS=OFF `
        -DOPUS_BUILD_DOCS=OFF `
        -DOPUS_BUILD_TESTS=OFF `
        -DOPUS_BUILD_EXAMPLES=OFF
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopus Debug CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building libopus (Debug)..." -ForegroundColor Yellow
    cmake --build $buildDirDebug --config Debug --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopus Debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

# Configure and build Release configuration
Write-Host "Configuring libopus (Release)..." -ForegroundColor Yellow
Push-Location $buildDirRelease
try {
    cmake -S $libopusDir `
        -B $buildDirRelease `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
        -DBUILD_SHARED_LIBS=OFF `
        -DOPUS_BUILD_PROGRAMS=OFF `
        -DOPUS_BUILD_DOCS=OFF `
        -DOPUS_BUILD_TESTS=OFF `
        -DOPUS_BUILD_EXAMPLES=OFF
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopus Release CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building libopus (Release)..." -ForegroundColor Yellow
    cmake --build $buildDirRelease --config Release --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopus Release build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "libopus built successfully!" -ForegroundColor Green

# Copy the built libraries and headers
$libDir = Join-Path $root "Engine\Vendor\libopus\lib"
$includeDir = Join-Path $root "Engine\Vendor\libopus\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Find and copy Debug libraries
$opusDebugLib = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opus*.lib" | Where-Object { $_.Name -like "*Debug*" -or $_.Name -like "*debug*" } | Select-Object -First 1
$opusDebugLibMain = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opus.lib" | Select-Object -First 1

if ($opusDebugLibMain -and (Test-Path $opusDebugLibMain.FullName)) {
    Copy-Item $opusDebugLibMain.FullName (Join-Path $libDir "opus-debug.lib") -Force
    Write-Host "Copied libopus (Debug) to $libDir\opus-debug.lib" -ForegroundColor Green
} elseif ($opusDebugLib -and (Test-Path $opusDebugLib.FullName)) {
    Copy-Item $opusDebugLib.FullName (Join-Path $libDir "opus-debug.lib") -Force
    Write-Host "Copied libopus (Debug) to $libDir\opus-debug.lib" -ForegroundColor Green
} else {
    throw "libopus Debug library not found in build directory: $buildDirDebug"
}

# Find and copy Release libraries
$opusReleaseLib = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opus*.lib" | Where-Object { $_.Name -like "*Release*" -or $_.Name -like "*release*" } | Select-Object -First 1
$opusReleaseLibMain = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opus.lib" | Select-Object -First 1

if ($opusReleaseLibMain -and (Test-Path $opusReleaseLibMain.FullName)) {
    Copy-Item $opusReleaseLibMain.FullName (Join-Path $libDir "opus-release.lib") -Force
    Write-Host "Copied libopus (Release) to $libDir\opus-release.lib" -ForegroundColor Green
} elseif ($opusReleaseLib -and (Test-Path $opusReleaseLib.FullName)) {
    Copy-Item $opusReleaseLib.FullName (Join-Path $libDir "opus-release.lib") -Force
    Write-Host "Copied libopus (Release) to $libDir\opus-release.lib" -ForegroundColor Green
} else {
    throw "libopus Release library not found in build directory: $buildDirRelease"
}

# Copy headers
$headerSourceDir = Join-Path $libopusDir "include"
$opusHeaderDestDir = Join-Path $includeDir "opus"

# Create opus subdirectory in include
New-Item -ItemType Directory -Force -Path $opusHeaderDestDir | Out-Null

# Copy all header files from include to include/opus
if (Test-Path $headerSourceDir) {
    $headerFiles = Get-ChildItem -Path $headerSourceDir -Filter "*.h" -File
    foreach ($headerFile in $headerFiles) {
        $destFile = Join-Path $opusHeaderDestDir $headerFile.Name
        Copy-Item -Path $headerFile.FullName -Destination $destFile -Force
    }
    Write-Host "Copied opus headers to $opusHeaderDestDir" -ForegroundColor Green
} else {
    Write-Host "Warning: opus header directory not found at $headerSourceDir" -ForegroundColor Yellow
    Write-Host "You may need to manually copy the headers or rebuild libopus." -ForegroundColor Yellow
}

Write-Host "libopus build complete!" -ForegroundColor Green
