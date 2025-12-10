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
$spirvCrossDir = Join-Path $root "Engine\Vendor\SPIRV-Cross"
$buildDir = Join-Path $spirvCrossDir "build"

# Check if SPIRV-Cross directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $spirvCrossDir "CMakeLists.txt"))) {
    Write-Host "SPIRV-Cross not found. Cloning SPIRV-Cross..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone SPIRV-Cross if it doesn't exist
    if (-not (Test-Path $spirvCrossDir)) {
        Push-Location $vendorDir
        try {
            git clone --depth 1 https://github.com/KhronosGroup/SPIRV-Cross.git SPIRV-Cross
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $spirvCrossDir "CMakeLists.txt"))) {
        throw "SPIRV-Cross CMakeLists.txt not found after cloning. SPIRV-Cross directory: $spirvCrossDir"
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $spirvCrossDir "build-debug"
$buildDirRelease = Join-Path $spirvCrossDir "build-release"
New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null

Write-Host "Building SPIRV-Cross with CMake..."

# Configure and build Debug configuration with static debug runtime
Write-Host "Configuring SPIRV-Cross (Debug) with static runtime..."
& cmake -S "$spirvCrossDir" -B "$buildDirDebug" `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_CXX_STANDARD=17 `
    -DCMAKE_CXX_STANDARD_REQUIRED=ON `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DSPIRV_CROSS_CLI=OFF `
    -DSPIRV_CROSS_ENABLE_TESTS=OFF `
    -DSPIRV_CROSS_SHARED=OFF `
    -DSPIRV_CROSS_STATIC=ON `
    -DSPIRV_CROSS_ENABLE_CPP=ON `
    -DSPIRV_CROSS_ENABLE_GLSL=ON `
    -DSPIRV_CROSS_ENABLE_HLSL=ON `
    -DSPIRV_CROSS_ENABLE_MSL=ON `
    -DSPIRV_CROSS_ENABLE_REFLECT=ON `
    -DSPIRV_CROSS_ENABLE_UTIL=ON

if ($LASTEXITCODE -ne 0) {
    throw "CMake Debug configuration failed with exit code $LASTEXITCODE"
}

# Force static runtime library in generated project files
# CMake's CMAKE_MSVC_RUNTIME_LIBRARY doesn't always work with Visual Studio generators
# We need to manually fix the .vcxproj files after CMake generation
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

Write-Host "Building SPIRV-Cross (Debug)..."
& cmake --build $buildDirDebug --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "SPIRV-Cross Debug build failed with exit code $LASTEXITCODE"
}

