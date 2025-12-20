# Build Troubleshooting Guide

This document provides solutions to common build issues encountered when building the Sabora Engine.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Common Issues](#common-issues)
- [Platform-Specific Issues](#platform-specific-issues)
- [Dependency Issues](#dependency-issues)
- [Premake Issues](#premake-issues)
- [Compiler Issues](#compiler-issues)

## Prerequisites

Before troubleshooting, ensure you have:

- **C++20 compatible compiler** (GCC 10+, Clang 12+, MSVC 2019+)
- **Premake5** installed and in PATH
- **Git** for cloning dependencies
- **CMake 3.20+** (for building some dependencies on Linux/macOS)
- Platform-specific build tools (Visual Studio, Xcode, etc.)

## Common Issues

### Issue: Premake Generation Fails

**Symptoms:**
- `premake5` command fails with errors
- Project files are not generated

**Solutions:**

1. **Verify Premake Installation:**
   ```bash
   premake5 --version
   ```
   Should show version 5.0 or later.

2. **Check Premake Script Syntax:**
   - Ensure all `.lua` files in the project are valid Lua syntax
   - Check for missing dependencies in `Engine/Vendor/dependencies.lua`

3. **Clean and Regenerate:**
   ```bash
   # Remove generated files
   rm -rf Build/
   
   # Regenerate
   premake5 vs2022  # or gmake2, xcode4, etc.
   ```

### Issue: Missing Dependencies

**Symptoms:**
- Linker errors about missing libraries
- Include path errors
- Build scripts fail

**Solutions:**

1. **Run Setup Script:**
   ```bash
   # Windows
   .\Setup.bat
   
   # Linux
   ./Setup_linux.sh
   
   # macOS
   ./Setup_macos.sh
   ```

2. **Manually Build Dependencies:**
   - Check `Scripts/` directory for platform-specific build scripts
   - Ensure all dependencies are built before building the engine

3. **Verify Dependency Paths:**
   - Check `Engine/Vendor/dependencies.lua` for correct paths
   - Ensure libraries are in expected locations

### Issue: Compiler Version Errors

**Symptoms:**
- C++20 feature errors
- `std::source_location` not found
- Concepts not supported

**Solutions:**

1. **Update Compiler:**
   - **Windows:** Use Visual Studio 2022 or later
   - **Linux:** Install GCC 10+ or Clang 12+
   - **macOS:** Update Xcode to 13+ (includes Clang 12+)

2. **Verify C++ Standard:**
   - Check that `cppdialect "C++20"` is set in premake files
   - Ensure compiler flags include `-std=c++20` (GCC/Clang) or `/std:c++20` (MSVC)

## Platform-Specific Issues

### Windows

#### Issue: Visual Studio Project Not Found

**Solution:**
```powershell
# Generate Visual Studio solution
premake5 vs2022

# Open the generated solution
start Build/Sabora.sln
```

#### Issue: Missing Windows SDK

**Symptoms:**
- `windows.h` not found
- DirectX headers missing

**Solution:**
- Install Windows SDK through Visual Studio Installer
- Ensure "Desktop development with C++" workload is installed

#### Issue: Static Runtime Mismatch

**Symptoms:**
- Linker errors about runtime library mismatch
- `LNK2038` errors

**Solution:**
- Ensure all dependencies are built with the same runtime setting
- Check `dependencies.lua` for `StaticRuntime` settings
- Rebuild dependencies if runtime settings changed

### Linux

#### Issue: Missing Development Packages

**Symptoms:**
- Headers not found (`SDL.h`, `glad/gl.h`, etc.)
- Library linking errors

**Solution:**
```bash
# Install common development packages
sudo apt-get update
sudo apt-get install build-essential cmake libsdl3-dev

# Or on Fedora/RHEL
sudo dnf install gcc-c++ cmake SDL3-devel
```

#### Issue: GLAD Loader Fails

**Symptoms:**
- OpenGL functions not loaded
- `gladLoadGL` returns 0

**Solution:**
- Ensure OpenGL drivers are installed
- Check that `libGL.so` is available
- Verify Mesa or proprietary drivers are properly installed

#### Issue: Premake Not Found

**Solution:**
```bash
# Install Premake
./Scripts/Linux/install_premake_linux.sh

# Or download from https://premake.github.io/download
```

### macOS

#### Issue: Xcode Command Line Tools Missing

**Symptoms:**
- `clang` not found
- Build tools unavailable

**Solution:**
```bash
# Install command line tools
xcode-select --install

# Verify installation
xcode-select -p
```

#### Issue: Homebrew Dependencies

**Symptoms:**
- CMake or other tools not found

**Solution:**
```bash
# Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake premake
```

#### Issue: Code Signing Issues

**Symptoms:**
- Apps won't run
- Code signing errors

**Solution:**
- Set up code signing in Xcode project settings
- For development, disable code signing or use ad-hoc signing

## Dependency Issues

### Issue: SDL3 Not Found

**Symptoms:**
- `SDL3/SDL.h` not found
- SDL functions undefined

**Solutions:**

1. **Build SDL3:**
   ```bash
   # Windows
   .\Scripts\Windows\build_sdl3_windows.ps1
   
   # Linux
   ./Scripts/Linux/build_sdl3_linux.sh
   
   # macOS
   ./Scripts/macOS/build_sdl3_macos.sh
   ```

2. **Verify Include Paths:**
   - Check that SDL3 headers are in `Engine/Vendor/SDL/include`
   - Verify library paths in `dependencies.lua`

### Issue: GLAD Not Generated

**Symptoms:**
- `glad/gl.h` not found
- OpenGL constants undefined

**Solution:**
```bash
# Build GLAD
# Windows
.\Scripts\Windows\build_glad_windows.ps1

# Linux
./Scripts/Linux/build_glad_linux.sh

# macOS
./Scripts/macOS/build_glad_macos.sh
```

### Issue: Shaderc Compilation Fails

**Symptoms:**
- Shader compilation errors
- Shaderc library not found

**Solutions:**

1. **Build Shaderc:**
   ```bash
   # Use platform-specific build script
   ./Scripts/[Platform]/build_shaderc_[platform].sh
   ```

2. **Check Dependencies:**
   - Shaderc requires Python 3.x
   - Ensure Git submodules are initialized

## Premake Issues

### Issue: Premake Script Errors

**Symptoms:**
- Lua syntax errors
- Undefined variables

**Solutions:**

1. **Check Lua Syntax:**
   ```bash
   # Validate Lua files
   lua -l premake5.lua
   ```

2. **Verify Dependencies File:**
   - Check `Engine/Vendor/dependencies.lua` exists
   - Ensure all required fields are present

3. **Check Premake Version:**
   - Use Premake 5.0 or later
   - Older versions may not support all features

### Issue: Generated Project Files Are Incorrect

**Symptoms:**
- Wrong include paths
- Missing source files
   - Incorrect library links

**Solutions:**

1. **Clean Generation:**
   ```bash
   # Remove old generated files
   rm -rf Build/
   
   # Regenerate
   premake5 [action]
   ```

2. **Check Premake Filters:**
   - Verify file filters in `.lua` files
   - Ensure platform-specific code is properly filtered

## Compiler Issues

### Issue: C++20 Features Not Available

**Symptoms:**
- `std::source_location` errors
- Concepts not recognized
- `std::format` not found

**Solutions:**

1. **Update Compiler:**
   - Use latest compiler version
   - Enable C++20 standard explicitly

2. **Check Compiler Flags:**
   ```bash
   # GCC/Clang
   -std=c++20
   
   # MSVC
   /std:c++20
   ```

### Issue: Template Instantiation Errors

**Symptoms:**
- Complex template errors
- Linker errors with templates

**Solutions:**

1. **Check Include Order:**
   - Ensure `pch.h` is included first
   - Verify all required headers are included

2. **Verify Template Definitions:**
   - Check that template implementations are visible
   - Use explicit instantiation if needed

### Issue: Linker Errors

**Symptoms:**
- Undefined references
- Missing symbols

**Solutions:**

1. **Check Library Order:**
   - Libraries must be linked in dependency order
   - Static libraries before shared libraries

2. **Verify Library Paths:**
   - Check `libdirs` in premake files
   - Ensure libraries are built and in correct locations

3. **Check Symbol Visibility:**
   - Verify exported symbols on Windows (DLL exports)
   - Check for name mangling issues

## Getting Help

If you encounter issues not covered here:

1. **Check Logs:**
   - Review build output for specific error messages
   - Check `logs/` directory for engine logs

2. **Verify Environment:**
   - Ensure all prerequisites are installed
   - Check environment variables (PATH, etc.)

3. **Clean Build:**
   ```bash
   # Remove all build artifacts
   rm -rf Build/ obj/ bin/
   
   # Rebuild from scratch
   ./Setup_[platform].sh
   ```

4. **Report Issues:**
   - Include compiler version and platform
   - Provide full error messages
   - Share relevant build configuration

## Additional Resources

- [Premake Documentation](https://github.com/premake/premake-core/wiki)
- [CMake Documentation](https://cmake.org/documentation/)
- Platform-specific build tool documentation