#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LIBOPUSENC_DIR="$ROOT_DIR/Engine/Vendor/libopusenc"
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

# Check if dependencies are built
LIBOPUS_LIB_DIR="$LIBOPUS_DIR/lib"

if [[ ! -f "$LIBOPUS_LIB_DIR/opus-debug.a" ]] && [[ ! -f "$LIBOPUS_LIB_DIR/opus-release.a" ]]; then
    echo "Error: libopus must be built before libopusenc. Please run build_libopus_linux.sh first." >&2
    exit 1
fi

# Check if libopusenc directory exists and has CMakeLists.txt or configure script
HAS_CMAKE=false
HAS_AUTOTOOLS=false

if [[ -f "$LIBOPUSENC_DIR/CMakeLists.txt" ]]; then
    HAS_CMAKE=true
elif [[ -f "$LIBOPUSENC_DIR/configure" ]] || [[ -f "$LIBOPUSENC_DIR/configure.ac" ]]; then
    HAS_AUTOTOOLS=true
fi

if [[ "$HAS_CMAKE" == false ]] && [[ "$HAS_AUTOTOOLS" == false ]]; then
    echo "libopusenc not found. Cloning libopusenc..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$LIBOPUSENC_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/xiph/libopusenc.git libopusenc
        cd "$ROOT_DIR"
    else
        # If directory exists but build files are missing, try to update
        if [[ ! -f "$LIBOPUSENC_DIR/CMakeLists.txt" ]] && [[ ! -f "$LIBOPUSENC_DIR/configure" ]]; then
            echo "Build files not found, updating repository..." >&2
            cd "$LIBOPUSENC_DIR"
            git pull || echo "Warning: git pull failed, but continuing..." >&2
            cd "$ROOT_DIR"
        fi
    fi
    
    # Re-check for build system
    if [[ -f "$LIBOPUSENC_DIR/CMakeLists.txt" ]]; then
        HAS_CMAKE=true
    elif [[ -f "$LIBOPUSENC_DIR/configure" ]] || [[ -f "$LIBOPUSENC_DIR/configure.ac" ]]; then
        HAS_AUTOTOOLS=true
    fi
    
    if [[ "$HAS_CMAKE" == false ]] && [[ "$HAS_AUTOTOOLS" == false ]]; then
        echo "Error: libopusenc CMakeLists.txt or configure script not found after cloning." >&2
        exit 1
    fi
fi

# Ensure version header exists (some platforms expect win32/version.h)
VERSION_HEADER="$LIBOPUSENC_DIR/win32/version.h"
if [[ ! -f "$VERSION_HEADER" ]]; then
    mkdir -p "$(dirname "$VERSION_HEADER")"
    cat > "$VERSION_HEADER" <<'EOF'
#ifndef VERSION_H
#define VERSION_H

#define PACKAGE_VERSION "0.2.1"
#define PACKAGE_VERSION_MAJOR 0
#define PACKAGE_VERSION_MINOR 2
#define PACKAGE_VERSION_PATCH 1
#define PACKAGE_NAME "libopusenc"

#endif /* VERSION_H */
EOF
fi

# Find library files for CMake configuration
OPUS_DEBUG_LIB="$LIBOPUS_LIB_DIR/opus-debug.a"
OPUS_RELEASE_LIB="$LIBOPUS_LIB_DIR/opus-release.a"

LIBOPUS_INCLUDE_DIR="$LIBOPUS_DIR/include"

# Get number of cores for parallel builds
NUM_CORES=$(nproc 2>/dev/null || echo 4)

