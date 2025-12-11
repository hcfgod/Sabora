#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBSNDFILE_DIR="$ROOT_DIR/Engine/Vendor/libsndfile"

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

# Check if libsndfile directory exists and has CMakeLists.txt
if [[ ! -f "$LIBSNDFILE_DIR/CMakeLists.txt" ]]; then
    echo "libsndfile not found. Cloning libsndfile..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBSNDFILE_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --depth 1 https://github.com/libsndfile/libsndfile.git libsndfile
        cd "$ROOT_DIR"
    fi
    
    if [[ ! -f "$LIBSNDFILE_DIR/CMakeLists.txt" ]]; then
        echo "Error: libsndfile CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$LIBSNDFILE_DIR/build-debug"
BUILD_DIR_RELEASE="$LIBSNDFILE_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Get number of cores for parallel builds
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building libsndfile with CMake..."

# Configure and build Debug configuration
echo "Configuring libsndfile (Debug)..."
cmake_args_debug=(
    "-S" "$LIBSNDFILE_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DBUILD_STATIC_LIBS=ON"
    "-DBUILD_PROGRAMS=OFF"
    "-DBUILD_EXAMPLES=OFF"
    "-DBUILD_TESTING=OFF"
    "-DENABLE_EXTERNAL_LIBS=OFF"
    "-DINSTALL_PKGCONFIG_MODULE=OFF"
    "-DINSTALL_MANPAGES=OFF"
)

cmake "${cmake_args_debug[@]}"

echo "Building libsndfile (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring libsndfile (Release)..."
cmake_args_release=(
    "-S" "$LIBSNDFILE_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DBUILD_STATIC_LIBS=ON"
    "-DBUILD_PROGRAMS=OFF"
    "-DBUILD_EXAMPLES=OFF"
    "-DBUILD_TESTING=OFF"
    "-DENABLE_EXTERNAL_LIBS=OFF"
    "-DINSTALL_PKGCONFIG_MODULE=OFF"
    "-DINSTALL_MANPAGES=OFF"
)

cmake "${cmake_args_release[@]}"

echo "Building libsndfile (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release --parallel "$NUM_CORES"

echo "libsndfile built successfully!"

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/libsndfile/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libsndfile/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Copy libraries
LIBS_TO_COPY=(
    "libsndfile.a"
)

# Copy Debug libraries
for lib in "${LIBS_TO_COPY[@]}"; do
    found_lib=$(find "$BUILD_DIR_DEBUG" -name "$lib" -type f | head -1)
    if [[ -n "$found_lib" && -f "$found_lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$found_lib" "$LIB_DIR/${lib_name}-debug.a"
        # Create symlink with expected name for linker (points to debug version)
        ln -sf "${lib_name}-debug.a" "$LIB_DIR/$lib"
        echo "Copied $lib (Debug) to $LIB_DIR/${lib_name}-debug.a and created symlink $lib"
    fi
done

# Copy Release libraries
for lib in "${LIBS_TO_COPY[@]}"; do
    found_lib=$(find "$BUILD_DIR_RELEASE" -name "$lib" -type f | head -1)
    if [[ -n "$found_lib" && -f "$found_lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$found_lib" "$LIB_DIR/${lib_name}-release.a"
        echo "Copied $lib (Release) to $LIB_DIR/${lib_name}-release.a"
    fi
done

# Copy headers
HEADER_SOURCE_DIR="$LIBSNDFILE_DIR/include"
if [[ -d "$HEADER_SOURCE_DIR" ]] && [[ -f "$HEADER_SOURCE_DIR/sndfile.h" ]]; then
    cp "$HEADER_SOURCE_DIR/sndfile.h" "$INCLUDE_DIR/" 2>/dev/null || true
    echo "Copied libsndfile headers to $INCLUDE_DIR"
else
    # Try alternative location (src directory)
    if [[ -f "$LIBSNDFILE_DIR/src/sndfile.h" ]]; then
        cp "$LIBSNDFILE_DIR/src/sndfile.h" "$INCLUDE_DIR/" 2>/dev/null || true
        echo "Copied libsndfile headers (from src) to $INCLUDE_DIR"
    else
        # Search for sndfile.h
        found_header=$(find "$LIBSNDFILE_DIR" -name "sndfile.h" -type f | head -1)
        if [[ -n "$found_header" && -f "$found_header" ]]; then
            cp "$found_header" "$INCLUDE_DIR/" 2>/dev/null || true
            echo "Copied libsndfile headers (found at $found_header) to $INCLUDE_DIR"
        else
            echo "Warning: sndfile.h not found in expected locations" >&2
        fi
    fi
fi

echo "libsndfile build complete!"

