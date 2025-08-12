#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SDL_DIR="$ROOT_DIR/Engine/Vendor/SDL"
BUILD_DIR="$SDL_DIR/build"

# Function to check if a command is available
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect package manager
detect_package_manager() {
    if command_exists apt-get; then
        echo "apt"
    elif command_exists dnf; then
        echo "dnf"
    elif command_exists yum; then
        echo "yum"
    elif command_exists pacman; then
        echo "pacman"
    elif command_exists zypper; then
        echo "zypper"
    else
        echo "unknown"
    fi
}

# Function to install CMake
install_cmake() {
    echo "CMake not found. Installing CMake..." >&2
    
    local pkg_manager=$(detect_package_manager)
    case $pkg_manager in
        "apt")
            sudo apt-get update
            sudo apt-get install -y cmake build-essential
            ;;
        "dnf"|"yum")
            sudo $pkg_manager install -y cmake gcc gcc-c++ make
            ;;
        "pacman")
            sudo pacman -S --noconfirm cmake base-devel
            ;;
        "zypper")
            sudo zypper install -y cmake gcc-c++ make
            ;;
        *)
            echo "Unsupported package manager. Please install CMake manually." >&2
            exit 1
            ;;
    esac
    
    if command_exists cmake; then
        echo "CMake installed successfully!" >&2
    else
        echo "Failed to install CMake. Please install manually." >&2
        exit 1
    fi
}

# Check and install CMake if needed
if ! command_exists cmake; then
    install_cmake
fi

echo "CMake is available!" >&2

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Building SDL3 with CMake..."

# Configure SDL3 with CMake
cmake_args=(
    -S "$SDL_DIR"
    -B "$BUILD_DIR"
    -DSDL_STATIC=ON
    -DSDL_SHARED=OFF
    -DSDL_TEST=OFF
    -DSDL_EXAMPLES=OFF
    -DSDL_INSTALL_TESTS=OFF
    -DSDL_VIDEO_OPENGL=ON
    -DSDL_VIDEO_VULKAN=ON
    -DSDL_VIDEO_METAL=ON
    -DSDL_AUDIO=ON
    -DSDL_JOYSTICK=ON
    -DSDL_HAPTIC=ON
    -DSDL_POWER=ON
    -DSDL_FILE=ON
    -DSDL_LOADSO=ON
    -DSDL_THREADS=ON
    -DSDL_TIMERS=ON
    -DSDL_ATOMIC=ON
    -DSDL_CPUINFO=ON
    -DSDL_EVENTS=ON
    -DSDL_VIDEO=ON
    -DSDL_RENDER=ON
    -DSDL_SENSOR=ON
    -DSDL_LOCALE=ON
    -DSDL_MISC=ON
    -DSDL_HIDAPI=ON
)

echo "Configuring SDL3..."
cmake "${cmake_args[@]}"

# Build SDL3
echo "Building SDL3..."
cmake --build "$BUILD_DIR" --config Release

echo "SDL3 built successfully!"

# Copy the built library and headers to a location where premake5 can find them
LIB_DIR="$ROOT_DIR/Engine/Vendor/SDL/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/SDL/include"

mkdir -p "$LIB_DIR"

# Copy the static library - CMake outputs it as libSDL3-static.a
LIB_FILE="$BUILD_DIR/libSDL3-static.a"
LIB_FILE_DEBUG="$BUILD_DIR/Debug/libSDL3-static.a"

if [[ -f "$LIB_FILE" ]]; then
    cp "$LIB_FILE" "$LIB_DIR/"
    cp "$LIB_FILE" "$LIB_DIR/libSDL3.a"
    echo "Copied libSDL3-static.a to $LIB_DIR (renamed to libSDL3.a)"
elif [[ -f "$LIB_FILE_DEBUG" ]]; then
    cp "$LIB_FILE_DEBUG" "$LIB_DIR/"
    cp "$LIB_FILE_DEBUG" "$LIB_DIR/libSDL3.a"
    echo "Copied libSDL3-static.a (Debug) to $LIB_DIR (renamed to libSDL3.a)"
else
    echo "Warning: SDL3 library not found. Checking build directory structure..."
    find "$BUILD_DIR" -name "*.a" -exec basename {} \;
fi

# Also copy to the Premake5 output directory for linking
# Copy to both Debug and Release directories to handle both build configurations
PREMAKE_LIB_DIR_RELEASE="$ROOT_DIR/Build/bin/Release_x64/SDL3"
PREMAKE_LIB_DIR_DEBUG="$ROOT_DIR/Build/bin/Debug_x64/SDL3"
mkdir -p "$PREMAKE_LIB_DIR_RELEASE"
mkdir -p "$PREMAKE_LIB_DIR_DEBUG"

if [[ -f "$LIB_FILE" ]]; then
    cp "$LIB_FILE" "$PREMAKE_LIB_DIR_RELEASE/libSDL3.a"
    cp "$LIB_FILE" "$PREMAKE_LIB_DIR_DEBUG/libSDL3.a"
    echo "Copied libSDL3-static.a to Premake5 output directories as libSDL3.a (both Debug and Release)"
elif [[ -f "$LIB_FILE_DEBUG" ]]; then
    cp "$LIB_FILE_DEBUG" "$PREMAKE_LIB_DIR_RELEASE/libSDL3.a"
    cp "$LIB_FILE_DEBUG" "$PREMAKE_LIB_DIR_DEBUG/libSDL3.a"
    echo "Copied libSDL3-static.a (Debug) to Premake5 output directories as libSDL3.a (both Debug and Release)"
fi

# Copy the generated SDL_build_config.h file - CMake puts it in include-config-release/build_config/
BUILD_CONFIG_FILE="$BUILD_DIR/include-config-release/build_config/SDL_build_config.h"
BUILD_CONFIG_FILE_DEBUG="$BUILD_DIR/include-config-debug/build_config/SDL_build_config.h"

if [[ -f "$BUILD_CONFIG_FILE" ]]; then
    cp "$BUILD_CONFIG_FILE" "$INCLUDE_DIR/"
    echo "Copied SDL_build_config.h (Release) to $INCLUDE_DIR"
elif [[ -f "$BUILD_CONFIG_FILE_DEBUG" ]]; then
    cp "$BUILD_CONFIG_FILE_DEBUG" "$INCLUDE_DIR/"
    echo "Copied SDL_build_config.h (Debug) to $INCLUDE_DIR"
else
    echo "Warning: SDL_build_config.h not found. Checking build directory structure..."
    find "$BUILD_DIR" -name "SDL_build_config.h" -exec basename {} \;
fi

echo "SDL3 build complete!"