# Configure and build Release configuration with static release runtime
Write-Host "Configuring SPIRV-Cross (Release) with static runtime..."
& cmake -S "$spirvCrossDir" -B "$buildDirRelease" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_CXX_STANDARD=17 `
    -DCMAKE_CXX_STANDARD_REQUIRED=ON `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DSPIRV_CROSS_CLI=OFF `
    -DSPIRV_CROSS_ENABLE_TESTS=OFF `
    -DSPIRV_CROSS_SHARED=OFF `
    -DSPIRV_CROSS_STATIC=ON `
    -DSPIRV_CROSS_ENABLE_CPP=ON `
    -DSPIRV_CROSS_ENABLE_GLSL=ON `
    -DSPIRV_CROSS_ENABLE_HLSL=ON `
    -DSPIRV_CROSS_ENABLE_MSL=ON `
    -DSPIRV_CROSS_ENABLE_REFLECT=ON `
    -DSPIRV_CROSS_ENABLE_UTIL=ON

if ($LASTEXITCODE -ne 0) {
    throw "CMake Release configuration failed with exit code $LASTEXITCODE"
}

# Force static runtime library in generated project files
# CMake's CMAKE_MSVC_RUNTIME_LIBRARY doesn't always work with Visual Studio generators
# We need to manually fix the .vcxproj files after CMake generation
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

Write-Host "Building SPIRV-Cross (Release)..."
& cmake --build $buildDirRelease --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "SPIRV-Cross Release build failed with exit code $LASTEXITCODE"
}

Write-Host "SPIRV-Cross built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\SPIRV-Cross\lib"
$includeDir = Join-Path $root "Engine\Vendor\SPIRV-Cross\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy SPIRV-Cross core library
# Note: Windows Debug builds use 'd' suffix (e.g., spirv-cross-cored.lib)
$spirvCrossCoreLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-core.lib"
$spirvCrossCoreLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-cored.lib"

if (Test-Path $spirvCrossCoreLibRelease) {
    Copy-Item $spirvCrossCoreLibRelease (Join-Path $libDir "spirv-cross-core-release.lib") -Force
    Write-Host "Copied spirv-cross-core library (Release) to $libDir"
}

if (Test-Path $spirvCrossCoreLibDebug) {
    Copy-Item $spirvCrossCoreLibDebug (Join-Path $libDir "spirv-cross-core-debug.lib") -Force
    Write-Host "Copied spirv-cross-core library (Debug) to $libDir"
}

# Copy SPIRV-Cross GLSL library
$spirvCrossGlslLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-glsl.lib"
$spirvCrossGlslLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-glsld.lib"

if (Test-Path $spirvCrossGlslLibRelease) {
    Copy-Item $spirvCrossGlslLibRelease (Join-Path $libDir "spirv-cross-glsl-release.lib") -Force
    Write-Host "Copied spirv-cross-glsl library (Release) to $libDir"
}

if (Test-Path $spirvCrossGlslLibDebug) {
    Copy-Item $spirvCrossGlslLibDebug (Join-Path $libDir "spirv-cross-glsl-debug.lib") -Force
    Write-Host "Copied spirv-cross-glsl library (Debug) to $libDir"
}

# Copy SPIRV-Cross HLSL library
$spirvCrossHlslLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-hlsl.lib"
$spirvCrossHlslLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-hlsld.lib"

if (Test-Path $spirvCrossHlslLibRelease) {
    Copy-Item $spirvCrossHlslLibRelease (Join-Path $libDir "spirv-cross-hlsl-release.lib") -Force
    Write-Host "Copied spirv-cross-hlsl library (Release) to $libDir"
}

if (Test-Path $spirvCrossHlslLibDebug) {
    Copy-Item $spirvCrossHlslLibDebug (Join-Path $libDir "spirv-cross-hlsl-debug.lib") -Force
    Write-Host "Copied spirv-cross-hlsl library (Debug) to $libDir"
}

# Copy SPIRV-Cross MSL library
$spirvCrossMslLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-msl.lib"
$spirvCrossMslLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-msld.lib"

if (Test-Path $spirvCrossMslLibRelease) {
    Copy-Item $spirvCrossMslLibRelease (Join-Path $libDir "spirv-cross-msl-release.lib") -Force
    Write-Host "Copied spirv-cross-msl library (Release) to $libDir"
}

if (Test-Path $spirvCrossMslLibDebug) {
    Copy-Item $spirvCrossMslLibDebug (Join-Path $libDir "spirv-cross-msl-debug.lib") -Force
    Write-Host "Copied spirv-cross-msl library (Debug) to $libDir"
}

# Copy SPIRV-Cross CPP library
$spirvCrossCppLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-cpp.lib"
$spirvCrossCppLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-cppd.lib"

if (Test-Path $spirvCrossCppLibRelease) {
    Copy-Item $spirvCrossCppLibRelease (Join-Path $libDir "spirv-cross-cpp-release.lib") -Force
    Write-Host "Copied spirv-cross-cpp library (Release) to $libDir"
}

if (Test-Path $spirvCrossCppLibDebug) {
    Copy-Item $spirvCrossCppLibDebug (Join-Path $libDir "spirv-cross-cpp-debug.lib") -Force
    Write-Host "Copied spirv-cross-cpp library (Debug) to $libDir"
}

# Copy SPIRV-Cross reflect library
$spirvCrossReflectLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-reflect.lib"
$spirvCrossReflectLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-reflectd.lib"

if (Test-Path $spirvCrossReflectLibRelease) {
    Copy-Item $spirvCrossReflectLibRelease (Join-Path $libDir "spirv-cross-reflect-release.lib") -Force
    Write-Host "Copied spirv-cross-reflect library (Release) to $libDir"
}

if (Test-Path $spirvCrossReflectLibDebug) {
    Copy-Item $spirvCrossReflectLibDebug (Join-Path $libDir "spirv-cross-reflect-debug.lib") -Force
    Write-Host "Copied spirv-cross-reflect library (Debug) to $libDir"
}

# Copy SPIRV-Cross util library
$spirvCrossUtilLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-util.lib"
$spirvCrossUtilLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-utild.lib"

if (Test-Path $spirvCrossUtilLibRelease) {
    Copy-Item $spirvCrossUtilLibRelease (Join-Path $libDir "spirv-cross-util-release.lib") -Force
    Write-Host "Copied spirv-cross-util library (Release) to $libDir"
}

if (Test-Path $spirvCrossUtilLibDebug) {
    Copy-Item $spirvCrossUtilLibDebug (Join-Path $libDir "spirv-cross-util-debug.lib") -Force
    Write-Host "Copied spirv-cross-util library (Debug) to $libDir"
}

# Copy SPIRV-Cross C library (optional, for C API)
$spirvCrossCLibRelease = Join-Path $buildDirRelease "Release\spirv-cross-c.lib"
$spirvCrossCLibDebug = Join-Path $buildDirDebug "Debug\spirv-cross-cd.lib"

if (Test-Path $spirvCrossCLibRelease) {
    Copy-Item $spirvCrossCLibRelease (Join-Path $libDir "spirv-cross-c-release.lib") -Force
    Write-Host "Copied spirv-cross-c library (Release) to $libDir"
}

if (Test-Path $spirvCrossCLibDebug) {
    Copy-Item $spirvCrossCLibDebug (Join-Path $libDir "spirv-cross-c-debug.lib") -Force
    Write-Host "Copied spirv-cross-c library (Debug) to $libDir"
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

# Copy headers (SPIRV-Cross headers are in the root directory)
$headerFiles = @(
    "spirv_cross.hpp",
    "spirv_cross_containers.hpp",
    "spirv_cross_error_handling.hpp",
    "spirv_glsl.hpp",
    "spirv_hlsl.hpp",
    "spirv_msl.hpp",
    "spirv_cpp.hpp",
    "spirv_reflect.hpp",
    "spirv_cross_c.h",
    "spirv_common.hpp",
    "spirv.hpp",
    "spirv.h",
    "GLSL.std.450.h",
    "spirv_cross_parsed_ir.hpp",
    "spirv_parser.hpp",
    "spirv_cross_util.hpp",
    "spirv_cfg.hpp"
)

foreach ($header in $headerFiles) {
    $headerPath = Join-Path $spirvCrossDir $header
    if (Test-Path $headerPath) {
        Copy-Item $headerPath $includeDir -Force
    }
}

# Also copy SPIRV directory if it exists (contains spirv.h for C bindings)
$spirvSubDir = Join-Path $spirvCrossDir "include\spirv\unified1"
if (Test-Path $spirvSubDir) {
    $spirvDestDir = Join-Path $includeDir "spirv\unified1"
    New-Item -ItemType Directory -Force -Path $spirvDestDir | Out-Null
    Copy-Item -Path "$spirvSubDir\*" -Destination $spirvDestDir -Recurse -Force
    Write-Host "Copied SPIRV unified headers to $spirvDestDir"
}

Write-Host "SPIRV-Cross headers copied to $includeDir"
Write-Host "SPIRV-Cross build complete!" -ForegroundColor Green