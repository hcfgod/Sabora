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
$opusfileDir = Join-Path $root "Engine\Vendor\opusfile"
$libopusDir = Join-Path $root "Engine\Vendor\libopus"
$liboggDir = Join-Path $root "Engine\Vendor\libogg"

# Check if dependencies are built
$libopusLibDir = Join-Path $libopusDir "lib"
$liboggLibDir = Join-Path $liboggDir "lib"

if (-not ((Test-Path (Join-Path $libopusLibDir "opus-debug.lib")) -or (Test-Path (Join-Path $libopusLibDir "opus-release.lib")))) {
    throw "libopus must be built before opusfile. Please run build_libopus_windows.ps1 first."
}

if (-not ((Test-Path (Join-Path $liboggLibDir "ogg-debug.lib")) -or (Test-Path (Join-Path $liboggLibDir "ogg-release.lib")))) {
    throw "libogg must be built before opusfile. Please run build_libogg_windows.ps1 first."
}

# Check if opusfile directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $opusfileDir "CMakeLists.txt"))) {
    Write-Host "opusfile not found. Cloning opusfile..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone opusfile if it doesn't exist
    if (-not (Test-Path $opusfileDir)) {
        Push-Location $vendorDir
        try {
            git clone https://github.com/xiph/opusfile.git opusfile
            Write-Host "Cloned opusfile repository" -ForegroundColor Green
        }
        finally {
            Pop-Location
        }
    } else {
        # If directory exists but headers are missing, try to update
        $opusfileHeaderDir = Join-Path $opusfileDir "include\opus"
        if (-not (Test-Path $opusfileHeaderDir)) {
            Write-Host "Headers not found, updating repository..." -ForegroundColor Yellow
            Push-Location $opusfileDir
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
    if (-not (Test-Path (Join-Path $opusfileDir "CMakeLists.txt"))) {
        throw "opusfile CMakeLists.txt not found after cloning. opusfile directory: $opusfileDir"
    }
}

# Patch opusfile's FindOgg.cmake to work without PkgConfig on Windows
$findOggPath = Join-Path $opusfileDir "cmake\FindOgg.cmake"
if (Test-Path $findOggPath) {
    $findOggContent = Get-Content $findOggPath -Raw
    if ($findOggContent -notmatch "OGG_INCLUDE_DIR AND OGG_LIBRARY") {
        # Patch FindOgg.cmake to make PkgConfig optional and use manual variables
        $patchedContent = @"
# Skip CONFIG mode since OggConfig.cmake may not be available
# find_package(Ogg CONFIG QUIET)
if(NOT TARGET Ogg::ogg)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(Ogg REQUIRED IMPORTED_TARGET ogg)
    set_target_properties(PkgConfig::Ogg PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(Ogg::ogg ALIAS PkgConfig::Ogg)
  else()
    # Fallback: use manual variables if PkgConfig is not available
    if(OGG_INCLUDE_DIR AND OGG_LIBRARY)
      if(NOT TARGET Ogg::ogg)
        add_library(Ogg::ogg UNKNOWN IMPORTED)
        set_target_properties(Ogg::ogg PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "`${OGG_INCLUDE_DIR}"
          IMPORTED_LOCATION "`${OGG_LIBRARY}"
        )
      endif()
      set(Ogg_FOUND TRUE)
    else()
      message(FATAL_ERROR "Ogg library not found. Please set OGG_INCLUDE_DIR and OGG_LIBRARY, or install PkgConfig.")
    endif()
  endif()
endif()
"@
        Set-Content -Path $findOggPath -Value $patchedContent -NoNewline
        Write-Host "Patched opusfile FindOgg.cmake to work without PkgConfig" -ForegroundColor Green
    }
}

# Patch opusfile's FindOpus.cmake to work without PkgConfig and incomplete OpusConfig.cmake
$findOpusPath = Join-Path $opusfileDir "cmake\FindOpus.cmake"
if (Test-Path $findOpusPath) {
    $findOpusContent = Get-Content $findOpusPath -Raw
    if ($findOpusContent -notmatch "OPUS_INCLUDE_DIR AND OPUS_LIBRARY") {
        # Patch FindOpus.cmake to make PkgConfig optional and use manual variables
        $patchedOpusContent = @"
# Skip CONFIG mode since OpusConfig.cmake from libopus is incomplete
# find_package(Opus CONFIG QUIET)
if(NOT TARGET Opus::opus)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(Opus REQUIRED IMPORTED_TARGET opus)
    set_target_properties(PkgConfig::Opus PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(Opus::opus ALIAS PkgConfig::Opus)
  else()
    # Fallback: use manual variables if PkgConfig is not available
    if(OPUS_INCLUDE_DIR AND OPUS_LIBRARY)
      if(NOT TARGET Opus::opus)
        add_library(Opus::opus UNKNOWN IMPORTED)
        set_target_properties(Opus::opus PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "`${OPUS_INCLUDE_DIR}"
          IMPORTED_LOCATION "`${OPUS_LIBRARY}"
        )
      endif()
      set(Opus_FOUND TRUE)
    else()
      message(FATAL_ERROR "Opus library not found. Please set OPUS_INCLUDE_DIR and OPUS_LIBRARY, or install PkgConfig.")
    endif()
  endif()
endif()
"@
        Set-Content -Path $findOpusPath -Value $patchedOpusContent -NoNewline
        Write-Host "Patched opusfile FindOpus.cmake to work without PkgConfig" -ForegroundColor Green
    }
}

# Create build directories for Debug and Release separately
$buildDirDebug = Join-Path $opusfileDir "build-debug"
$buildDirRelease = Join-Path $opusfileDir "build-release"

if ($Force -or -not (Test-Path $buildDirDebug)) {
    New-Item -ItemType Directory -Force -Path $buildDirDebug | Out-Null
}

if ($Force -or -not (Test-Path $buildDirRelease)) {
    New-Item -ItemType Directory -Force -Path $buildDirRelease | Out-Null
}

# Find library files for CMake configuration
$opusDebugLib = Join-Path $libopusLibDir "opus-debug.lib"
$opusReleaseLib = Join-Path $libopusLibDir "opus-release.lib"
$oggDebugLib = Join-Path $liboggLibDir "ogg-debug.lib"
$oggReleaseLib = Join-Path $liboggLibDir "ogg-release.lib"

$libopusIncludeDir = Join-Path $libopusDir "include"
$liboggIncludeDir = Join-Path $liboggDir "include"

Write-Host "Building opusfile with CMake..." -ForegroundColor Green

# Configure and build Debug configuration
Write-Host "Configuring opusfile (Debug)..." -ForegroundColor Yellow
Push-Location $buildDirDebug
try {
    $cmakeArgs = @(
        "-S", $opusfileDir,
        "-B", $buildDirDebug,
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DOP_DISABLE_DOCS=ON",
        "-DOP_DISABLE_EXAMPLES=ON",
        "-DOP_DISABLE_TESTS=ON",
        "-DOP_DISABLE_HTTP=ON",
        "-DCMAKE_PREFIX_PATH=$liboggDir;$libopusDir",
        "-DOGG_ROOT=$liboggDir",
        "-DOGG_INCLUDE_DIR=$liboggIncludeDir",
        "-DOGG_LIBRARY=$oggDebugLib",
        "-DOPUS_ROOT=$libopusDir",
        "-DOPUS_INCLUDE_DIR=$libopusIncludeDir",
        "-DOPUS_LIBRARY=$opusDebugLib"
    )
    
    & cmake $cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "opusfile Debug CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building opusfile (Debug)..." -ForegroundColor Yellow
    cmake --build $buildDirDebug --config Debug --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "opusfile Debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

# Configure and build Release configuration
Write-Host "Configuring opusfile (Release)..." -ForegroundColor Yellow
Push-Location $buildDirRelease
try {
    $cmakeArgs = @(
        "-S", $opusfileDir,
        "-B", $buildDirRelease,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DOP_DISABLE_DOCS=ON",
        "-DOP_DISABLE_EXAMPLES=ON",
        "-DOP_DISABLE_TESTS=ON",
        "-DOP_DISABLE_HTTP=ON",
        "-DCMAKE_PREFIX_PATH=$liboggDir;$libopusDir",
        "-DOGG_ROOT=$liboggDir",
        "-DOGG_INCLUDE_DIR=$liboggIncludeDir",
        "-DOGG_LIBRARY=$oggReleaseLib",
        "-DOPUS_ROOT=$libopusDir",
        "-DOPUS_INCLUDE_DIR=$libopusIncludeDir",
        "-DOPUS_LIBRARY=$opusReleaseLib"
    )
    
    & cmake $cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "opusfile Release CMake configuration failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Building opusfile (Release)..." -ForegroundColor Yellow
    cmake --build $buildDirRelease --config Release --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "opusfile Release build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "opusfile built successfully!" -ForegroundColor Green

# Copy the built libraries and headers
$libDir = Join-Path $root "Engine\Vendor\opusfile\lib"
$includeDir = Join-Path $root "Engine\Vendor\opusfile\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Find and copy Debug libraries
$opusfileDebugLib = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opusfile*.lib" | Where-Object { $_.Name -like "*Debug*" -or $_.Name -like "*debug*" } | Select-Object -First 1
$opusfileDebugLibMain = Get-ChildItem -Path $buildDirDebug -Recurse -Filter "opusfile.lib" | Select-Object -First 1

if ($opusfileDebugLibMain -and (Test-Path $opusfileDebugLibMain.FullName)) {
    Copy-Item $opusfileDebugLibMain.FullName (Join-Path $libDir "opusfile-debug.lib") -Force
    Write-Host "Copied opusfile (Debug) to $libDir\opusfile-debug.lib" -ForegroundColor Green
} elseif ($opusfileDebugLib -and (Test-Path $opusfileDebugLib.FullName)) {
    Copy-Item $opusfileDebugLib.FullName (Join-Path $libDir "opusfile-debug.lib") -Force
    Write-Host "Copied opusfile (Debug) to $libDir\opusfile-debug.lib" -ForegroundColor Green
} else {
    throw "opusfile Debug library not found in build directory: $buildDirDebug"
}

# Find and copy Release libraries
$opusfileReleaseLib = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opusfile*.lib" | Where-Object { $_.Name -like "*Release*" -or $_.Name -like "*release*" } | Select-Object -First 1
$opusfileReleaseLibMain = Get-ChildItem -Path $buildDirRelease -Recurse -Filter "opusfile.lib" | Select-Object -First 1

if ($opusfileReleaseLibMain -and (Test-Path $opusfileReleaseLibMain.FullName)) {
    Copy-Item $opusfileReleaseLibMain.FullName (Join-Path $libDir "opusfile-release.lib") -Force
    Write-Host "Copied opusfile (Release) to $libDir\opusfile-release.lib" -ForegroundColor Green
} elseif ($opusfileReleaseLib -and (Test-Path $opusfileReleaseLib.FullName)) {
    Copy-Item $opusfileReleaseLib.FullName (Join-Path $libDir "opusfile-release.lib") -Force
    Write-Host "Copied opusfile (Release) to $libDir\opusfile-release.lib" -ForegroundColor Green
} else {
    throw "opusfile Release library not found in build directory: $buildDirRelease"
}

# Copy headers
$headerSourceDir = Join-Path $opusfileDir "include"
$opusfileHeaderDestDir = Join-Path $includeDir "opus"

# Create opus subdirectory in include
New-Item -ItemType Directory -Force -Path $opusfileHeaderDestDir | Out-Null

# Copy all header files from include to include/opus
if (Test-Path $headerSourceDir) {
    $headerFiles = Get-ChildItem -Path $headerSourceDir -Filter "*.h" -File -Recurse
    foreach ($headerFile in $headerFiles) {
        $relativePath = $headerFile.FullName.Substring($headerSourceDir.Length + 1)
        $destFile = Join-Path $opusfileHeaderDestDir $relativePath
        $destDir = Split-Path $destFile -Parent
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item -Path $headerFile.FullName -Destination $destFile -Force
    }
    Write-Host "Copied opusfile headers to $opusfileHeaderDestDir" -ForegroundColor Green
} else {
    Write-Host "Warning: opusfile header directory not found at $headerSourceDir" -ForegroundColor Yellow
    Write-Host "You may need to manually copy the headers or rebuild opusfile." -ForegroundColor Yellow
}

Write-Host "opusfile build complete!" -ForegroundColor Green
