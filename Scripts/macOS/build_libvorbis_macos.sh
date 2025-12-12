#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBVORBIS_DIR="$ROOT_DIR/Engine/Vendor/libvorbis"
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

# Check if libogg is built first (dependency)
LIBOGG_LIB_DIR="$LIBOGG_DIR/lib"
if [[ ! -f "$LIBOGG_LIB_DIR/ogg-debug.a" ]] && [[ ! -f "$LIBOGG_LIB_DIR/ogg-release.a" ]]; then
    echo "Error: libogg must be built before libvorbis. Please run build_libogg_macos.sh first." >&2
    exit 1
fi

# Check if libvorbis directory exists and has CMakeLists.txt
if [[ ! -f "$LIBVORBIS_DIR/CMakeLists.txt" ]]; then
    echo "libvorbis not found. Cloning libvorbis..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBVORBIS_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/xiph/vorbis.git libvorbis
        cd libvorbis
        git fetch --tags
        git checkout v1.3.7
        cd "$ROOT_DIR"
    else
        # If directory exists but headers are missing, try to checkout tag
        if [[ ! -d "$LIBVORBIS_DIR/include/vorbis" ]]; then
            echo "Headers not found, checking out v1.3.7 tag..." >&2
            cd "$LIBVORBIS_DIR"
            git fetch --tags
            git checkout v1.3.7
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$LIBVORBIS_DIR/CMakeLists.txt" ]]; then
        echo "Error: libvorbis CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$LIBVORBIS_DIR/build-debug"
BUILD_DIR_RELEASE="$LIBVORBIS_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Configure libogg paths
LIBOGG_INCLUDE_DIR="$LIBOGG_DIR/include"
if [[ ! -d "$LIBOGG_INCLUDE_DIR" ]]; then
    LIBOGG_INCLUDE_DIR="$LIBOGG_DIR/src"
fi

# Get number of cores for parallel builds
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building libvorbis with CMake..."

# Configure and build Debug configuration
echo "Configuring libvorbis (Debug)..."
cmake_args_debug=(
    "-S" "$LIBVORBIS_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_DOCS=OFF"
    "-DINSTALL_PKG_CONFIG_MODULE=OFF"
    "-DOGG_ROOT=$LIBOGG_DIR"
    "-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR"
    "-DOGG_LIBRARY=$LIBOGG_LIB_DIR/ogg-debug.a"
    "-DCMAKE_PREFIX_PATH=$LIBOGG_DIR"
)

cmake "${cmake_args_debug[@]}"

echo "Building libvorbis (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring libvorbis (Release)..."
cmake_args_release=(
    "-S" "$LIBVORBIS_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DINSTALL_DOCS=OFF"
    "-DINSTALL_PKG_CONFIG_MODULE=OFF"
    "-DOGG_ROOT=$LIBOGG_DIR"
    "-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR"
    "-DOGG_LIBRARY=$LIBOGG_LIB_DIR/ogg-release.a"
    "-DCMAKE_PREFIX_PATH=$LIBOGG_DIR"
)

cmake "${cmake_args_release[@]}"

echo "Building libvorbis (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"

echo "libvorbis built successfully!" >&2

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/libvorbis/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libvorbis/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy Debug libraries (vorbis, vorbisfile, and vorbisenc)
VORBIS_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libvorbis.a" -o -name "libvorbisd.a" | head -1)
VORBISFILE_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libvorbisfile.a" -o -name "libvorbisfiled.a" | head -1)
VORBISENC_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libvorbisenc.a" -o -name "libvorbisencd.a" | head -1)

if [[ -n "$VORBIS_DEBUG_LIB" && -f "$VORBIS_DEBUG_LIB" ]]; then
    cp "$VORBIS_DEBUG_LIB" "$LIB_DIR/vorbis-debug.a"
    # Create symlink for linker compatibility
    ln -sf "vorbis-debug.a" "$LIB_DIR/libvorbis.a"
    echo "Copied libvorbis (Debug) to $LIB_DIR/vorbis-debug.a" >&2
else
    echo "Error: libvorbis Debug library not found!" >&2
    exit 1
fi

if [[ -n "$VORBISFILE_DEBUG_LIB" && -f "$VORBISFILE_DEBUG_LIB" ]]; then
    cp "$VORBISFILE_DEBUG_LIB" "$LIB_DIR/vorbisfile-debug.a"
    # Create symlink for linker compatibility
    ln -sf "vorbisfile-debug.a" "$LIB_DIR/libvorbisfile.a"
    echo "Copied libvorbisfile (Debug) to $LIB_DIR/vorbisfile-debug.a" >&2
