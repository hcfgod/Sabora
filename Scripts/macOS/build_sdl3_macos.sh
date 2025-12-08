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

# Function to check if Homebrew is available
has_homebrew() {
    command -v brew >/dev/null 2>&1
}

# Function to install Homebrew
install_homebrew() {
    echo "Homebrew not found. Installing Homebrew..." >&2
    
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # Add Homebrew to PATH for current session
    if [[ -f "/opt/homebrew/bin/brew" ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [[ -f "/usr/local/bin/brew" ]]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi
    
    if has_homebrew; then
        echo "Homebrew installed successfully!" >&2
    else
        echo "Failed to install Homebrew. Please install manually from https://brew.sh/" >&2
        exit 1
    fi
}

# Function to install CMake
install_cmake() {
    echo "CMake not found. Installing CMake..." >&2
    
    if has_homebrew; then
        brew install cmake
    else
        # Try to install Homebrew first
        install_homebrew
        brew install cmake
    fi
    
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

# Configure SDL3 with CMake (unified with Windows options, tuned for macOS)
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
# Note: On macOS/Linux, CMake uses single-config generators (not multi-config like Windows)
# The build type is set via CMAKE_BUILD_TYPE during configuration (set to Release above)
echo "Building SDL3..."
cmake --build "$BUILD_DIR"

echo "SDL3 built successfully!"

# Copy the built library and headers to a location where premake5 can find them
LIB_DIR="$ROOT_DIR/Engine/Vendor/SDL/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/SDL/include"

mkdir -p "$LIB_DIR"

# Copy the static library
# On macOS/Linux, CMake uses single-config generators, so the library is built directly
# in the build directory, not in Release/Debug subdirectories
# The library name is typically libSDL3-static.a or libSDL3.a

# Try to find the SDL3 static library
LIB_FILE=""
# Check common locations and names
if [[ -f "$BUILD_DIR/libSDL3-static.a" ]]; then
    LIB_FILE="$BUILD_DIR/libSDL3-static.a"
elif [[ -f "$BUILD_DIR/libSDL3.a" ]]; then
    LIB_FILE="$BUILD_DIR/libSDL3.a"
else
    # Search for any SDL3 library file
    LIB_FILE=$(find "$BUILD_DIR" -name "*SDL3*.a" -type f | head -1)
fi

if [[ -z "$LIB_FILE" || ! -f "$LIB_FILE" ]]; then
    echo "Error: SDL3 library not found in build directory!"
    echo "Build directory: $BUILD_DIR"
    echo "Searching for all .a files..."
    find "$BUILD_DIR" -name "*.a" -type f | head -10
    exit 1
fi

# Copy the library to the expected location
cp "$LIB_FILE" "$LIB_DIR/libSDL3.a"
echo "Copied $(basename "$LIB_FILE") to $LIB_DIR/libSDL3.a"

# Also copy to Premake output directories for convenience
if [[ -n "$LIB_FILE" && -f "$LIB_FILE" ]]; then
    for cfg in Debug_x64 Release_x64 Debug_ARM64 Release_ARM64; do
        PREMAKE_LIB_DIR="$ROOT_DIR/Build/bin/$cfg/SDL3"
        mkdir -p "$PREMAKE_LIB_DIR"
        cp "$LIB_FILE" "$PREMAKE_LIB_DIR/libSDL3.a"
    done
    echo "Copied libSDL3.a to Premake output directories (Debug/Release x64 and ARM64)"
fi

# Copy the generated SDL_build_config.h file
# Path depends on build type; prefer Release, fallback to RelWithDebInfo
BUILD_CONFIG_FILE="$BUILD_DIR/include-config-release/build_config/SDL_build_config.h"
BUILD_CONFIG_FILE_DEBUG="$BUILD_DIR/include-config-relwithdebinfo/build_config/SDL_build_config.h"

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