# Build using CMake if available
if [[ "$HAS_CMAKE" == true ]]; then
    echo "Building libopusenc with CMake..."
    
    # Create build directories for Debug and Release separately
    BUILD_DIR_DEBUG="$LIBOPUSENC_DIR/build-debug"
    BUILD_DIR_RELEASE="$LIBOPUSENC_DIR/build-release"
    mkdir -p "$BUILD_DIR_DEBUG"
    mkdir -p "$BUILD_DIR_RELEASE"
    
    # Configure and build Debug configuration
    echo "Configuring libopusenc (Debug)..."
    cmake_args_debug=(
        "-S" "$LIBOPUSENC_DIR"
        "-B" "$BUILD_DIR_DEBUG"
        "-DCMAKE_BUILD_TYPE=Debug"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DCMAKE_PREFIX_PATH=$LIBOPUS_DIR"
        "-DOPUS_ROOT=$LIBOPUS_DIR"
        "-DOPUS_INCLUDE_DIR=$LIBOPUS_INCLUDE_DIR"
        "-DOPUS_LIBRARY=$OPUS_DEBUG_LIB"
    )
    
    cmake "${cmake_args_debug[@]}"
    
    echo "Building libopusenc (Debug)..."
    cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"
    
    # Configure and build Release configuration
    echo "Configuring libopusenc (Release)..."
    cmake_args_release=(
        "-S" "$LIBOPUSENC_DIR"
        "-B" "$BUILD_DIR_RELEASE"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DCMAKE_PREFIX_PATH=$LIBOPUS_DIR"
        "-DOPUS_ROOT=$LIBOPUS_DIR"
        "-DOPUS_INCLUDE_DIR=$LIBOPUS_INCLUDE_DIR"
        "-DOPUS_LIBRARY=$OPUS_RELEASE_LIB"
    )
    
    cmake "${cmake_args_release[@]}"
    
    echo "Building libopusenc (Release)..."
    cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"
    
    echo "libopusenc built successfully!" >&2
    
    # Copy the built libraries and headers
    LIB_DIR="$ROOT_DIR/Engine/Vendor/libopusenc/lib"
    INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libopusenc/include"
    
    mkdir -p "$LIB_DIR"
    mkdir -p "$INCLUDE_DIR"
    
    # Find and copy Debug libraries
    OPUSENC_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libopusenc.a" -o -name "libopusencd.a" | head -1)
    
    if [[ -n "$OPUSENC_DEBUG_LIB" && -f "$OPUSENC_DEBUG_LIB" ]]; then
        cp "$OPUSENC_DEBUG_LIB" "$LIB_DIR/opusenc-debug.a"
        # Create symlink with expected name for linker (points to debug version)
        rm -f "$LIB_DIR/libopusenc.a"
        ln -sf "opusenc-debug.a" "$LIB_DIR/libopusenc.a"
        echo "Copied libopusenc (Debug) to $LIB_DIR/opusenc-debug.a" >&2
        echo "Created symlink: libopusenc.a -> opusenc-debug.a" >&2
    else
        echo "Error: libopusenc Debug library not found!" >&2
        exit 1
    fi
    
    # Find and copy Release libraries
    OPUSENC_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libopusenc.a" | head -1)
    
    if [[ -n "$OPUSENC_RELEASE_LIB" && -f "$OPUSENC_RELEASE_LIB" ]]; then
        cp "$OPUSENC_RELEASE_LIB" "$LIB_DIR/opusenc-release.a"
        echo "Copied libopusenc (Release) to $LIB_DIR/opusenc-release.a" >&2
    else
        echo "Error: libopusenc Release library not found!" >&2
        exit 1
    fi
    
    # Verify symlink exists and is correct
    if [[ ! -L "$LIB_DIR/libopusenc.a" ]] || [[ ! -f "$LIB_DIR/libopusenc.a" ]]; then
        echo "Error: libopusenc.a symlink is missing or broken!" >&2
        exit 1
    fi
    
    # Verify symlink points to the debug version
    SYMLINK_TARGET=$(readlink -f "$LIB_DIR/libopusenc.a")
    if [[ "$SYMLINK_TARGET" != "$(cd "$LIB_DIR" && pwd)/opusenc-debug.a" ]]; then
        echo "Error: libopusenc.a symlink points to wrong target!" >&2
        exit 1
    fi
    
    echo "Verified libopusenc symlink is correct (points to opusenc-debug.a)" >&2
    
    # Copy headers
    HEADER_SOURCE_DIR="$LIBOPUSENC_DIR/include"
    OPUSENC_HEADER_DEST_DIR="$INCLUDE_DIR/opus"
    
    # Create opus subdirectory in include
    mkdir -p "$OPUSENC_HEADER_DEST_DIR"
    
    # Copy all header files from include to include/opus (preserving directory structure)
    if [[ -d "$HEADER_SOURCE_DIR" ]]; then
        find "$HEADER_SOURCE_DIR" -name "*.h" -type f | while read -r headerFile; do
            relativePath="${headerFile#$HEADER_SOURCE_DIR/}"
            if [[ "$relativePath" == "$(basename "$headerFile")" ]]; then
                # File is directly in include, copy to opus subdirectory
                destFile="$OPUSENC_HEADER_DEST_DIR/$(basename "$headerFile")"
            else
                destFile="$OPUSENC_HEADER_DEST_DIR/$relativePath"
            fi
            destDir=$(dirname "$destFile")
            mkdir -p "$destDir"
            cp "$headerFile" "$destFile"
        done
        echo "Copied libopusenc headers to $OPUSENC_HEADER_DEST_DIR" >&2
    else
        # Try alternative location - headers might be directly in source root
        if [[ -f "$LIBOPUSENC_DIR/include/opusenc.h" ]] || [[ -f "$LIBOPUSENC_DIR/opusenc.h" ]]; then
            find "$LIBOPUSENC_DIR" -maxdepth 2 -name "*.h" -type f ! -path "*/build*/*" | while read -r headerFile; do
                cp "$headerFile" "$OPUSENC_HEADER_DEST_DIR/"
            done
            echo "Copied libopusenc headers to $OPUSENC_HEADER_DEST_DIR" >&2
        else
            echo "Warning: libopusenc header directory not found at $HEADER_SOURCE_DIR" >&2
            echo "You may need to manually copy the headers or rebuild libopusenc." >&2
        fi
    fi
