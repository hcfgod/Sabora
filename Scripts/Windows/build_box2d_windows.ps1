Param(
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = (Resolve-Path "$scriptDir\..\..").Path
$vendor = Join-Path $root "Engine\Vendor"
$srcDir = Join-Path $vendor "Box2D"
$buildDir = Join-Path $srcDir "build"
$libDir = Join-Path $vendor "Box2D\lib"

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $libDir | Out-Null

$cmakeCommon = @(
    "-DBUILD_SHARED_LIBS=OFF",
    "-DBOX2D_BUILD_UNIT_TESTS=OFF",
    "-DBOX2D_BUILD_TESTBED=OFF",
    "-DBOX2D_BUILD_DOCS=OFF",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$libDir",
    "-DCMAKE_ARCHIVE_OUTPUT_NAME_DEBUG=box2d-debug",
    "-DCMAKE_ARCHIVE_OUTPUT_NAME_RELEASE=box2d-release"
)

Write-Host "Configuring Box2D..." -ForegroundColor Green
cmake -S $srcDir -B $buildDir -G $Generator -A $Arch @cmakeCommon

Write-Host "Building Box2D (Debug)..." -ForegroundColor Green
cmake --build $buildDir --config Debug

Write-Host "Building Box2D (Release)..." -ForegroundColor Green
cmake --build $buildDir --config Release

Write-Host "Box2D built successfully. Libraries are in $libDir" -ForegroundColor Green