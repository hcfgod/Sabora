#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBOPUS_DIR="$ROOT_DIR/Engine/Vendor/libopus"

# Function to check if a command is available
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install CMake
install_cmake() {
    echo "CMake not found. Installing CMake..." >&2
    
    if command_exists apt-get; then
        sudo apt-get update
        sudo apt-get install -y cmake build-essential
    elif command_exists dnf; then
        sudo dnf install -y cmake gcc gcc-c++ make
    elif command_exists yum; then
        sudo yum install -y cmake gcc gcc-c++ make
    elif command_exists pacman; then
        sudo pacman -S --noconfirm cmake base-devel
    elif command_exists zypper; then
        sudo zypper install -y cmake gcc-c++ make
    else
        echo "Unsupported package manager. Please install CMake manually." >&2
        exit 1
    fi
}

# Check and install CMake if needed
if ! command_exists cmake; then
    install_cmake
fi

echo "CMake is available!" >&2

# Check if libopus directory exists and has CMakeLists.txt
if [[ ! -f "$LIBOPUS_DIR/CMakeLists.txt" ]]; then
    echo "libopus not found. Cloning libopus..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBOPUS_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://gitlab.xiph.org/xiph/opus.git libopus
        cd "$ROOT_DIR"
    else
        # If directory exists but headers are missing, try to update
        if [[ ! -d "$LIBOPUS_DIR/include/opus" ]]; then
            echo "Headers not found, updating repository..." >&2
            cd "$LIBOPUS_DIR"
            git pull || echo "Warning: git pull failed, but continuing..." >&2
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$LIBOPUS_DIR/CMakeLists.txt" ]]; then
        echo "Error: libopus CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$LIBOPUS_DIR/build-debug"
BUILD_DIR_RELEASE="$LIBOPUS_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Get number of cores for parallel builds
NUM_CORES=$(nproc 2>/dev/null || echo 4)

echo "Building libopus with CMake..."

# Configure and build Debug configuration
echo "Configuring libopus (Debug)..."
cmake_args_debug=(
    "-S" "$LIBOPUS_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DOPUS_BUILD_PROGRAMS=OFF"
    "-DOPUS_BUILD_DOCS=OFF"
    "-DOPUS_BUILD_TESTS=OFF"
    "-DOPUS_BUILD_EXAMPLES=OFF"
)

cmake "${cmake_args_debug[@]}"

echo "Building libopus (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring libopus (Release)..."
cmake_args_release=(
    "-S" "$LIBOPUS_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DOPUS_BUILD_PROGRAMS=OFF"
    "-DOPUS_BUILD_DOCS=OFF"
    "-DOPUS_BUILD_TESTS=OFF"
    "-DOPUS_BUILD_EXAMPLES=OFF"
)

cmake "${cmake_args_release[@]}"

echo "Building libopus (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"

echo "libopus built successfully!" >&2

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/libopus/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libopus/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy Debug libraries
OPUS_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libopus.a" -o -name "libopusd.a" | head -1)

if [[ -n "$OPUS_DEBUG_LIB" && -f "$OPUS_DEBUG_LIB" ]]; then
    cp "$OPUS_DEBUG_LIB" "$LIB_DIR/opus-debug.a"
    # Create symlink with expected name for linker (points to debug version)
    # This allows the linker to find libopus.a when using -lopus
    rm -f "$LIB_DIR/libopus.a"
    ln -sf "opus-debug.a" "$LIB_DIR/libopus.a"
    echo "Copied libopus (Debug) to $LIB_DIR/opus-debug.a" >&2
    echo "Created symlink: libopus.a -> opus-debug.a" >&2
else
    echo "Error: libopus Debug library not found!" >&2
    exit 1
fi

# Find and copy Release libraries
OPUS_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libopus.a" | head -1)

if [[ -n "$OPUS_RELEASE_LIB" && -f "$OPUS_RELEASE_LIB" ]]; then
    cp "$OPUS_RELEASE_LIB" "$LIB_DIR/opus-release.a"
    echo "Copied libopus (Release) to $LIB_DIR/opus-release.a" >&2
else
    echo "Error: libopus Release library not found!" >&2
    exit 1
fi

# Verify symlink exists and is correct
if [[ ! -L "$LIB_DIR/libopus.a" ]] || [[ ! -f "$LIB_DIR/libopus.a" ]]; then
    echo "Error: libopus.a symlink is missing or broken!" >&2
    exit 1
fi

# Verify symlink points to the debug version
SYMLINK_TARGET=$(readlink -f "$LIB_DIR/libopus.a")
if [[ "$SYMLINK_TARGET" != "$(cd "$LIB_DIR" && pwd)/opus-debug.a" ]]; then
    echo "Error: libopus.a symlink points to wrong target!" >&2
    exit 1
fi

echo "Verified libopus symlink is correct (points to opus-debug.a)" >&2

# Copy headers
HEADER_SOURCE_DIR="$LIBOPUS_DIR/include"
OPUS_HEADER_DEST_DIR="$INCLUDE_DIR/opus"

# Create opus subdirectory in include
mkdir -p "$OPUS_HEADER_DEST_DIR"

# Copy all header files from include to include/opus
if [[ -d "$HEADER_SOURCE_DIR" ]]; then
    find "$HEADER_SOURCE_DIR" -maxdepth 1 -name "*.h" -type f -exec cp {} "$OPUS_HEADER_DEST_DIR/" \;
    echo "Copied opus headers to $OPUS_HEADER_DEST_DIR" >&2
else
    echo "Warning: opus header directory not found at $HEADER_SOURCE_DIR" >&2
    echo "You may need to manually copy the headers or rebuild libopus." >&2
fi

echo "libopus build complete!" >&2
