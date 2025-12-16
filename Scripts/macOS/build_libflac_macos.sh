#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBFLAC_DIR="$ROOT_DIR/Engine/Vendor/libflac"
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

# Check if libogg is built first (optional dependency for OGG FLAC)
LIBOGG_LIB_DIR="$LIBOGG_DIR/lib"
HAS_LIBOGG=false
if [[ -f "$LIBOGG_LIB_DIR/ogg-debug.a" ]] || [[ -f "$LIBOGG_LIB_DIR/ogg-release.a" ]]; then
    HAS_LIBOGG=true
fi

# Check if libflac directory exists and has CMakeLists.txt
if [[ ! -f "$LIBFLAC_DIR/CMakeLists.txt" ]]; then
    echo "libFLAC not found. Cloning libFLAC..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBFLAC_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/xiph/flac.git libflac
        cd libflac
        git fetch --tags
        git checkout 1.4.3
        cd "$ROOT_DIR"
    else
        # If directory exists but headers are missing, try to checkout tag
        if [[ ! -d "$LIBFLAC_DIR/include/FLAC" ]]; then
            echo "Headers not found, checking out 1.4.3 tag..." >&2
            cd "$LIBFLAC_DIR"
            git fetch --tags
            git checkout 1.4.3
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$LIBFLAC_DIR/CMakeLists.txt" ]]; then
        echo "Error: libFLAC CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$LIBFLAC_DIR/build-debug"
BUILD_DIR_RELEASE="$LIBFLAC_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Configure libogg paths if available
LIBOGG_INCLUDE_DIR="$LIBOGG_DIR/include"
if [[ ! -d "$LIBOGG_INCLUDE_DIR" ]]; then
    LIBOGG_INCLUDE_DIR="$LIBOGG_DIR/src"
fi

# Get number of cores for parallel builds
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building libFLAC with CMake..."

# Configure and build Debug configuration
echo "Configuring libFLAC (Debug)..."
cmake_args_debug=(
    "-S" "$LIBFLAC_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_MANPAGES=OFF"
    "-DINSTALL_CMAKE_CONFIG_MODULE=OFF"
    "-DWITH_STACK_PROTECTOR=OFF"
)

# Add libogg support if available
if [[ "$HAS_LIBOGG" == "true" ]]; then
    OGG_DEBUG_LIB="$LIBOGG_LIB_DIR/ogg-debug.a"
    if [[ -f "$OGG_DEBUG_LIB" ]]; then
        cmake_args_debug+=("-DOGG_ROOT=$LIBOGG_DIR")
        cmake_args_debug+=("-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR")
        cmake_args_debug+=("-DOGG_LIBRARY=$OGG_DEBUG_LIB")
        echo "  Configuring with libogg support (OGG FLAC enabled)" >&2
    fi
fi

cmake "${cmake_args_debug[@]}"

echo "Building libFLAC (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring libFLAC (Release)..."
cmake_args_release=(
    "-S" "$LIBFLAC_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_MANPAGES=OFF"
    "-DINSTALL_CMAKE_CONFIG_MODULE=OFF"
    "-DWITH_STACK_PROTECTOR=OFF"
)

# Add libogg support if available
if [[ "$HAS_LIBOGG" == "true" ]]; then
    OGG_RELEASE_LIB="$LIBOGG_LIB_DIR/ogg-release.a"
    if [[ -f "$OGG_RELEASE_LIB" ]]; then
        cmake_args_release+=("-DOGG_ROOT=$LIBOGG_DIR")
        cmake_args_release+=("-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR")
        cmake_args_release+=("-DOGG_LIBRARY=$OGG_RELEASE_LIB")
        echo "  Configuring with libogg support (OGG FLAC enabled)" >&2
    fi
fi

cmake "${cmake_args_release[@]}"

echo "Building libFLAC (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"

echo "libFLAC built successfully!" >&2

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/libflac/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libflac/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy Debug libraries
FLAC_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libFLAC.a" -o -name "libFLACd.a" | head -1)

if [[ -n "$FLAC_DEBUG_LIB" && -f "$FLAC_DEBUG_LIB" ]]; then
    cp "$FLAC_DEBUG_LIB" "$LIB_DIR/flac-debug.a"
    echo "Copied libFLAC (Debug) to $LIB_DIR/flac-debug.a" >&2
else
    echo "Error: libFLAC Debug library not found!" >&2
    exit 1
fi

# Find and copy Release libraries
FLAC_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libFLAC.a" | head -1)

if [[ -n "$FLAC_RELEASE_LIB" && -f "$FLAC_RELEASE_LIB" ]]; then
    cp "$FLAC_RELEASE_LIB" "$LIB_DIR/flac-release.a"
    echo "Copied libFLAC (Release) to $LIB_DIR/flac-release.a" >&2
else
    echo "Error: libFLAC Release library not found!" >&2
    exit 1
fi

# Copy headers
HEADER_SOURCE_DIR="$LIBFLAC_DIR/include"
if [[ -d "$HEADER_SOURCE_DIR/FLAC" ]]; then
    # Check if source and destination are the same (to avoid copying to itself)
    SOURCE_PATH=$(cd "$HEADER_SOURCE_DIR/FLAC" && pwd)
    DEST_PATH=$(cd "$INCLUDE_DIR" && pwd 2>/dev/null || echo "$INCLUDE_DIR")
    
    if [[ "$SOURCE_PATH" == "$DEST_PATH/FLAC" ]] || [[ "$SOURCE_PATH" == "$DEST_PATH" ]]; then
        echo "Headers already in correct location: $HEADER_SOURCE_DIR/FLAC" >&2
    else
        cp -r "$HEADER_SOURCE_DIR/FLAC" "$INCLUDE_DIR/"
        echo "Copied FLAC headers to $INCLUDE_DIR" >&2
    fi
else
    echo "Warning: FLAC header directory not found. Downloading headers from GitHub..." >&2
    # Download headers directly from GitHub repository
    mkdir -p "$INCLUDE_DIR/FLAC"
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/format.h" -o "$INCLUDE_DIR/FLAC/format.h" || echo "Failed to download format.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/stream_decoder.h" -o "$INCLUDE_DIR/FLAC/stream_decoder.h" || echo "Failed to download stream_decoder.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/stream_encoder.h" -o "$INCLUDE_DIR/FLAC/stream_encoder.h" || echo "Failed to download stream_encoder.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/metadata.h" -o "$INCLUDE_DIR/FLAC/metadata.h" || echo "Failed to download metadata.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/ordinals.h" -o "$INCLUDE_DIR/FLAC/ordinals.h" || echo "Failed to download ordinals.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/export.h" -o "$INCLUDE_DIR/FLAC/export.h" || echo "Failed to download export.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/assert.h" -o "$INCLUDE_DIR/FLAC/assert.h" || echo "Failed to download assert.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/flac/1.4.3/include/FLAC/callback.h" -o "$INCLUDE_DIR/FLAC/callback.h" || echo "Failed to download callback.h" >&2
    
    # Verify headers were downloaded
    if [[ -f "$INCLUDE_DIR/FLAC/format.h" ]] && [[ -f "$INCLUDE_DIR/FLAC/stream_decoder.h" ]]; then
        echo "Downloaded FLAC headers to $INCLUDE_DIR" >&2
    else
        echo "Error: Failed to download FLAC header files!" >&2
        exit 1
    fi
fi

echo "libFLAC build complete!" >&2
