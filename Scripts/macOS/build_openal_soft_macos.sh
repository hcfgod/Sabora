#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
OPENAL_SOFT_DIR="$ROOT_DIR/Engine/Vendor/openal-soft"

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

# Check if openal-soft directory exists and has CMakeLists.txt
if [[ ! -f "$OPENAL_SOFT_DIR/CMakeLists.txt" ]]; then
    echo "openal-soft not found. Cloning openal-soft..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$OPENAL_SOFT_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --depth 1 https://github.com/kcat/openal-soft.git openal-soft
        cd "$ROOT_DIR"
    fi
    
    if [[ ! -f "$OPENAL_SOFT_DIR/CMakeLists.txt" ]]; then
        echo "Error: openal-soft CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$OPENAL_SOFT_DIR/build-debug"
BUILD_DIR_RELEASE="$OPENAL_SOFT_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Get number of cores for parallel builds
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building openal-soft with CMake..."

# Configure and build Debug configuration
echo "Configuring openal-soft (Debug)..."
cmake_args_debug=(
    "-S" "$OPENAL_SOFT_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DLIBTYPE=STATIC"
    "-DALSOFT_UTILS=OFF"
    "-DALSOFT_EXAMPLES=OFF"
    "-DALSOFT_TESTS=OFF"
    "-DALSOFT_INSTALL=OFF"
)

cmake "${cmake_args_debug[@]}"

echo "Building openal-soft (Debug) with $NUM_CORES cores..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring openal-soft (Release)..."
cmake_args_release=(
    "-S" "$OPENAL_SOFT_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DLIBTYPE=STATIC"
    "-DALSOFT_UTILS=OFF"
    "-DALSOFT_EXAMPLES=OFF"
    "-DALSOFT_TESTS=OFF"
    "-DALSOFT_INSTALL=OFF"
)

cmake "${cmake_args_release[@]}"

echo "Building openal-soft (Release) with $NUM_CORES cores..."
cmake --build "$BUILD_DIR_RELEASE" --config Release --parallel "$NUM_CORES"

echo "openal-soft built successfully!"

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/openal-soft/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/openal-soft/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Copy libraries
LIBS_TO_COPY=(
    "libopenal.a"
)

# Copy Debug libraries
for lib in "${LIBS_TO_COPY[@]}"; do
    if [[ -f "$BUILD_DIR_DEBUG/$lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$BUILD_DIR_DEBUG/$lib" "$LIB_DIR/${lib_name}-debug.a"
        echo "Copied $lib (Debug) to $LIB_DIR/${lib_name}-debug.a"
    else
        found_lib=$(find "$BUILD_DIR_DEBUG" -name "$lib" -type f | head -1)
        if [[ -n "$found_lib" && -f "$found_lib" ]]; then
            lib_name=$(basename "$lib" .a | sed 's/^lib//')
            cp "$found_lib" "$LIB_DIR/${lib_name}-debug.a"
            echo "Copied $lib (Debug) to $LIB_DIR/${lib_name}-debug.a"
        fi
    fi
done

# Copy Release libraries
for lib in "${LIBS_TO_COPY[@]}"; do
    if [[ -f "$BUILD_DIR_RELEASE/$lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$BUILD_DIR_RELEASE/$lib" "$LIB_DIR/${lib_name}-release.a"
        echo "Copied $lib (Release) to $LIB_DIR/${lib_name}-release.a"
    else
        found_lib=$(find "$BUILD_DIR_RELEASE" -name "$lib" -type f | head -1)
        if [[ -n "$found_lib" && -f "$found_lib" ]]; then
            lib_name=$(basename "$lib" .a | sed 's/^lib//')
            cp "$found_lib" "$LIB_DIR/${lib_name}-release.a"
            echo "Copied $lib (Release) to $LIB_DIR/${lib_name}-release.a"
        fi
    fi
done

# Also copy without suffix for compatibility (use Release version)
for lib in "${LIBS_TO_COPY[@]}"; do
    if [[ -f "$BUILD_DIR_RELEASE/$lib" ]]; then
        cp "$BUILD_DIR_RELEASE/$lib" "$LIB_DIR/"
        echo "Copied $lib (Release, no suffix) to $LIB_DIR"
    fi
done

# Copy headers
HEADER_SOURCE_DIR="$OPENAL_SOFT_DIR/include"
if [[ -d "$HEADER_SOURCE_DIR" ]]; then
    cp -r "$HEADER_SOURCE_DIR"/* "$INCLUDE_DIR/" 2>/dev/null || true
    echo "Copied openal-soft headers to $INCLUDE_DIR"
else
    # Try alternative location
    ALT_HEADER_DIR="$OPENAL_SOFT_DIR/include/AL"
    if [[ -d "$ALT_HEADER_DIR" ]]; then
        mkdir -p "$INCLUDE_DIR/AL"
        cp -r "$ALT_HEADER_DIR"/* "$INCLUDE_DIR/AL/" 2>/dev/null || true
        echo "Copied openal-soft headers to $INCLUDE_DIR"
    fi
fi

echo "openal-soft build complete!"

