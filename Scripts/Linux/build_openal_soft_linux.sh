#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
OPENAL_SOFT_DIR="$ROOT_DIR/Engine/Vendor/openal-soft"

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

echo "Building openal-soft (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug --parallel "$(nproc)"

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

echo "Building openal-soft (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release --parallel "$(nproc)"

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

