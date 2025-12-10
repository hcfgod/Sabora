#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SPIRV_CROSS_DIR="$ROOT_DIR/Engine/Vendor/SPIRV-Cross"
BUILD_DIR="$SPIRV_CROSS_DIR/build"

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

# Check if SPIRV-Cross directory exists and has CMakeLists.txt
if [[ ! -f "$SPIRV_CROSS_DIR/CMakeLists.txt" ]]; then
    echo "SPIRV-Cross not found. Cloning SPIRV-Cross..." >&2
    
    # Ensure Vendor directory exists
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    # Clone SPIRV-Cross if it doesn't exist
    if [[ ! -d "$SPIRV_CROSS_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --depth 1 https://github.com/KhronosGroup/SPIRV-Cross.git SPIRV-Cross
        cd "$ROOT_DIR"
    fi
    
    # Verify CMakeLists.txt exists now
    if [[ ! -f "$SPIRV_CROSS_DIR/CMakeLists.txt" ]]; then
        echo "Error: SPIRV-Cross CMakeLists.txt not found after cloning. SPIRV-Cross directory: $SPIRV_CROSS_DIR" >&2
        exit 1
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$SPIRV_CROSS_DIR/build-debug"
BUILD_DIR_RELEASE="$SPIRV_CROSS_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

echo "Building SPIRV-Cross with CMake..."

# Configure and build Debug configuration
echo "Configuring SPIRV-Cross (Debug)..."
cmake_args_debug=(
    "-S" "$SPIRV_CROSS_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_CXX_STANDARD=17"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DSPIRV_CROSS_CLI=OFF"
    "-DSPIRV_CROSS_ENABLE_TESTS=OFF"
    "-DSPIRV_CROSS_SHARED=OFF"
    "-DSPIRV_CROSS_STATIC=ON"
    "-DSPIRV_CROSS_ENABLE_CPP=ON"
    "-DSPIRV_CROSS_ENABLE_GLSL=ON"
    "-DSPIRV_CROSS_ENABLE_HLSL=ON"
    "-DSPIRV_CROSS_ENABLE_MSL=ON"
    "-DSPIRV_CROSS_ENABLE_REFLECT=ON"
    "-DSPIRV_CROSS_ENABLE_UTIL=ON"
)

cmake "${cmake_args_debug[@]}"

echo "Building SPIRV-Cross (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug --parallel "$(nproc)"

# Configure and build Release configuration
echo "Configuring SPIRV-Cross (Release)..."
cmake_args_release=(
    "-S" "$SPIRV_CROSS_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_CXX_STANDARD=17"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DSPIRV_CROSS_CLI=OFF"
    "-DSPIRV_CROSS_ENABLE_TESTS=OFF"
    "-DSPIRV_CROSS_SHARED=OFF"
    "-DSPIRV_CROSS_STATIC=ON"
    "-DSPIRV_CROSS_ENABLE_CPP=ON"
    "-DSPIRV_CROSS_ENABLE_GLSL=ON"
    "-DSPIRV_CROSS_ENABLE_HLSL=ON"
    "-DSPIRV_CROSS_ENABLE_MSL=ON"
    "-DSPIRV_CROSS_ENABLE_REFLECT=ON"
    "-DSPIRV_CROSS_ENABLE_UTIL=ON"
)

cmake "${cmake_args_release[@]}"

echo "Building SPIRV-Cross (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release --parallel "$(nproc)"

echo "SPIRV-Cross built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
LIB_DIR="$ROOT_DIR/Engine/Vendor/SPIRV-Cross/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/SPIRV-Cross/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Copy SPIRV-Cross libraries
# On Linux with single-config generator, libraries are directly in build directory
LIBS_TO_COPY=(
    "libspirv-cross-core.a"
    "libspirv-cross-glsl.a"
    "libspirv-cross-hlsl.a"
    "libspirv-cross-msl.a"
    "libspirv-cross-cpp.a"
    "libspirv-cross-reflect.a"
    "libspirv-cross-util.a"
    "libspirv-cross-c.a"
)

# Copy Debug libraries
for lib in "${LIBS_TO_COPY[@]}"; do
    if [[ -f "$BUILD_DIR_DEBUG/$lib" ]]; then
        # Remove 'lib' prefix and '.a' extension, add '-debug' suffix
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$BUILD_DIR_DEBUG/$lib" "$LIB_DIR/${lib_name}-debug.a"
        echo "Copied $lib (Debug) to $LIB_DIR/${lib_name}-debug.a"
    else
        # Try to find it elsewhere in the build directory
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
        # Remove 'lib' prefix and '.a' extension, add '-release' suffix
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$BUILD_DIR_RELEASE/$lib" "$LIB_DIR/${lib_name}-release.a"
        echo "Copied $lib (Release) to $LIB_DIR/${lib_name}-release.a"
    else
        # Try to find it elsewhere in the build directory
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

# List what we found
echo "Libraries in $LIB_DIR:"
ls -la "$LIB_DIR"/*.a 2>/dev/null || echo "No libraries found"

# Copy headers (SPIRV-Cross headers are in the root directory)
HEADER_FILES=(
    "spirv_cross.hpp"
    "spirv_cross_containers.hpp"
    "spirv_cross_error_handling.hpp"
    "spirv_glsl.hpp"
    "spirv_hlsl.hpp"
    "spirv_msl.hpp"
    "spirv_cpp.hpp"
    "spirv_reflect.hpp"
    "spirv_cross_c.h"
    "spirv_common.hpp"
    "spirv.hpp"
    "spirv.h"
    "GLSL.std.450.h"
    "spirv_cross_parsed_ir.hpp"
    "spirv_parser.hpp"
    "spirv_cross_util.hpp"
    "spirv_cfg.hpp"
)

for header in "${HEADER_FILES[@]}"; do
    if [[ -f "$SPIRV_CROSS_DIR/$header" ]]; then
        cp "$SPIRV_CROSS_DIR/$header" "$INCLUDE_DIR/"
    fi
done

# Also copy SPIRV directory if it exists (contains spirv.h for C bindings)
if [[ -d "$SPIRV_CROSS_DIR/include/spirv/unified1" ]]; then
    mkdir -p "$INCLUDE_DIR/spirv/unified1"
    cp -r "$SPIRV_CROSS_DIR/include/spirv/unified1"/* "$INCLUDE_DIR/spirv/unified1/"
    echo "Copied SPIRV unified headers"
fi

echo "SPIRV-Cross headers copied to $INCLUDE_DIR"
echo "SPIRV-Cross build complete!"