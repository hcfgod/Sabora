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
$msdfAtlasGenDir = Join-Path $root "Engine\Vendor\msdf-atlas-gen"
$buildDir = Join-Path $msdfAtlasGenDir "build"

# Check if msdf-atlas-gen directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $msdfAtlasGenDir "CMakeLists.txt"))) {
    Write-Host "msdf-atlas-gen not found. Cloning msdf-atlas-gen..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone msdf-atlas-gen if it doesn't exist (with submodules)
    if (-not (Test-Path $msdfAtlasGenDir)) {
        Push-Location $vendorDir
        try {
            git clone --recursive --depth 1 https://github.com/Chlumsky/msdf-atlas-gen.git msdf-atlas-gen
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists, make sure submodules are initialized
        Push-Location $msdfAtlasGenDir
        try {
            git submodule update --init --recursive
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $msdfAtlasGenDir "CMakeLists.txt"))) {
        throw "msdf-atlas-gen CMakeLists.txt not found after cloning. msdf-atlas-gen directory: $msdfAtlasGenDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $msdfAtlasGenDir "build-debug"
$buildDirRelease = Join-Path $msdfAtlasGenDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

# Build Freetype first (required dependency)
$freetypeDir = Join-Path $root "Engine\Vendor\freetype"
$freetypeBuildDirDebug = Join-Path $freetypeDir "build-debug"
$freetypeBuildDirRelease = Join-Path $freetypeDir "build-release"

if (-not (Test-Path (Join-Path $freetypeDir "CMakeLists.txt"))) {
    Write-Host "Freetype not found. Cloning Freetype..." -ForegroundColor Yellow
    
    $vendorDir = Join-Path $root "Engine\Vendor"
    if (-not (Test-Path $freetypeDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/freetype/freetype.git freetype
        }
        finally {
            Pop-Location
        }
    }
}

# Patch msdfgen CMakeLists.txt to disable Skia by default
if (Test-Path "$msdfAtlasGenDir\msdfgen\CMakeLists.txt") {
    Write-Host "Patching msdfgen CMakeLists.txt to disable Skia..." -ForegroundColor Yellow
    $msdfgenCMake = Get-Content "$msdfAtlasGenDir\msdfgen\CMakeLists.txt" -Raw
    
    # Change the default from ON to OFF
    $msdfgenCMake = $msdfgenCMake -replace 'option\(MSDFGEN_USE_SKIA\s+"Build with the Skia library"\s+ON\)', 'option(MSDFGEN_USE_SKIA "Build with the Skia library" OFF)'
    
    # Replace all if(MSDFGEN_USE_SKIA) with if(FALSE) to disable the Skia block
    $msdfgenCMake = $msdfgenCMake -replace 'if\(MSDFGEN_USE_SKIA\)', 'if(FALSE)  # Disabled: MSDFGEN_USE_SKIA'
    
    Set-Content -Path "$msdfAtlasGenDir\msdfgen\CMakeLists.txt" -Value $msdfgenCMake -NoNewline
    Write-Host "msdfgen CMakeLists.txt patched successfully!" -ForegroundColor Green
}

# Build Freetype Debug
if (-not (Test-Path "$freetypeBuildDirDebug\Debug\freetype.lib")) {
    Write-Host "Building Freetype (Debug)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $freetypeBuildDirDebug | Out-Null
    
    & cmake -S "$freetypeDir" -B "$freetypeBuildDirDebug" `
        -DCMAKE_BUILD_TYPE=Debug `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DFT_DISABLE_BZIP2=ON `
        -DFT_DISABLE_BROTLI=ON `
        -DFT_DISABLE_PNG=ON `
        -DFT_DISABLE_HARFBUZZ=ON `
        -DFT_DISABLE_ZLIB=ON
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $freetypeBuildDirDebug -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and ($propGroup.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $propGroup.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $propGroup.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and ($itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $freetypeBuildDirDebug --config Debug --parallel
}

# Build Freetype Release
if (-not (Test-Path "$freetypeBuildDirRelease\Release\freetype.lib")) {
    Write-Host "Building Freetype (Release)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $freetypeBuildDirRelease | Out-Null
    
    & cmake -S "$freetypeDir" -B "$freetypeBuildDirRelease" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DFT_DISABLE_BZIP2=ON `
        -DFT_DISABLE_BROTLI=ON `
        -DFT_DISABLE_PNG=ON `
        -DFT_DISABLE_HARFBUZZ=ON `
        -DFT_DISABLE_ZLIB=ON
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $freetypeBuildDirRelease -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and $propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL") {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $freetypeBuildDirRelease --config Release --parallel
}

# Build zlib (required by libpng)
$zlibDir = Join-Path $root "Engine\Vendor\zlib"
$zlibBuildDirDebug = Join-Path $zlibDir "build-debug"
$zlibBuildDirRelease = Join-Path $zlibDir "build-release"

if (-not (Test-Path (Join-Path $zlibDir "CMakeLists.txt"))) {
    Write-Host "zlib not found. Cloning zlib..." -ForegroundColor Yellow
    
    $vendorDir = Join-Path $root "Engine\Vendor"
    if (-not (Test-Path $zlibDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/madler/zlib.git zlib
        }
        finally {
            Pop-Location
        }
    }
}

# Build zlib Debug
if (-not (Test-Path "$zlibBuildDirDebug\Debug\zsd.lib")) {
    Write-Host "Building zlib (Debug)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $zlibBuildDirDebug | Out-Null
    
    & cmake -S "$zlibDir" -B "$zlibBuildDirDebug" `
        -DCMAKE_BUILD_TYPE=Debug `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $zlibBuildDirDebug -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and ($propGroup.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $propGroup.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $propGroup.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and ($itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $zlibBuildDirDebug --config Debug --parallel
}

# Build zlib Release
if (-not (Test-Path "$zlibBuildDirRelease\Release\zs.lib")) {
    Write-Host "Building zlib (Release)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $zlibBuildDirRelease | Out-Null
    
    & cmake -S "$zlibDir" -B "$zlibBuildDirRelease" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $zlibBuildDirRelease -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and $propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL") {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $zlibBuildDirRelease --config Release --parallel
}

# Build libpng (required by msdfgen submodule)
$pngDir = Join-Path $root "Engine\Vendor\libpng"
$pngBuildDirDebug = Join-Path $pngDir "build-debug"
$pngBuildDirRelease = Join-Path $pngDir "build-release"

if (-not (Test-Path (Join-Path $pngDir "CMakeLists.txt"))) {
    Write-Host "libpng not found. Cloning libpng..." -ForegroundColor Yellow
    
    $vendorDir = Join-Path $root "Engine\Vendor"
    if (-not (Test-Path $pngDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/glennrp/libpng.git libpng
        }
        finally {
            Pop-Location
        }
    }
}

# Build libpng Debug
if (-not (Test-Path "$pngBuildDirDebug\Debug\libpng18_staticd.lib")) {
    Write-Host "Building libpng (Debug)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $pngBuildDirDebug | Out-Null
    
    & cmake -S "$pngDir" -B "$pngBuildDirDebug" `
        -DCMAKE_BUILD_TYPE=Debug `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DPNG_SHARED=OFF `
        -DPNG_STATIC=ON `
        -DPNG_TESTS=OFF `
        -DZLIB_ROOT="$zlibDir" `
        -DZLIB_LIBRARY_DEBUG="$zlibBuildDirDebug\Debug\zsd.lib" `
        -DZLIB_LIBRARY_RELEASE="$zlibBuildDirRelease\Release\zs.lib" `
        -DZLIB_INCLUDE_DIR="$zlibDir"
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $pngBuildDirDebug -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and ($propGroup.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $propGroup.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $propGroup.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and ($itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL")) {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $pngBuildDirDebug --config Debug --parallel
}

# Build libpng Release
if (-not (Test-Path "$pngBuildDirRelease\Release\libpng18_static.lib")) {
    Write-Host "Building libpng (Release)..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $pngBuildDirRelease | Out-Null
    
    & cmake -S "$pngDir" -B "$pngBuildDirRelease" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DPNG_SHARED=OFF `
        -DPNG_STATIC=ON `
        -DPNG_TESTS=OFF `
        -DZLIB_ROOT="$zlibDir" `
        -DZLIB_LIBRARY_DEBUG="$zlibBuildDirDebug\Debug\zsd.lib" `
        -DZLIB_LIBRARY_RELEASE="$zlibBuildDirRelease\Release\zs.lib" `
        -DZLIB_INCLUDE_DIR="$zlibDir"
    
    # Fix runtime library in generated files
    $vcxprojFiles = Get-ChildItem -Path $pngBuildDirRelease -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
    foreach ($projFile in $vcxprojFiles) {
        [xml]$xml = Get-Content $projFile.FullName
        $modified = $false
        foreach ($propGroup in $xml.Project.PropertyGroup) {
            if ($propGroup.RuntimeLibrary -and $propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        foreach ($itemDefGroup in $xml.Project.ItemDefinitionGroup) {
            if ($itemDefGroup.ClCompile.RuntimeLibrary -and $itemDefGroup.ClCompile.RuntimeLibrary -eq "MultiThreadedDLL") {
                $itemDefGroup.ClCompile.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
        if ($modified) { $xml.Save($projFile.FullName) }
    }
    
    & cmake --build $pngBuildDirRelease --config Release --parallel
}

Write-Host "Building msdf-atlas-gen with CMake..."

# Copy pnglibconf.h.prebuilt to pnglibconf.h if it doesn't exist
# This is needed because png.h includes pnglibconf.h
if ((Test-Path "$pngDir\pnglibconf.h.prebuilt") -and (-not (Test-Path "$pngDir\pnglibconf.h"))) {
    Write-Host "Copying pnglibconf.h.prebuilt to pnglibconf.h..." -ForegroundColor Yellow
    Copy-Item "$pngDir\pnglibconf.h.prebuilt" "$pngDir\pnglibconf.h"
}

# Create a minimal dummy vcpkg toolchain file to bypass the check
$dummyVcpkgToolchain = Join-Path $msdfAtlasGenDir "dummy_vcpkg.cmake"
@"
# Dummy vcpkg toolchain file to bypass vcpkg check
# This file exists only to satisfy msdf-atlas-gen's CMakeLists.txt check
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
set(VCPKG_CRT_LINKAGE "static")
set(VCPKG_LIBRARY_LINKAGE "static")
"@ | Out-File -FilePath $dummyVcpkgToolchain -Encoding UTF8

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring msdf-atlas-gen (Debug) with static runtime..."

# Set MSDFGEN_USE_SKIA as a cache variable before configuration
# This ensures it's available to the msdfgen submodule
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
$initialCache = Join-Path $buildDirDebug "initial_cache.cmake"
@"
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "Build with the Skia library" FORCE)
"@ | Out-File -FilePath $initialCache -Encoding ASCII

& cmake -S "$msdfAtlasGenDir" -B "$buildDirDebug" `
    -C "$initialCache" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_CXX_STANDARD=17 `
    -DCMAKE_CXX_STANDARD_REQUIRED=ON `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DCMAKE_TOOLCHAIN_FILE="$dummyVcpkgToolchain" `
    -DVCPKG_TARGET_TRIPLET="x64-windows-static" `
    -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF `
    -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF `
    -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON `
    -DMSDFGEN_USE_SKIA:BOOL=OFF `
    -DFREETYPE_DIR="$freetypeDir" `
    -DFREETYPE_LIBRARY_DEBUG="$freetypeBuildDirDebug\Debug\freetyped.lib" `
    -DFREETYPE_LIBRARY_RELEASE="$freetypeBuildDirRelease\Release\freetype.lib" `
    -DFREETYPE_INCLUDE_DIRS="$freetypeDir\include" `
    -DPNG_DIR="$pngDir" `
    -DPNG_LIBRARY_DEBUG="$pngBuildDirDebug\Debug\libpng18_staticd.lib" `
    -DPNG_LIBRARY_RELEASE="$pngBuildDirRelease\Release\libpng18_static.lib" `
    -DPNG_PNG_INCLUDE_DIR="$pngDir" `
    -DZLIB_ROOT="$zlibDir" `
    -DZLIB_LIBRARY_DEBUG="$zlibBuildDirDebug\Debug\zsd.lib" `
    -DZLIB_LIBRARY_RELEASE="$zlibBuildDirRelease\Release\zs.lib" `
    -DZLIB_INCLUDE_DIR="$zlibDir" `
    -DCMAKE_PREFIX_PATH="$pngDir;$zlibDir"

if ($LASTEXITCODE -ne 0) {
    Write-Warning "CMake Debug configuration failed. Trying with explicit library paths..."
    # Try setting as cache variables that submodules can find
    & cmake -S "$msdfAtlasGenDir" -B "$buildDirDebug" `
        -DCMAKE_BUILD_TYPE=Debug `
        -DCMAKE_CXX_STANDARD=17 `
        -DCMAKE_CXX_STANDARD_REQUIRED=ON `
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DCMAKE_TOOLCHAIN_FILE="$dummyVcpkgToolchain" `
        -DVCPKG_TARGET_TRIPLET="x64-windows-static" `
        -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF `
        -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF `
        -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON `
        -DMSDFGEN_USE_SKIA:BOOL=OFF `
        -DFREETYPE_DIR="$freetypeDir" `
        -DFREETYPE_LIBRARY_DEBUG="$freetypeBuildDirDebug\Debug\freetyped.lib" `
        -DFREETYPE_LIBRARY_RELEASE="$freetypeBuildDirRelease\Release\freetype.lib" `
        -DFREETYPE_INCLUDE_DIRS="$freetypeDir\include" `
        -DPNG_LIBRARY="$pngBuildDirDebug\Debug\libpng18_staticd.lib" `
        -DPNG_PNG_INCLUDE_DIR="$pngDir" `
        -DZLIB_LIBRARY="$zlibBuildDirDebug\Debug\zsd.lib" `
        -DZLIB_INCLUDE_DIR="$zlibDir" `
        -DCMAKE_PREFIX_PATH="$pngDir;$zlibDir"
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake Debug configuration failed with exit code $LASTEXITCODE"
    }
}

# Force static runtime library in generated project files
# CMake's CMAKE_MSVC_RUNTIME_LIBRARY doesn't always work with Visual Studio generators
Write-Host "Fixing runtime library settings in Debug project files..."
$vcxprojFiles = Get-ChildItem -Path $buildDirDebug -Filter "*.vcxproj" -Recurse
foreach ($projFile in $vcxprojFiles) {
    [xml]$xml = Get-Content $projFile.FullName
    $modified = $false
    
    # Find all PropertyGroup elements and fix RuntimeLibrary settings
    foreach ($propGroup in $xml.Project.PropertyGroup) {
        if ($propGroup.RuntimeLibrary) {
            # For Debug build directory, force MultiThreadedDebug (MTd)
            if ($propGroup.RuntimeLibrary -eq "MultiThreadedDebugDLL" -or 
                $propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreadedDebug"
                $modified = $true
            }
        }
    }
    
    # Also check ItemDefinitionGroup (for per-file settings)
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

Write-Host "Building msdf-atlas-gen (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "msdf-atlas-gen Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring msdf-atlas-gen (Release) with static runtime..."

# Set MSDFGEN_USE_SKIA as a cache variable before configuration
# This ensures it's available to the msdfgen submodule
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null
$initialCacheRelease = Join-Path $buildDirRelease "initial_cache.cmake"
@"
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "Build with the Skia library" FORCE)
"@ | Out-File -FilePath $initialCacheRelease -Encoding ASCII

& cmake -S "$msdfAtlasGenDir" -B "$buildDirRelease" `
    -C "$initialCacheRelease" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_CXX_STANDARD=17 `
    -DCMAKE_CXX_STANDARD_REQUIRED=ON `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DCMAKE_TOOLCHAIN_FILE="$dummyVcpkgToolchain" `
    -DVCPKG_TARGET_TRIPLET="x64-windows-static" `
    -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF `
    -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF `
    -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON `
    -DMSDFGEN_USE_SKIA:BOOL=OFF `
    -DFREETYPE_DIR="$freetypeDir" `
    -DFREETYPE_LIBRARY_DEBUG="$freetypeBuildDirDebug\Debug\freetyped.lib" `
    -DFREETYPE_LIBRARY_RELEASE="$freetypeBuildDirRelease\Release\freetype.lib" `
    -DFREETYPE_INCLUDE_DIRS="$freetypeDir\include" `
    -DPNG_DIR="$pngDir" `
    -DPNG_LIBRARY_DEBUG="$pngBuildDirDebug\Debug\libpng18_staticd.lib" `
    -DPNG_LIBRARY_RELEASE="$pngBuildDirRelease\Release\libpng18_static.lib" `
    -DPNG_PNG_INCLUDE_DIR="$pngDir" `
    -DZLIB_ROOT="$zlibDir" `
    -DZLIB_LIBRARY_DEBUG="$zlibBuildDirDebug\Debug\zsd.lib" `
    -DZLIB_LIBRARY_RELEASE="$zlibBuildDirRelease\Release\zs.lib" `
    -DZLIB_INCLUDE_DIR="$zlibDir" `
    -DCMAKE_PREFIX_PATH="$pngDir;$zlibDir"

if ($LASTEXITCODE -ne 0) {
    Write-Warning "CMake Release configuration failed. Trying with explicit library paths..."
    # Try setting as cache variables that submodules can find
    & cmake -S "$msdfAtlasGenDir" -B "$buildDirRelease" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_CXX_STANDARD=17 `
        -DCMAKE_CXX_STANDARD_REQUIRED=ON `
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
        -DCMAKE_TOOLCHAIN_FILE="$dummyVcpkgToolchain" `
        -DVCPKG_TARGET_TRIPLET="x64-windows-static" `
        -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF `
        -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF `
        -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON `
        -DMSDFGEN_USE_SKIA:BOOL=OFF `
        -DFREETYPE_DIR="$freetypeDir" `
        -DFREETYPE_LIBRARY_DEBUG="$freetypeBuildDirDebug\Debug\freetyped.lib" `
        -DFREETYPE_LIBRARY_RELEASE="$freetypeBuildDirRelease\Release\freetype.lib" `
        -DFREETYPE_INCLUDE_DIRS="$freetypeDir\include" `
        -DPNG_LIBRARY="$pngBuildDirRelease\Release\libpng18_static.lib" `
        -DPNG_PNG_INCLUDE_DIR="$pngDir" `
        -DZLIB_LIBRARY="$zlibBuildDirRelease\Release\zs.lib" `
        -DZLIB_INCLUDE_DIR="$zlibDir" `
        -DCMAKE_PREFIX_PATH="$pngDir;$zlibDir"
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake Release configuration failed with exit code $LASTEXITCODE"
    }
}

# Force static runtime library in generated project files
Write-Host "Fixing runtime library settings in Release project files..."
$vcxprojFiles = Get-ChildItem -Path $buildDirRelease -Filter "*.vcxproj" -Recurse
foreach ($projFile in $vcxprojFiles) {
    [xml]$xml = Get-Content $projFile.FullName
    $modified = $false
    
    # Find all PropertyGroup elements and fix RuntimeLibrary settings
    foreach ($propGroup in $xml.Project.PropertyGroup) {
        if ($propGroup.RuntimeLibrary) {
            # For Release build directory, force MultiThreaded (MT)
            if ($propGroup.RuntimeLibrary -eq "MultiThreadedDLL") {
                $propGroup.RuntimeLibrary = "MultiThreaded"
                $modified = $true
            }
        }
    }
    
    # Also check ItemDefinitionGroup (for per-file settings)
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

Write-Host "Building msdf-atlas-gen (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "msdf-atlas-gen Release build failed with exit code $LASTEXITCODE"
}

Write-Host "msdf-atlas-gen built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\msdf-atlas-gen\lib"
$includeDir = Join-Path $root "Engine\Vendor\msdf-atlas-gen\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy msdf-atlas-gen library
# Note: The library name is msdf-atlas-gen.lib for both Debug and Release
$msdfAtlasGenLibRelease = Join-Path $buildDirRelease "Release\msdf-atlas-gen.lib"
$msdfAtlasGenLibDebug = Join-Path $buildDirDebug "Debug\msdf-atlas-gen.lib"

if (Test-Path $msdfAtlasGenLibRelease) {
    Copy-Item $msdfAtlasGenLibRelease (Join-Path $libDir "msdf-atlas-gen-release.lib") -Force
    Write-Host "Copied msdf-atlas-gen library (Release) to $libDir"
} else {
    Write-Warning "Release library not found at expected path: $msdfAtlasGenLibRelease"
}

if (Test-Path $msdfAtlasGenLibDebug) {
    Copy-Item $msdfAtlasGenLibDebug (Join-Path $libDir "msdf-atlas-gen-debug.lib") -Force
    Write-Host "Copied msdf-atlas-gen library (Debug) to $libDir"
} else {
    Write-Warning "Debug library not found at expected path: $msdfAtlasGenLibDebug"
}

# Copy msdfgen-core and msdfgen-ext libraries (dependencies of msdf-atlas-gen)
$msdfgenCoreLibRelease = Join-Path $buildDirRelease "msdfgen\Release\msdfgen-core.lib"
$msdfgenExtLibRelease = Join-Path $buildDirRelease "msdfgen\Release\msdfgen-ext.lib"
$msdfgenCoreLibDebug = Join-Path $buildDirDebug "msdfgen\Debug\msdfgen-core.lib"
$msdfgenExtLibDebug = Join-Path $buildDirDebug "msdfgen\Debug\msdfgen-ext.lib"

if (Test-Path $msdfgenCoreLibRelease) {
    Copy-Item $msdfgenCoreLibRelease (Join-Path $libDir "msdfgen-core-release.lib") -Force
    Write-Host "Copied msdfgen-core library (Release) to $libDir"
}
if (Test-Path $msdfgenExtLibRelease) {
    Copy-Item $msdfgenExtLibRelease (Join-Path $libDir "msdfgen-ext-release.lib") -Force
    Write-Host "Copied msdfgen-ext library (Release) to $libDir"
}
if (Test-Path $msdfgenCoreLibDebug) {
    Copy-Item $msdfgenCoreLibDebug (Join-Path $libDir "msdfgen-core-debug.lib") -Force
    Write-Host "Copied msdfgen-core library (Debug) to $libDir"
}
if (Test-Path $msdfgenExtLibDebug) {
    Copy-Item $msdfgenExtLibDebug (Join-Path $libDir "msdfgen-ext-debug.lib") -Force
    Write-Host "Copied msdfgen-ext library (Debug) to $libDir"
}

# Copy Freetype libraries (dependency of msdfgen-ext)
$freetypeDir = Join-Path $vendorDir "freetype"
$freetypeBuildDirDebug = Join-Path $freetypeDir "build-debug"
$freetypeBuildDirRelease = Join-Path $freetypeDir "build-release"
$freetypeLibDebug = Join-Path $freetypeBuildDirDebug "Debug\freetyped.lib"
$freetypeLibRelease = Join-Path $freetypeBuildDirRelease "Release\freetype.lib"

if (Test-Path $freetypeLibRelease) {
    Copy-Item $freetypeLibRelease (Join-Path $libDir "freetype-release.lib") -Force
    Write-Host "Copied freetype library (Release) to $libDir"
}
if (Test-Path $freetypeLibDebug) {
    Copy-Item $freetypeLibDebug (Join-Path $libDir "freetype-debug.lib") -Force
    Write-Host "Copied freetype library (Debug) to $libDir"
}

# If libraries weren't found, list what we have
$foundLibs = Get-ChildItem -Path $libDir -Filter "*.lib" -ErrorAction SilentlyContinue
if ($foundLibs.Count -eq 0) {
    Write-Warning "No libraries found. Searching build directories..."
    Write-Host "Debug build directory:"
    Get-ChildItem -Recurse -Path $buildDirDebug -Name "*.lib" | ForEach-Object { Write-Host "Found lib: $_" }
    Write-Host "Release build directory:"
    Get-ChildItem -Recurse -Path $buildDirRelease -Name "*.lib" | ForEach-Object { Write-Host "Found lib: $_" }
}

# Copy headers (msdf-atlas-gen headers are typically in include/msdf-atlas-gen/)
$headerSourceDir = Join-Path $msdfAtlasGenDir "include"
if (Test-Path $headerSourceDir) {
    # Copy the entire include directory structure
    Copy-Item -Path "$headerSourceDir\*" -Destination $includeDir -Recurse -Force
    Write-Host "Copied msdf-atlas-gen headers to $includeDir"
} else {
    # Try alternative locations
    $altHeaderDirs = @(
        Join-Path $msdfAtlasGenDir "msdf-atlas-gen",
        Join-Path $msdfAtlasGenDir "src"
    )
    foreach ($altDir in $altHeaderDirs) {
        if (Test-Path $altDir) {
            Copy-Item -Path "$altDir\*.h" -Destination $includeDir -Recurse -Force -ErrorAction SilentlyContinue
            Copy-Item -Path "$altDir\*.hpp" -Destination $includeDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-Host "Copied msdf-atlas-gen headers to $includeDir (from alternative location)"
}

# Generate msdfgen-config.h file (required by msdfgen headers)
$msdfgenConfigDir = Join-Path $msdfAtlasGenDir "msdfgen\msdfgen"
New-Item -ItemType Directory -Force -Path $msdfgenConfigDir | Out-Null

$msdfgenConfigContent = @"
#pragma once

// Minimal msdfgen-config.h for header-only usage
// This file is typically generated by CMake, but we provide a minimal version
// for when using msdf-atlas-gen as a pre-built library

#define MSDFGEN_PUBLIC
#define MSDFGEN_EXT_PUBLIC

// Version information from vcpkg.json
#define MSDFGEN_VERSION "1.13.0"
#define MSDFGEN_VERSION_MAJOR 1
#define MSDFGEN_VERSION_MINOR 13
#define MSDFGEN_VERSION_REVISION 0
#define MSDFGEN_COPYRIGHT_YEAR 2025
"@

$msdfgenConfigPath = Join-Path $msdfgenConfigDir "msdfgen-config.h"
Set-Content -Path $msdfgenConfigPath -Value $msdfgenConfigContent -NoNewline
Write-Host "Generated msdfgen-config.h at $msdfgenConfigPath"

Write-Host "msdf-atlas-gen build complete!" -ForegroundColor Green

