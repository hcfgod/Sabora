#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBOGG_DIR="$ROOT_DIR/Engine/Vendor/libogg"

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
cmake --build "$BUILD_DIR_DEBUG" --parallel "$(nproc)"

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
cmake --build "$BUILD_DIR_RELEASE" --parallel "$(nproc)"

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
    # Ensure symlink exists for Release (Debug symlink may already exist)
    if [[ ! -L "$LIB_DIR/libogg.a" ]] || [[ "$(readlink "$LIB_DIR/libogg.a")" != "ogg-debug.a" ]]; then
        ln -sf "ogg-debug.a" "$LIB_DIR/libogg.a"
    fi
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
    # Check if source and destination are the same (to avoid copying to itself)
    SOURCE_PATH=$(cd "$HEADER_SOURCE_DIR/ogg" && pwd)
    DEST_PATH=$(cd "$INCLUDE_DIR" && pwd 2>/dev/null || echo "$INCLUDE_DIR")
    
    if [[ "$SOURCE_PATH" == "$DEST_PATH/ogg" ]] || [[ "$SOURCE_PATH" == "$DEST_PATH" ]]; then
        echo "Headers already in correct location: $HEADER_SOURCE_DIR/ogg" >&2
    else
        cp -r "$HEADER_SOURCE_DIR/ogg" "$INCLUDE_DIR/"
        echo "Copied ogg headers to $INCLUDE_DIR" >&2
    fi
else
    echo "Error: ogg header directory not found!" >&2
    exit 1
fi

# Copy config_types.h from build directory (generated by CMake)
# This file is required by os_types.h
CONFIG_TYPES_DEBUG="$BUILD_DIR_DEBUG/include/ogg/config_types.h"
CONFIG_TYPES_RELEASE="$BUILD_DIR_RELEASE/include/ogg/config_types.h"
CONFIG_TYPES_DEST="$INCLUDE_DIR/ogg/config_types.h"

if [[ -f "$CONFIG_TYPES_DEBUG" ]]; then
    cp "$CONFIG_TYPES_DEBUG" "$CONFIG_TYPES_DEST"
    echo "Copied config_types.h from Debug build" >&2
elif [[ -f "$CONFIG_TYPES_RELEASE" ]]; then
    cp "$CONFIG_TYPES_RELEASE" "$CONFIG_TYPES_DEST"
    echo "Copied config_types.h from Release build" >&2
else
    echo "Warning: config_types.h not found in build directories" >&2
    echo "Searching for config_types.h..." >&2
    FOUND_CONFIG=$(find "$BUILD_DIR_DEBUG" "$BUILD_DIR_RELEASE" -name "config_types.h" 2>/dev/null | head -1)
    if [[ -n "$FOUND_CONFIG" && -f "$FOUND_CONFIG" ]]; then
        cp "$FOUND_CONFIG" "$CONFIG_TYPES_DEST"
        echo "Found and copied config_types.h from $FOUND_CONFIG" >&2
    else
        echo "Error: config_types.h not found! This file is required." >&2
        exit 1
    fi
fi

echo "libogg build complete!" >&2

