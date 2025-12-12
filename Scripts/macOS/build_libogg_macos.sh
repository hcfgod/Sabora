#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBOGG_DIR="$ROOT_DIR/Engine/Vendor/libogg"

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

# Check if libogg directory exists and has CMakeLists.txt
if [[ ! -f "$LIBOGG_DIR/CMakeLists.txt" ]]; then
    echo "libogg not found. Cloning libogg..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBOGG_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/xiph/ogg.git libogg
        cd libogg
        git fetch --tags
        git checkout v1.3.6
        cd "$ROOT_DIR"
    else
        # If directory exists but headers are missing, try to checkout tag
        if [[ ! -d "$LIBOGG_DIR/include/ogg" ]]; then
            echo "Headers not found, checking out v1.3.6 tag..." >&2
            cd "$LIBOGG_DIR"
            git fetch --tags
            git checkout v1.3.6
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$LIBOGG_DIR/CMakeLists.txt" ]]; then
        echo "Error: libogg CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$LIBOGG_DIR/build-debug"
BUILD_DIR_RELEASE="$LIBOGG_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Get number of cores for parallel builds
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building libogg with CMake..."

# Configure and build Debug configuration
echo "Configuring libogg (Debug)..."
cmake_args_debug=(
    "-S" "$LIBOGG_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_DOCS=OFF"
    "-DINSTALL_PKG_CONFIG_MODULE=OFF"
)

cmake "${cmake_args_debug[@]}"

echo "Building libogg (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring libogg (Release)..."
cmake_args_release=(
    "-S" "$LIBOGG_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_DOCS=OFF"
    "-DINSTALL_PKG_CONFIG_MODULE=OFF"
)

cmake "${cmake_args_release[@]}"

echo "Building libogg (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"

echo "libogg built successfully!" >&2

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/libogg/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libogg/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy Debug library
OGG_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libogg.a" -o -name "liboggd.a" | head -1)
if [[ -n "$OGG_DEBUG_LIB" && -f "$OGG_DEBUG_LIB" ]]; then
    cp "$OGG_DEBUG_LIB" "$LIB_DIR/ogg-debug.a"
    # Create symlink for linker compatibility
    ln -sf "ogg-debug.a" "$LIB_DIR/libogg.a"
    echo "Copied libogg (Debug) to $LIB_DIR/ogg-debug.a" >&2
else
    echo "Error: libogg Debug library not found!" >&2
    exit 1
fi

# Find and copy Release library
OGG_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libogg.a" | head -1)
if [[ -n "$OGG_RELEASE_LIB" && -f "$OGG_RELEASE_LIB" ]]; then
    cp "$OGG_RELEASE_LIB" "$LIB_DIR/ogg-release.a"
    echo "Copied libogg (Release) to $LIB_DIR/ogg-release.a" >&2
else
    echo "Error: libogg Release library not found!" >&2
    exit 1
fi

# Copy headers
HEADER_SOURCE_DIR="$LIBOGG_DIR/include"
if [[ ! -d "$HEADER_SOURCE_DIR" ]]; then
    HEADER_SOURCE_DIR="$LIBOGG_DIR/src"
fi

if [[ -d "$HEADER_SOURCE_DIR/ogg" ]]; then
    cp -r "$HEADER_SOURCE_DIR/ogg" "$INCLUDE_DIR/"
    echo "Copied ogg headers to $INCLUDE_DIR" >&2
else
    echo "Error: ogg header directory not found!" >&2
    exit 1
fi

echo "libogg build complete!" >&2