else
    echo "Error: libvorbisfile Debug library not found!" >&2
    exit 1
fi

if [[ -n "$VORBISENC_DEBUG_LIB" && -f "$VORBISENC_DEBUG_LIB" ]]; then
    cp "$VORBISENC_DEBUG_LIB" "$LIB_DIR/vorbisenc-debug.a"
    # Create symlink for linker compatibility
    ln -sf "vorbisenc-debug.a" "$LIB_DIR/libvorbisenc.a"
    echo "Copied libvorbisenc (Debug) to $LIB_DIR/vorbisenc-debug.a" >&2
else
    echo "Warning: libvorbisenc Debug library not found (may not be built)" >&2
fi

# Find and copy Release libraries
VORBIS_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libvorbis.a" | head -1)
VORBISFILE_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libvorbisfile.a" | head -1)
VORBISENC_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libvorbisenc.a" | head -1)

if [[ -n "$VORBIS_RELEASE_LIB" && -f "$VORBIS_RELEASE_LIB" ]]; then
    cp "$VORBIS_RELEASE_LIB" "$LIB_DIR/vorbis-release.a"
    echo "Copied libvorbis (Release) to $LIB_DIR/vorbis-release.a" >&2
else
    echo "Error: libvorbis Release library not found!" >&2
    exit 1
fi

if [[ -n "$VORBISFILE_RELEASE_LIB" && -f "$VORBISFILE_RELEASE_LIB" ]]; then
    cp "$VORBISFILE_RELEASE_LIB" "$LIB_DIR/vorbisfile-release.a"
    echo "Copied libvorbisfile (Release) to $LIB_DIR/vorbisfile-release.a" >&2
else
    echo "Error: libvorbisfile Release library not found!" >&2
    exit 1
fi

if [[ -n "$VORBISENC_RELEASE_LIB" && -f "$VORBISENC_RELEASE_LIB" ]]; then
    cp "$VORBISENC_RELEASE_LIB" "$LIB_DIR/vorbisenc-release.a"
    echo "Copied libvorbisenc (Release) to $LIB_DIR/vorbisenc-release.a" >&2
else
    echo "Warning: libvorbisenc Release library not found (may not be built)" >&2
fi

# Copy headers
HEADER_SOURCE_DIR="$LIBVORBIS_DIR/include"
if [[ ! -d "$HEADER_SOURCE_DIR" ]]; then
    HEADER_SOURCE_DIR="$LIBVORBIS_DIR/lib"
fi

if [[ -d "$HEADER_SOURCE_DIR/vorbis" ]]; then
    # Check if source and destination are the same (to avoid copying to itself)
    SOURCE_PATH=$(cd "$HEADER_SOURCE_DIR/vorbis" && pwd)
    DEST_PATH=$(cd "$INCLUDE_DIR" && pwd 2>/dev/null || echo "$INCLUDE_DIR")
    
    if [[ "$SOURCE_PATH" == "$DEST_PATH/vorbis" ]] || [[ "$SOURCE_PATH" == "$DEST_PATH" ]]; then
        echo "Headers already in correct location: $HEADER_SOURCE_DIR/vorbis" >&2
    else
        cp -r "$HEADER_SOURCE_DIR/vorbis" "$INCLUDE_DIR/"
        echo "Copied vorbis headers to $INCLUDE_DIR" >&2
    fi
else
    echo "Warning: vorbis header directory not found. Downloading headers from GitHub..." >&2
    # Download headers directly from GitHub repository
    mkdir -p "$INCLUDE_DIR/vorbis"
    curl -sL "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/codec.h" -o "$INCLUDE_DIR/vorbis/codec.h" || echo "Failed to download codec.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/vorbisenc.h" -o "$INCLUDE_DIR/vorbis/vorbisenc.h" || echo "Failed to download vorbisenc.h" >&2
    curl -sL "https://raw.githubusercontent.com/xiph/vorbis/v1.3.7/include/vorbis/vorbisfile.h" -o "$INCLUDE_DIR/vorbis/vorbisfile.h" || echo "Failed to download vorbisfile.h" >&2
    
    # Verify headers were downloaded
    if [[ -f "$INCLUDE_DIR/vorbis/codec.h" ]] && [[ -f "$INCLUDE_DIR/vorbis/vorbisenc.h" ]] && [[ -f "$INCLUDE_DIR/vorbis/vorbisfile.h" ]]; then
        echo "Downloaded vorbis headers to $INCLUDE_DIR" >&2
    else
        echo "Error: Failed to download vorbis header files!" >&2
        exit 1
    fi
fi

echo "libvorbis build complete!" >&2

