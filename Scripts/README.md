# Dependency Installation Scripts

This directory contains scripts to install and manage dependencies for the Sabora engine across different platforms.

## What's Included

The scripts will install the following dependencies:

- **spdlog** - Fast C++ logging library
- **doctest** - Lightweight C++ testing framework
- **glm** - OpenGL Mathematics library
- **json** - JSON library for C++
- **SDL3** - Simple DirectMedia Layer 3 (cross-platform multimedia library)

## Prerequisites

- **Git** - Required for downloading dependencies
- **CMake** - Required for building SDL3 (version 3.1 or later recommended)

### Installing CMake

#### Windows
```powershell
# Using Chocolatey
choco install cmake

# Using winget
winget install Kitware.CMake

# Or download from https://cmake.org/download/
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# Fedora/RHEL
sudo dnf install cmake

# Arch Linux
sudo pacman -S cmake
```

#### macOS
```bash
# Using Homebrew
brew install cmake

# Using MacPorts
sudo port install cmake
```

## Platform-Specific Scripts

### Windows
```powershell
# Run from PowerShell
.\Scripts\install_deps_windows.ps1

# Force reinstall all dependencies
.\Scripts\install_deps_windows.ps1 -Force
```

### Linux
```bash
# Run from bash
./Scripts/install_deps_linux.sh

# Force reinstall all dependencies
FORCE=1 ./Scripts/install_deps_linux.sh
```

### macOS
```bash
# Run from bash
./Scripts/install_deps_macos.sh

# Force reinstall all dependencies
FORCE=1 ./Scripts/install_deps_macos.sh
```

## SDL3 Configuration

SDL3 is built using CMake to generate the necessary build configuration files, then integrated with premake5 for cross-platform project generation.

### Build Process

1. **Clone SDL3** - Downloads the latest SDL3 source code
2. **Build with CMake** - Generates `SDL_build_config.h` and other required files
3. **Copy premake5.lua** - Applies our custom build configuration
4. **Generate project files** - Uses premake5 to create platform-specific projects

### Important: Build Order

**SDL3 must be built with CMake BEFORE running premake5.** The workflow is:

```bash
# 1. Install dependencies (includes building SDL3 with CMake)
./Scripts/install_deps_windows.ps1    # Windows
./Scripts/install_deps_linux.sh       # Linux
./Scripts/install_deps_macos.sh       # macOS

# 2. Verify SDL3 setup (optional but recommended)
./Scripts/test_sdl3_setup.ps1         # Windows
./Scripts/test_sdl3_setup.sh          # Linux/macOS

# 3. Generate project files with premake5
premake5 vs2022                        # Windows
premake5 gmake2                        # Linux/macOS
```

### What CMake Builds

The CMake build process:
- Generates `SDL_build_config.h` (required for compilation)
- Compiles SDL3 as a static library
- Copies the library to `Engine/Vendor/SDL/lib/`
- Copies the build config to `Engine/Vendor/SDL/include/`

### What premake5 Does

The premake5 configuration:
- Links against the pre-built SDL3 library
- Provides platform-specific linking options
- Integrates SDL3 into your build system

### Features

- **Static linking** - All SDL3 code is compiled into your executable
- **Cross-platform support** - Windows, macOS, and Linux
- **Full feature set** - Audio, video, input, haptics, and more
- **Optimized builds** - Debug and Release configurations

### Platform-Specific SDL3 Features

#### Windows
- DirectX support
- Windows API integration
- XInput and DirectInput support

#### macOS
- Metal graphics API
- Core Audio integration
- Cocoa window management
- Game Controller framework

#### Linux
- X11 and Wayland support
- ALSA and PulseAudio audio
- udev device management
- Full desktop integration

## Manual SDL3 Build

If you need to rebuild SDL3 separately, you can use the build scripts directly:

```bash
# Windows
.\Scripts\build_sdl3_windows.ps1

# Linux
./Scripts/build_sdl3_linux.sh

# macOS
./Scripts/build_sdl3_macos.sh
```

## Building

After installing dependencies, you can build the project using:

```bash
# Generate project files
premake5 gmake2    # Linux/macOS
premake5 vs2022    # Windows

# Build
make               # Linux/macOS
# Or open the generated .sln file in Visual Studio on Windows
```

## Troubleshooting

### Common Issues

1. **Git not found**: Ensure git is installed and in your PATH
2. **CMake not found**: Install CMake and add it to your PATH
3. **Permission denied**: Make sure you have write permissions to the Engine/Vendor directory
4. **Build errors**: Try running with `-Force` flag to reinstall dependencies

### SDL3-Specific Issues

- **CMake errors**: Ensure CMake version 3.1 or later is installed
- **Build configuration errors**: The CMake build generates necessary header files
- **Linker errors**: Ensure `SDL_STATIC` is defined in your build
- **Runtime errors**: Check that all required system libraries are available

### Platform-Specific Issues

#### Linux
- **Missing X11 libraries**: Install development packages for your distribution
- **Audio backend issues**: Install ALSA and PulseAudio development packages

#### macOS
- **Framework linking errors**: Ensure Xcode command line tools are installed
- **Metal API issues**: Requires macOS 10.14 or later

#### Windows
- **DirectX SDK issues**: Visual Studio includes necessary DirectX headers
- **Build tool version**: Ensure Visual Studio 2019 or later is installed

## Dependencies Location

All dependencies are installed under `Engine/Vendor/`:
```
Engine/Vendor/
├── spdlog/
├── doctest/
├── glm/
├── json/
└── SDL/
    ├── include/          # SDL3 headers
    ├── lib/              # Built static libraries
    ├── build/            # CMake build directory
    └── premake5.lua      # Our build configuration
```

## Updating Dependencies

To update dependencies to their latest versions:

```bash
# Windows
.\Scripts\install_deps_windows.ps1 -Force

# Linux/macOS
FORCE=1 ./Scripts/install_deps_linux.sh
FORCE=1 ./Scripts/install_deps_macos.sh
```

## Advanced Configuration

### Custom SDL3 Build Options

You can modify the CMake build options in the build scripts:

- **Video backends**: OpenGL, Vulkan, Metal, DirectX
- **Audio backends**: ALSA, PulseAudio, Core Audio, DirectSound
- **Input systems**: X11, Wayland, Windows API, Cocoa
- **Optional features**: HIDAPI, sensors, haptics

### Cross-Compilation

The build scripts support cross-compilation by setting environment variables:

```bash
# Linux cross-compilation example
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
./Scripts/build_sdl3_linux.sh
```
