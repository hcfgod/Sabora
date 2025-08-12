# Sabora Engine Scripts

This directory contains build and setup scripts for the Sabora Engine project.

## Overview

The scripts automatically detect and install missing dependencies (Git, CMake) instead of just reporting that they're missing. This makes the setup process much more user-friendly.

## Automatic Dependency Installation

### Windows (PowerShell)
- **Git**: Automatically installed via `winget` if missing
- **CMake**: Automatically installed via `winget` if missing
- **Requirements**: Windows 10/11 with `winget` available

### Linux (Bash)
- **Git**: Automatically installed via the system package manager
- **CMake**: Automatically installed via the system package manager
- **Supported Package Managers**: apt, dnf, yum, pacman, zypper
- **Requirements**: sudo access for package installation

### macOS (Bash)
- **Git**: Automatically installed via Homebrew if missing
- **CMake**: Automatically installed via Homebrew if missing
- **Homebrew**: Automatically installed if missing
- **Requirements**: Internet connection for Homebrew installation

## Scripts

### Dependency Installation
- `install_deps_windows.ps1` - Install all vendor dependencies on Windows
- `install_deps_linux.sh` - Install all vendor dependencies on Linux
- `install_deps_macos.sh` - Install all vendor dependencies on macOS

### SDL3 Building
- `build_sdl3_windows.ps1` - Build SDL3 on Windows
- `build_sdl3_linux.sh` - Build SDL3 on Linux
- `build_sdl3_macos.sh` - Build SDL3 on macOS

### Project Building
- `build_windows.ps1` - Build the entire project on Windows
- `build_linux.sh` - Build the entire project on Linux
- `build_macos.sh` - Build the entire project on macOS

### Premake Installation
- `install_premake_windows.ps1` - Install Premake on Windows
- `install_premake_linux.sh` - Install Premake on Linux
- `install_premake_macos.sh` - Install Premake on macOS

## Usage

### First Time Setup
1. Run the appropriate `install_deps_*.sh/ps1` script for your platform
2. The script will automatically install Git and CMake if they're missing
3. Dependencies will be downloaded and SDL3 will be built

### Building the Project
1. Run the appropriate `build_*.sh/ps1` script for your platform
2. The script will automatically install Premake if it's missing
3. The project will be built using the generated build files

## Notes

- **Windows**: After installing Git or CMake, you may need to restart your terminal for PATH changes to take effect
- **Linux**: Requires sudo access for package installation
- **macOS**: Homebrew installation requires user interaction (accepting license agreements)
- All scripts use `set -euo pipefail` (bash) or `$ErrorActionPreference = 'Stop'` (PowerShell) for robust error handling

## Troubleshooting

If automatic installation fails:
- **Windows**: Ensure `winget` is available (Windows 10 1709+ with App Installer)
- **Linux**: Ensure you have sudo access and a supported package manager
- **macOS**: Ensure you have an internet connection for Homebrew installation

Manual installation links:
- Git: https://git-scm.com/
- CMake: https://cmake.org/
- Homebrew: https://brew.sh/