else
    # Build using Autotools
    echo "Building libopusenc with Autotools..."
    
    # Install autotools dependencies if needed
    if ! command_exists autoconf || ! command_exists automake || ! command_exists libtool; then
        echo "Installing autotools dependencies..." >&2
        if command_exists apt-get; then
            sudo apt-get update
            sudo apt-get install -y autoconf automake libtool
        elif command_exists dnf; then
            sudo dnf install -y autoconf automake libtool
        elif command_exists yum; then
            sudo yum install -y autoconf automake libtool
        elif command_exists pacman; then
            sudo pacman -S --noconfirm autoconf automake libtool
        elif command_exists zypper; then
            sudo zypper install -y autoconf automake libtool
        else
            echo "Please install autoconf, automake, and libtool manually." >&2
            exit 1
        fi
    fi
    
    # Create build directories for Debug and Release separately
    BUILD_DIR_DEBUG="$LIBOPUSENC_DIR/build-debug"
    BUILD_DIR_RELEASE="$LIBOPUSENC_DIR/build-release"
    mkdir -p "$BUILD_DIR_DEBUG"
    mkdir -p "$BUILD_DIR_RELEASE"
    
    # Preserve existing PKG_CONFIG_PATH if set (avoid unbound variable under `set -u`)
    PKG_CONFIG_PATH_SAFE="${PKG_CONFIG_PATH:-}"

    # Build Debug configuration
    echo "Configuring libopusenc (Debug)..."
    cd "$LIBOPUSENC_DIR"
    if [[ ! -f "configure" ]]; then
        ./autogen.sh || autoreconf -fiv
    fi
    
    cd "$BUILD_DIR_DEBUG"
    "$LIBOPUSENC_DIR/configure" \
        --prefix="$BUILD_DIR_DEBUG/install" \
        --enable-static \
        --disable-shared \
        --disable-doc \
        --disable-examples \
        CFLAGS="-g -O0" \
        CPPFLAGS="-I$LIBOPUS_INCLUDE_DIR" \
        LDFLAGS="-L$LIBOPUS_LIB_DIR" \
        PKG_CONFIG_PATH="$LIBOPUS_DIR/lib/pkgconfig:$PKG_CONFIG_PATH_SAFE"
    
    echo "Building libopusenc (Debug)..."
    make -j"$NUM_CORES"
    
    # Build Release configuration
    echo "Configuring libopusenc (Release)..."
    cd "$BUILD_DIR_RELEASE"
    "$LIBOPUSENC_DIR/configure" \
        --prefix="$BUILD_DIR_RELEASE/install" \
        --enable-static \
        --disable-shared \
        --disable-doc \
        --disable-examples \
        CFLAGS="-O3" \
        CPPFLAGS="-I$LIBOPUS_INCLUDE_DIR" \
        LDFLAGS="-L$LIBOPUS_LIB_DIR" \
        PKG_CONFIG_PATH="$LIBOPUS_DIR/lib/pkgconfig:$PKG_CONFIG_PATH_SAFE"
    
    echo "Building libopusenc (Release)..."
    make -j"$NUM_CORES"
    
    echo "libopusenc built successfully!" >&2
    
    # Copy the built libraries and headers
    LIB_DIR="$ROOT_DIR/Engine/Vendor/libopusenc/lib"
    INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/libopusenc/include"
    
    mkdir -p "$LIB_DIR"
    mkdir -p "$INCLUDE_DIR"
    
    # Find and copy Debug libraries
    OPUSENC_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libopusenc.a" | head -1)
    
    if [[ -n "$OPUSENC_DEBUG_LIB" && -f "$OPUSENC_DEBUG_LIB" ]]; then
        cp "$OPUSENC_DEBUG_LIB" "$LIB_DIR/opusenc-debug.a"
        rm -f "$LIB_DIR/libopusenc.a"
        ln -sf "opusenc-debug.a" "$LIB_DIR/libopusenc.a"
        echo "Copied libopusenc (Debug) to $LIB_DIR/opusenc-debug.a" >&2
        echo "Created symlink: libopusenc.a -> opusenc-debug.a" >&2
    else
        echo "Error: libopusenc Debug library not found!" >&2
        exit 1
    fi
    
    # Find and copy Release libraries
    OPUSENC_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libopusenc.a" | head -1)
    
    if [[ -n "$OPUSENC_RELEASE_LIB" && -f "$OPUSENC_RELEASE_LIB" ]]; then
        cp "$OPUSENC_RELEASE_LIB" "$LIB_DIR/opusenc-release.a"
        echo "Copied libopusenc (Release) to $LIB_DIR/opusenc-release.a" >&2
    else
        echo "Error: libopusenc Release library not found!" >&2
        exit 1
    fi
    
    # Verify symlink
    if [[ ! -L "$LIB_DIR/libopusenc.a" ]] || [[ ! -f "$LIB_DIR/libopusenc.a" ]]; then
        echo "Error: libopusenc.a symlink is missing or broken!" >&2
        exit 1
    fi
    
    echo "Verified libopusenc symlink is correct (points to opusenc-debug.a)" >&2
    
    # Copy headers
    HEADER_SOURCE_DIR="$LIBOPUSENC_DIR/include"
    OPUSENC_HEADER_DEST_DIR="$INCLUDE_DIR/opus"
    
    mkdir -p "$OPUSENC_HEADER_DEST_DIR"
    
    if [[ -d "$HEADER_SOURCE_DIR" ]]; then
        find "$HEADER_SOURCE_DIR" -name "*.h" -type f -exec cp {} "$OPUSENC_HEADER_DEST_DIR/" \;
        echo "Copied libopusenc headers to $OPUSENC_HEADER_DEST_DIR" >&2
    else
        echo "Warning: libopusenc header directory not found at $HEADER_SOURCE_DIR" >&2
    fi
fi

echo "libopusenc build complete!" >&2
