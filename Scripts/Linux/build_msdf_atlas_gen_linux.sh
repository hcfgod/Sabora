#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
MSDF_ATLAS_GEN_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen"

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

# Check if msdf-atlas-gen directory exists and has CMakeLists.txt
if [[ ! -f "$MSDF_ATLAS_GEN_DIR/CMakeLists.txt" ]]; then
    echo "msdf-atlas-gen not found. Cloning msdf-atlas-gen..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$MSDF_ATLAS_GEN_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --recursive --depth 1 https://github.com/Chlumsky/msdf-atlas-gen.git msdf-atlas-gen
        cd "$ROOT_DIR"
    else
        # If directory exists, make sure submodules are initialized
        cd "$MSDF_ATLAS_GEN_DIR"
        git submodule update --init --recursive
        cd "$ROOT_DIR"
    fi
    
    if [[ ! -f "$MSDF_ATLAS_GEN_DIR/CMakeLists.txt" ]]; then
        echo "Error: msdf-atlas-gen CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$MSDF_ATLAS_GEN_DIR/build-debug"
BUILD_DIR_RELEASE="$MSDF_ATLAS_GEN_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

echo "Building msdf-atlas-gen with CMake..."

# Configure and build Debug configuration
echo "Configuring msdf-atlas-gen (Debug)..."
cmake_args_debug=(
    "-S" "$MSDF_ATLAS_GEN_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_CXX_STANDARD=17"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF"
    "-DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF"
    "-DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON"
    "-DMSDF_ATLAS_GEN_USE_VCPKG=OFF"
    "-DMSDF_ATLAS_GEN_USE_PNG=OFF"
)

cmake "${cmake_args_debug[@]}"

echo "Building msdf-atlas-gen (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug --parallel "$(nproc)"

# Configure and build Release configuration
echo "Configuring msdf-atlas-gen (Release)..."
cmake_args_release=(
    "-S" "$MSDF_ATLAS_GEN_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_CXX_STANDARD=17"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF"
    "-DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF"
    "-DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON"
    "-DMSDF_ATLAS_GEN_USE_VCPKG=OFF"
    "-DMSDF_ATLAS_GEN_USE_PNG=OFF"
)

cmake "${cmake_args_release[@]}"

echo "Building msdf-atlas-gen (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release --parallel "$(nproc)"

echo "msdf-atlas-gen built successfully!"

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Copy libraries
LIBS_TO_COPY=(
    "libmsdf-atlas-gen.a"
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
HEADER_SOURCE_DIR="$MSDF_ATLAS_GEN_DIR/include"
if [[ -d "$HEADER_SOURCE_DIR" ]]; then
    cp -r "$HEADER_SOURCE_DIR"/* "$INCLUDE_DIR/" 2>/dev/null || true
    echo "Copied msdf-atlas-gen headers to $INCLUDE_DIR"
else
    # Try alternative locations
    ALT_HEADER_DIRS=(
        "$MSDF_ATLAS_GEN_DIR/msdf-atlas-gen"
        "$MSDF_ATLAS_GEN_DIR/src"
    )
    for alt_dir in "${ALT_HEADER_DIRS[@]}"; do
        if [[ -d "$alt_dir" ]]; then
            find "$alt_dir" -name "*.h" -o -name "*.hpp" | while read -r header; do
                cp "$header" "$INCLUDE_DIR/" 2>/dev/null || true
            done
        fi
    done
    echo "Copied msdf-atlas-gen headers to $INCLUDE_DIR (from alternative location)"
fi

echo "msdf-atlas-gen build complete!"

