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
$libopusencDir = Join-Path $root "Engine\Vendor\libopusenc"
$libopusDir = Join-Path $root "Engine\Vendor\libopus"

# Check if libopus is built
$libopusLibDir = Join-Path $libopusDir "lib"
if (-not (Test-Path (Join-Path $libopusLibDir "opus-debug.lib")) -and -not (Test-Path (Join-Path $libopusLibDir "opus-release.lib"))) {
    throw "libopus must be built before libopusenc. Please run build_libopus_windows.ps1 first."
}

# Check if libopusenc directory exists and has build files
$hasCmake = Test-Path (Join-Path $libopusencDir "CMakeLists.txt")
$hasAutotools = Test-Path (Join-Path $libopusencDir "configure.ac")

if (-not $hasCmake -and -not $hasAutotools) {
    Write-Host "libopusenc not found. Cloning libopusenc..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone libopusenc if it doesn't exist
    if (-not (Test-Path $libopusencDir)) {
        Push-Location $vendorDir
        try {
            git clone https://github.com/xiph/libopusenc.git libopusenc
            Write-Host "Cloned libopusenc repository" -ForegroundColor Green
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists but build files are missing, try to update
        if (-not $hasCmake -and -not $hasAutotools) {
            Write-Host "Build files not found, updating repository..." -ForegroundColor Yellow
            Push-Location $libopusencDir
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
    
    # Re-check for build files
    $hasCmake = Test-Path (Join-Path $libopusencDir "CMakeLists.txt")
    $hasAutotools = Test-Path (Join-Path $libopusencDir "configure.ac")
}

# Ensure version header exists (some platforms expect win32/version.h)
$versionHeader = Join-Path $libopusencDir "win32\version.h"
if (-not (Test-Path $versionHeader)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $versionHeader) | Out-Null
    @"
#ifndef VERSION_H
#define VERSION_H

#define PACKAGE_VERSION "0.2.1"
#define PACKAGE_VERSION_MAJOR 0
#define PACKAGE_VERSION_MINOR 2
#define PACKAGE_VERSION_PATCH 1
#define PACKAGE_NAME "libopusenc"

#endif /* VERSION_H */
"@ | Set-Content -Path $versionHeader -Encoding ASCII
}

# If upstream has no CMake, generate a minimal one in a temp source dir so we match other libraries' CMake flow.
if (-not $hasCmake) {
    $tempSource = Join-Path $libopusencDir "_cmake"
    if (-not (Test-Path $tempSource)) {
        New-Item -ItemType Directory -Force -Path $tempSource | Out-Null
    }
    $cmakeFile = Join-Path $tempSource "CMakeLists.txt"
    Set-Content -Path $cmakeFile -Encoding ASCII -Value @'
cmake_minimum_required(VERSION 3.16)
project(libopusenc LANGUAGES C)
option(OPUSENC_BUILD_SHARED "Build libopusenc as a shared library" OFF)
set(OPUS_ROOT "" CACHE PATH "Path to the libopus install/prefix")
set(OPUS_INCLUDE_DIR "" CACHE PATH "Path to the libopus include directory (contains opus/opus.h)")
set(OPUS_LIBRARY "" CACHE FILEPATH "Path to the libopus library")
if(NOT OPUS_INCLUDE_DIR AND OPUS_ROOT)
  set(OPUS_INCLUDE_DIR "${OPUS_ROOT}/include")
endif()
if(NOT OPUS_LIBRARY AND OPUS_ROOT)
  find_library(OPUS_LIBRARY NAMES opus opus_static PATHS "${OPUS_ROOT}/lib" "${OPUS_ROOT}/lib64" NO_DEFAULT_PATH)
endif()
if(NOT OPUS_INCLUDE_DIR)
  find_path(OPUS_INCLUDE_DIR opus/opus.h)
endif()
if(NOT OPUS_LIBRARY)
  find_library(OPUS_LIBRARY NAMES opus opus_static)
endif()
if(NOT OPUS_INCLUDE_DIR OR NOT OPUS_LIBRARY)
  message(FATAL_ERROR "libopus not found. Set OPUS_ROOT or OPUS_INCLUDE_DIR and OPUS_LIBRARY.")
endif()
set(OPUSENC_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/../src/ogg_packer.c
  ${CMAKE_CURRENT_LIST_DIR}/../src/opus_header.c
  ${CMAKE_CURRENT_LIST_DIR}/../src/opusenc.c
  ${CMAKE_CURRENT_LIST_DIR}/../src/picture.c
  ${CMAKE_CURRENT_LIST_DIR}/../src/resample.c
  ${CMAKE_CURRENT_LIST_DIR}/../src/unicode_support.c
)
add_library(opusenc ${OPUSENC_SOURCES})
add_library(OPE::opusenc ALIAS opusenc)
if(NOT OPUSENC_BUILD_SHARED)
  set_target_properties(opusenc PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(opusenc
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${OPUS_INCLUDE_DIR}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${CMAKE_CURRENT_LIST_DIR}/../win32
)
target_compile_definitions(opusenc PRIVATE
  HAVE_CONFIG_H
  OUTSIDE_SPEEX
  RANDOM_PREFIX=ope
  RESAMPLE_FULL_SINC_TABLE=1
  PACKAGE_NAME="libopusenc"
  PACKAGE_VERSION="0.2.1"
)
if(MSVC)
  target_compile_definitions(opusenc PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
target_link_libraries(opusenc PUBLIC ${OPUS_LIBRARY})
set_property(TARGET opusenc PROPERTY C_STANDARD 99)
set_property(TARGET opusenc PROPERTY C_STANDARD_REQUIRED ON)
'@
    $cmakeSourceDir = $tempSource
} else {
    $cmakeSourceDir = $libopusencDir
}

if (-not (Test-Path (Join-Path $cmakeSourceDir "CMakeLists.txt"))) {
    throw "libopusenc CMakeLists.txt not found after preparation. Checked $cmakeSourceDir"
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $libopusencDir "build-debug"
$buildDirRelease = Join-Path $libopusencDir "build-release"

if ($Force -or -not (Test-Path $buildDirDebug)) {
    New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
}

if ($Force -or -not (Test-Path $buildDirRelease)) {
    New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null
}

# Find library files for CMake configuration
$opusDebugLib = Join-Path $libopusLibDir "opus-debug.lib"
$opusReleaseLib = Join-Path $libopusLibDir "opus-release.lib"
$libopusIncludeDir = Join-Path $libopusDir "include"

Write-Host "Building libopusenc with CMake..." -ForegroundColor Green

# Configure and build Debug configuration
Write-Host "Configuring libopusenc (Debug)..." -ForegroundColor Yellow
Push-Location $buildDirDebug
try {
    $cmakeConfigureDebug = @(
        "-S", $cmakeSourceDir
        "-B", $buildDirDebug
        "-DCMAKE_BUILD_TYPE=Debug"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug"
        "-DCMAKE_PREFIX_PATH=$libopusDir"
        "-DOPUS_ROOT=$libopusDir"
        "-DOPUS_INCLUDE_DIR=$libopusIncludeDir"
        "-DOPUS_LIBRARY=$opusDebugLib"
    )

    cmake @cmakeConfigureDebug
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopusenc Debug CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building libopusenc (Debug)..." -ForegroundColor Yellow
    cmake --build $buildDirDebug --config Debug --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopusenc Debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

# Configure and build Release configuration
Write-Host "Configuring libopusenc (Release)..." -ForegroundColor Yellow
Push-Location $buildDirRelease
try {
    $cmakeConfigureRelease = @(
        "-S", $cmakeSourceDir
        "-B", $buildDirRelease
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"
        "-DCMAKE_PREFIX_PATH=$libopusDir"
        "-DOPUS_ROOT=$libopusDir"
        "-DOPUS_INCLUDE_DIR=$libopusIncludeDir"
        "-DOPUS_LIBRARY=$opusReleaseLib"
    )

    cmake @cmakeConfigureRelease
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopusenc Release CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building libopusenc (Release)..." -ForegroundColor Yellow
    cmake --build $buildDirRelease --config Release --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "libopusenc Release build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "libopusenc built successfully!" -ForegroundColor Green

# Copy the built libraries and headers
$libDir = Join-Path $root "Engine\Vendor\libopusenc\lib"
$includeDir = Join-Path $root "Engine\Vendor\libopusenc\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Find and copy Debug libraries
$opusencDebugLib = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opusenc*.lib" | Where-Object { $_.Name -like "*Debug*" -or $_.Name -like "*debug*" } | Select-Object -First 1
$opusencDebugLibMain = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opusenc.lib" | Select-Object -First 1

if ($opusencDebugLibMain -and (Test-Path $opusencDebugLibMain.FullName)) {
    Copy-Item $opusencDebugLibMain.FullName (Join-Path $libDir "opusenc-debug.lib") -Force
    Write-Host "Copied libopusenc (Debug) to $libDir\opusenc-debug.lib" -ForegroundColor Green
} elseif ($opusencDebugLib -and (Test-Path $opusencDebugLib.FullName)) {
    Copy-Item $opusencDebugLib.FullName (Join-Path $libDir "opusenc-debug.lib") -Force
    Write-Host "Copied libopusenc (Debug) to $libDir\opusenc-debug.lib" -ForegroundColor Green
} else {
    throw "libopusenc Debug library not found in build directory: $buildDirDebug"
}

# Find and copy Release libraries
$opusencReleaseLib = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opusenc*.lib" | Where-Object { $_.Name -like "*Release*" -or $_.Name -like "*release*" } | Select-Object -First 1
$opusencReleaseLibMain = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opusenc.lib" | Select-Object -First 1

if ($opusencReleaseLibMain -and (Test-Path $opusencReleaseLibMain.FullName)) {
    Copy-Item $opusencReleaseLibMain.FullName (Join-Path $libDir "opusenc-release.lib") -Force
    Write-Host "Copied libopusenc (Release) to $libDir\opusenc-release.lib" -ForegroundColor Green
} elseif ($opusencReleaseLib -and (Test-Path $opusencReleaseLib.FullName)) {
    Copy-Item $opusencReleaseLib.FullName (Join-Path $libDir "opusenc-release.lib") -Force
    Write-Host "Copied libopusenc (Release) to $libDir\opusenc-release.lib" -ForegroundColor Green
} else {
    throw "libopusenc Release library not found in build directory: $buildDirRelease"
}

# Copy headers
$headerSourceDir = Join-Path $libopusencDir "include"
$opusencHeaderDestDir = Join-Path $includeDir "opus"

# Create opus subdirectory in include
New-Item -ItemType Directory -Force -Path $opusencHeaderDestDir | Out-Null

# Copy all header files from include to include/opus
if (Test-Path $headerSourceDir) {
    $headerFiles = Get-ChildItem -Path $headerSourceDir -Filter "*.h" -File -Recurse
    foreach ($headerFile in $headerFiles) {
        $relativePath = $headerFile.FullName.Substring($headerSourceDir.Length + 1)
        if ($relativePath -eq $headerFile.Name) {
            # File is directly in include, copy to opus subdirectory
            $destFile = Join-Path $opusencHeaderDestDir $headerFile.Name
        } else {
            $destFile = Join-Path $opusencHeaderDestDir $relativePath
        }
        $destDir = Split-Path $destFile -Parent
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        Copy-Item -Path $headerFile.FullName -Destination $destFile -Force
    }
    Write-Host "Copied libopusenc headers to $opusencHeaderDestDir" -ForegroundColor Green
} else {
    Write-Host "Warning: libopusenc header directory not found at $headerSourceDir" -ForegroundColor Yellow
    Write-Host "You may need to manually copy the headers or rebuild libopusenc." -ForegroundColor Yellow
}

Write-Host "libopusenc build complete!" -ForegroundColor Green
