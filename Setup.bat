@echo off
echo Setting up Sabora project...

REM Check if PowerShell is available
powershell -Command "Get-Host" >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: PowerShell is required to run this setup.
    echo Please install PowerShell and try again.
    pause
    exit /b 1
)

REM Run the PowerShell script to download Premake
echo Downloading Premake...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Windows\install_premake_windows.ps1"

if %errorlevel% neq 0 (
    echo Error: Failed to download Premake.
    pause
    exit /b 1
)

REM Run the PowerShell script to download Premake
echo Downloading dependencies...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Windows\install_deps_windows.ps1"

echo Generating and building...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Windows\build_windows.ps1"

if %errorlevel% neq 0 (
    echo Error: Failed to download Premake.
    pause
    exit /b 1
)

pause