Param(
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = (Resolve-Path "$scriptDir\..\..").Path
$vendor = Join-Path $root "Engine\Vendor"
$srcDir = Join-Path $vendor "Jolt\\Build"
$buildDir = Join-Path $srcDir "out"
$libDir = Join-Path $vendor "Jolt\lib"

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $libDir | Out-Null

$cmakeCommon = @(
    "-DJPH_BUILD_SHARED_LIBRARY=OFF",
    "-DJPH_BUILD_EXAMPLES=OFF",
    "-DJPH_BUILD_TESTS=OFF",
    "-DFLOATING_POINT_EXCEPTIONS_ENABLED=OFF",
    "-DPROFILER_IN_DEBUG_AND_RELEASE=OFF",
    "-DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF",
    "-DENABLE_OBJECT_STREAM=OFF",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>",
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$libDir",
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=$libDir/Debug",
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=$libDir/Release",
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$libDir",
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG=$libDir/Debug",
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE=$libDir/Release",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$libDir",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=$libDir/Debug",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$libDir/Release",
    "-DCMAKE_DEBUG_POSTFIX=_debug"
)

Write-Host "Configuring Jolt..." -ForegroundColor Green
cmake -S $srcDir -B $buildDir -G $Generator -A $Arch @cmakeCommon

Write-Host "Building Jolt (Debug)..." -ForegroundColor Green
cmake --build $buildDir --config Debug --target Jolt

Write-Host "Building Jolt (Release)..." -ForegroundColor Green
cmake --build $buildDir --config Release --target Jolt

Write-Host "Jolt built successfully. Libraries are in $libDir" -ForegroundColor Green
