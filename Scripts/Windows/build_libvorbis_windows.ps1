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
$libvorbisDir = Join-Path $root "Engine\Vendor\libvorbis"
$liboggDir = Join-Path $root "Engine\Vendor\libogg"

# Check if libogg is built first (dependency)
$liboggLibDir = Join-Path $liboggDir "lib"
if (-not (Test-Path (Join-Path $liboggLibDir "ogg-debug.lib")) -and -not (Test-Path (Join-Path $liboggLibDir "ogg-release.lib"))) {
    throw "libogg must be built before libvorbis. Please run build_libogg_windows.ps1 first."
}

# Check if libvorbis directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $libvorbisDir "CMakeLists.txt"))) {
    Write-Host "libvorbis not found. Cloning libvorbis..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libvorbis if it doesn't exist
    if (-not (Test-Path $libvorbisDir)) {
        Push-Location $vendorDir
        try {
            # Clone with full history to access tags
            git clone https://github.com/xiph/vorbis.git libvorbis
            # Checkout latest stable tag to ensure headers are present
            Push-Location libvorbis
            try {
                git fetch --tags
                git checkout v1.3.7
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
        $vorbisHeaderDir = Join-Path $libvorbisDir "include\vorbis"
        if (-not (Test-Path $vorbisHeaderDir)) {
            Write-Host "Headers not found, checking out v1.3.7 tag..." -ForegroundColor Yellow
            Push-Location $libvorbisDir
            try {
                git fetch --tags
                git checkout v1.3.7
            }
            finally {
                Pop-Location
            }
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $libvorbisDir "CMakeLists.txt"))) {
        throw "libvorbis CMakeLists.txt not found after cloning. libvorbis directory: $libvorbisDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $libvorbisDir "build-debug"
$buildDirRelease = Join-Path $libvorbisDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building libvorbis with CMake..."

# Configure libogg paths
$liboggIncludeDir = Join-Path $liboggDir "include"
if (-not (Test-Path $liboggIncludeDir)) {
    $liboggIncludeDir = Join-Path $liboggDir "src"
}

# Verify libogg library exists
$oggLibDebugPath = Join-Path $liboggLibDir "ogg-debug.lib"
$oggLibReleasePath = Join-Path $liboggLibDir "ogg-release.lib"

if (-not (Test-Path $oggLibDebugPath)) {
    throw "libogg Debug library not found at $oggLibDebugPath. Please build libogg first."
}
if (-not (Test-Path $oggLibReleasePath)) {
    throw "libogg Release library not found at $oggLibReleasePath. Please build libogg first."
}

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring libvorbis (Debug) with static runtime..."
$cmakeArgsDebug = @(
    "-S", "$libvorbisDir",
    "-B", "$buildDirDebug",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DINSTALL_DOCS=OFF",
    "-DINSTALL_PKG_CONFIG_MODULE=OFF",
    "-DOGG_ROOT=$liboggDir",
    "-DOGG_INCLUDE_DIR=$liboggIncludeDir",
    "-DOGG_LIBRARY=$oggLibDebugPath",
    "-DCMAKE_PREFIX_PATH=$liboggDir"
)

& cmake @cmakeArgsDebug

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

Write-Host "Building libvorbis (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libvorbis Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring libvorbis (Release) with static runtime..."
$cmakeArgsRelease = @(
    "-S", "$libvorbisDir",
    "-B", "$buildDirRelease",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DINSTALL_DOCS=OFF",
    "-DINSTALL_PKG_CONFIG_MODULE=OFF",
    "-DOGG_ROOT=$liboggDir",
    "-DOGG_INCLUDE_DIR=$liboggIncludeDir",
    "-DOGG_LIBRARY=$oggLibReleasePath",
    "-DCMAKE_PREFIX_PATH=$liboggDir"
)

& cmake @cmakeArgsRelease

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

Write-Host "Building libvorbis (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "libvorbis Release build failed with exit code $LASTEXITCODE"
}

Write-Host "libvorbis built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\libvorbis\lib"
$includeDir = Join-Path $root "Engine\Vendor\libvorbis\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy libvorbis libraries
# Note: libvorbis builds as vorbis.lib, vorbisfile.lib, and vorbisenc.lib on Windows
$vorbisLibRelease = Join-Path $buildDirRelease "lib\Release\vorbis.lib"
$vorbisLibDebug = Join-Path $buildDirDebug "lib\Debug\vorbis.lib"
$vorbisfileLibRelease = Join-Path $buildDirRelease "lib\Release\vorbisfile.lib"
$vorbisfileLibDebug = Join-Path $buildDirDebug "lib\Debug\vorbisfile.lib"
$vorbisencLibRelease = Join-Path $buildDirRelease "lib\Release\vorbisenc.lib"
$vorbisencLibDebug = Join-Path $buildDirDebug "lib\Debug\vorbisenc.lib"

# Try alternative paths (check both lib/Release and Release directories)
if (-not (Test-Path $vorbisLibRelease)) {
    $vorbisLibRelease = Join-Path $buildDirRelease "Release\vorbis.lib"
    if (-not (Test-Path $vorbisLibRelease)) {
        $vorbisLibRelease = Join-Path $buildDirRelease "Release\libvorbis.lib"
    }
}
if (-not (Test-Path $vorbisLibDebug)) {
    $vorbisLibDebug = Join-Path $buildDirDebug "Debug\vorbis.lib"
    if (-not (Test-Path $vorbisLibDebug)) {
        $vorbisLibDebug = Join-Path $buildDirDebug "Debug\vorbisd.lib"
        if (-not (Test-Path $vorbisLibDebug)) {
            $vorbisLibDebug = Join-Path $buildDirDebug "Debug\libvorbisd.lib"
        }
    }
}
if (-not (Test-Path $vorbisfileLibRelease)) {
    $vorbisfileLibRelease = Join-Path $buildDirRelease "Release\vorbisfile.lib"
    if (-not (Test-Path $vorbisfileLibRelease)) {
        $vorbisfileLibRelease = Join-Path $buildDirRelease "Release\libvorbisfile.lib"
    }
}
if (-not (Test-Path $vorbisfileLibDebug)) {
    $vorbisfileLibDebug = Join-Path $buildDirDebug "Debug\vorbisfile.lib"
    if (-not (Test-Path $vorbisfileLibDebug)) {
        $vorbisfileLibDebug = Join-Path $buildDirDebug "Debug\vorbisfiled.lib"
        if (-not (Test-Path $vorbisfileLibDebug)) {
            $vorbisfileLibDebug = Join-Path $buildDirDebug "Debug\libvorbisfiled.lib"
        }
    }
}
if (-not (Test-Path $vorbisencLibRelease)) {
    $vorbisencLibRelease = Join-Path $buildDirRelease "Release\vorbisenc.lib"
    if (-not (Test-Path $vorbisencLibRelease)) {
        $vorbisencLibRelease = Join-Path $buildDirRelease "Release\libvorbisenc.lib"
    }
}
if (-not (Test-Path $vorbisencLibDebug)) {
    $vorbisencLibDebug = Join-Path $buildDirDebug "Debug\vorbisenc.lib"
    if (-not (Test-Path $vorbisencLibDebug)) {
        $vorbisencLibDebug = Join-Path $buildDirDebug "Debug\vorbisencd.lib"
        if (-not (Test-Path $vorbisencLibDebug)) {
            $vorbisencLibDebug = Join-Path $buildDirDebug "Debug\libvorbisencd.lib"
        }
    }
}

if (Test-Path $vorbisLibRelease) {
    Copy-Item $vorbisLibRelease (Join-Path $libDir "vorbis-release.lib") -Force
    Write-Host "Copied libvorbis library (Release) to $libDir"
} else {
    Write-Warning "Release library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirRelease -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $vorbisLibDebug) {
    Copy-Item $vorbisLibDebug (Join-Path $libDir "vorbis-debug.lib") -Force
    Write-Host "Copied libvorbis library (Debug) to $libDir"
} else {
    Write-Warning "Debug library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirDebug -Filter "*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $vorbisfileLibRelease) {
    Copy-Item $vorbisfileLibRelease (Join-Path $libDir "vorbisfile-release.lib") -Force
    Write-Host "Copied libvorbisfile library (Release) to $libDir"
}

if (Test-Path $vorbisfileLibDebug) {
    Copy-Item $vorbisfileLibDebug (Join-Path $libDir "vorbisfile-debug.lib") -Force
    Write-Host "Copied libvorbisfile library (Debug) to $libDir"
}

if (Test-Path $vorbisencLibRelease) {
    Copy-Item $vorbisencLibRelease (Join-Path $libDir "vorbisenc-release.lib") -Force
    Write-Host "Copied libvorbisenc library (Release) to $libDir"
} else {
    Write-Warning "vorbisenc Release library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirRelease -Filter "*vorbisenc*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

if (Test-Path $vorbisencLibDebug) {
    Copy-Item $vorbisencLibDebug (Join-Path $libDir "vorbisenc-debug.lib") -Force
    Write-Host "Copied libvorbisenc library (Debug) to $libDir"
} else {
    Write-Warning "vorbisenc Debug library not found. Searching..."
    Get-ChildItem -Recurse -Path $buildDirDebug -Filter "*vorbisenc*.lib" | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}

# Copy headers (libvorbis headers are in include/vorbis/)
$headerSourceDir = Join-Path $libvorbisDir "include"
$vorbisHeaderDir = Join-Path $headerSourceDir "vorbis"
$destVorbisDir = Join-Path $includeDir "vorbis"

New-Item -ItemType Directory -Force -Path $includeDir | Out-Null
New-Item -ItemType Directory -Force -Path $destVorbisDir | Out-Null

# Check if headers exist in repository
if ((Test-Path $vorbisHeaderDir) -and (Get-ChildItem -Path $vorbisHeaderDir -Filter "*.h" -ErrorAction SilentlyContinue)) {
    # Check if source and destination are the same (to avoid copying to itself)
    $sourcePath = (Resolve-Path $vorbisHeaderDir -ErrorAction SilentlyContinue).Path
    $destPath = (Resolve-Path $destVorbisDir -ErrorAction SilentlyContinue).Path
    
    if ($sourcePath -and $destPath -and $sourcePath -eq $destPath) {
        Write-Host "Headers already in correct location: $destVorbisDir"
    } else {
        if (Test-Path $destVorbisDir) {
            Remove-Item $destVorbisDir -Recurse -Force
            New-Item -ItemType Directory -Force -Path $destVorbisDir | Out-Null
        }
        Copy-Item $vorbisHeaderDir $destVorbisDir -Recurse -Force
        Write-Host "Copied vorbis headers to $includeDir"
    }
} else {
    Write-Warning "vorbis header directory not found at $vorbisHeaderDir. Downloading headers from GitHub..."
    
    # Download headers directly from GitHub repository
    $headers = @(
        @{ Name = "codec.h"; Url = "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/codec.h" }
        @{ Name = "vorbisenc.h"; Url = "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/vorbisenc.h" }
        @{ Name = "vorbisfile.h"; Url = "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/vorbisfile.h" }
    )
    
    $downloaded = $false
    foreach ($header in $headers) {
        try {
            $headerPath = Join-Path $destVorbisDir $header.Name
            Write-Host "Downloading $($header.Name)..." -ForegroundColor Yellow
            Invoke-WebRequest -Uri $header.Url -OutFile $headerPath -UseBasicParsing -ErrorAction Stop
            $downloaded = $true
            Write-Host "Downloaded $($header.Name) to $headerPath"
        } catch {
            Write-Warning "Failed to download $($header.Name): $($_.Exception.Message)"
        }
    }
    
    if (-not $downloaded) {
        # Fallback: search for headers in the repository
        Write-Warning "Download failed. Searching repository for headers..."
        $codecHeader = Get-ChildItem -Recurse -Path $libvorbisDir -Filter "codec.h" -ErrorAction SilentlyContinue | Select-Object -First 1
        $vorbisencHeader = Get-ChildItem -Recurse -Path $libvorbisDir -Filter "vorbisenc.h" -ErrorAction SilentlyContinue | Select-Object -First 1
        $vorbisfileHeader = Get-ChildItem -Recurse -Path $libvorbisDir -Filter "vorbisfile.h" -ErrorAction SilentlyContinue | Select-Object -First 1
        
        if ($codecHeader -and $vorbisencHeader -and $vorbisfileHeader) {
            Copy-Item $codecHeader.FullName (Join-Path $destVorbisDir "codec.h") -Force -ErrorAction SilentlyContinue
            Copy-Item $vorbisencHeader.FullName (Join-Path $destVorbisDir "vorbisenc.h") -Force -ErrorAction SilentlyContinue
            Copy-Item $vorbisfileHeader.FullName (Join-Path $destVorbisDir "vorbisfile.h") -Force -ErrorAction SilentlyContinue
            Write-Host "Copied vorbis headers from repository to $includeDir"
        } else {
            throw "Failed to find or download vorbis header files. Please ensure libvorbis repository is properly cloned and headers are available."
        }
    }
}

Write-Host "libvorbis build complete!" -ForegroundColor Green

