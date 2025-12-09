#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
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

# Check if SDL3 directory exists and has CMakeLists.txt
if [[ ! -f "$SDL_DIR/CMakeLists.txt" ]]; then
    echo "SDL3 not found. Cloning SDL3..." >&2
    
    # Ensure Vendor directory exists
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    # Clone SDL3 if it doesn't exist
    if [[ ! -d "$SDL_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --depth 1 https://github.com/libsdl-org/SDL.git SDL
        cd "$ROOT_DIR"
    fi
    
    # Verify CMakeLists.txt exists now
    if [[ ! -f "$SDL_DIR/CMakeLists.txt" ]]; then
        echo "Error: SDL3 CMakeLists.txt not found after cloning. SDL directory: $SDL_DIR" >&2
        exit 1
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Building SDL3 with CMake..."

# Configure SDL3 with CMake
cmake_args=(
    "-S" "$SDL_DIR"
    "-B" "$BUILD_DIR"

    # Build configuration
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"

    # Build static, not shared, and avoid installing extras
    "-DSDL_STATIC=ON"
    "-DSDL_SHARED=OFF"
    "-DSDL_INSTALL=OFF"
    "-DSDL_TESTS=OFF"
    "-DSDL_EXAMPLES=OFF"
    "-DSDL_INSTALL_TESTS=OFF"
    "-DSDL_DEPS_SHARED=OFF"
    "-DSDL_RPATH=OFF"

    # Subsystems (Windows-specific)
    "-DSDL_AUDIO=ON"
    "-DSDL_VIDEO=ON"
    "-DSDL_GPU=ON"
    "-DSDL_RENDER=ON"
    "-DSDL_CAMERA=ON"
    "-DSDL_JOYSTICK=ON"
    "-DSDL_HAPTIC=ON"
    "-DSDL_HIDAPI=ON"
    "-DSDL_POWER=ON"
    "-DSDL_SENSOR=ON"
    "-DSDL_DIALOG=ON"

    # Windows backends
    "-DSDL_DIRECTX=ON"
    "-DSDL_RENDER_D3D=ON"
    "-DSDL_RENDER_D3D11=ON"
    "-DSDL_RENDER_D3D12=ON"
    "-DSDL_RENDER_GPU=ON"
    "-DSDL_RENDER_VULKAN=ON"
    "-DSDL_WASAPI=ON"
    "-DSDL_XINPUT=ON"

    # Graphics APIs
    "-DSDL_OPENGL=ON"
    "-DSDL_OPENGLES=ON"
    "-DSDL_VULKAN=ON"

    # Audio backends
    "-DSDL_DISKAUDIO=ON"
    "-DSDL_DUMMYAUDIO=ON"

    # Video backends
    "-DSDL_DUMMYVIDEO=ON"
    "-DSDL_OFFSCREEN=ON"

    # Camera backends
    "-DSDL_DUMMYCAMERA=ON"

    # Input backends
    "-DSDL_VIRTUAL_JOYSTICK=ON"

    # CPU optimizations
    "-DSDL_ASSEMBLY=ON"
    "-DSDL_AVX=ON"
    "-DSDL_AVX2=ON"
    "-DSDL_AVX512F=ON"
    "-DSDL_SSE=ON"
    "-DSDL_SSE2=ON"
    "-DSDL_SSE3=ON"
    "-DSDL_SSE4_1=ON"
    "-DSDL_SSE4_2=ON"
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

# Copy the static library - SDL3 CMake may output different names
# Try multiple possible locations and names
LIB_FILE=""
if [[ -f "$BUILD_DIR/libSDL3.a" ]]; then
    LIB_FILE="$BUILD_DIR/libSDL3.a"
elif [[ -f "$BUILD_DIR/libSDL3-static.a" ]]; then
    LIB_FILE="$BUILD_DIR/libSDL3-static.a"
elif [[ -f "$BUILD_DIR/Debug/libSDL3.a" ]]; then
    LIB_FILE="$BUILD_DIR/Debug/libSDL3.a"
elif [[ -f "$BUILD_DIR/Debug/libSDL3-static.a" ]]; then
    LIB_FILE="$BUILD_DIR/Debug/libSDL3-static.a"
else
    # Search for any SDL3 library file
    LIB_FILE=$(find "$BUILD_DIR" -name "*SDL3*.a" -type f | head -1)
fi

if [[ -n "$LIB_FILE" && -f "$LIB_FILE" ]]; then
    cp "$LIB_FILE" "$LIB_DIR/libSDL3.a"
    echo "Copied $(basename "$LIB_FILE") to $LIB_DIR/libSDL3.a"
else
    echo "Error: SDL3 library not found. Checking build directory structure..."
    find "$BUILD_DIR" -name "*.a" -type f
    exit 1
fi

# Also copy to the Premake5 output directory for linking
# Copy to both Debug and Release directories to handle both build configurations
if [[ -n "$LIB_FILE" && -f "$LIB_FILE" ]]; then
    PREMAKE_LIB_DIR_RELEASE="$ROOT_DIR/Build/bin/Release_x64/SDL3"
    PREMAKE_LIB_DIR_DEBUG="$ROOT_DIR/Build/bin/Debug_x64/SDL3"
    mkdir -p "$PREMAKE_LIB_DIR_RELEASE"
    mkdir -p "$PREMAKE_LIB_DIR_DEBUG"
    
    cp "$LIB_FILE" "$PREMAKE_LIB_DIR_RELEASE/libSDL3.a"
    cp "$LIB_FILE" "$PREMAKE_LIB_DIR_DEBUG/libSDL3.a"
    echo "Copied libSDL3.a to Premake5 output directories (both Debug and Release)"
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
