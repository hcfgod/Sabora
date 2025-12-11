#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
MSDF_ATLAS_GEN_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen"

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

# Patch msdfgen CMakeLists.txt to disable Skia
if [[ -f "$MSDF_ATLAS_GEN_DIR/msdfgen/CMakeLists.txt" ]]; then
    echo "Patching msdfgen CMakeLists.txt to disable Skia..." >&2
    sed -i '' 's/option(MSDFGEN_USE_SKIA "Build with the Skia library" ON)/option(MSDFGEN_USE_SKIA "Build with the Skia library" OFF)/' "$MSDF_ATLAS_GEN_DIR/msdfgen/CMakeLists.txt"
    sed -i '' 's/if(MSDFGEN_USE_SKIA)/if(FALSE)  # Disabled: MSDFGEN_USE_SKIA/' "$MSDF_ATLAS_GEN_DIR/msdfgen/CMakeLists.txt"
    echo "msdfgen CMakeLists.txt patched successfully!" >&2
fi

# Try to use system packages first, fall back to building from source
# Always build our own libraries for static linking consistency
# This ensures we have static libraries that match our build configuration
USE_SYSTEM_FREETYPE=false
USE_SYSTEM_PNG=false
USE_SYSTEM_ZLIB=false

# Note: We always build our own freetype to ensure static linking
# System freetype might be dynamic, which would cause linking issues
echo "Building our own Freetype library for static linking" >&2

# Check for system libpng
if pkg-config --exists libpng 2>/dev/null || [[ -f "/opt/homebrew/lib/libpng.dylib" ]] || [[ -f "/usr/local/lib/libpng.dylib" ]]; then
    USE_SYSTEM_PNG=true
    echo "Using system libpng library" >&2
fi

# Check for system zlib (macOS has it built-in)
if pkg-config --exists zlib 2>/dev/null || [[ -f "/usr/lib/libz.dylib" ]]; then
    USE_SYSTEM_ZLIB=true
    echo "Using system zlib library" >&2
fi

# Build zlib if not using system version
if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
    ZLIB_DIR="$ROOT_DIR/Engine/Vendor/zlib"
    ZLIB_BUILD_DIR_DEBUG="$ZLIB_DIR/build-debug"
    ZLIB_BUILD_DIR_RELEASE="$ZLIB_DIR/build-release"
    
    if [[ ! -f "$ZLIB_DIR/CMakeLists.txt" ]]; then
        echo "zlib not found. Cloning zlib..." >&2
        VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
        mkdir -p "$VENDOR_DIR"
        if [[ ! -d "$ZLIB_DIR" ]]; then
            cd "$VENDOR_DIR"
            git clone --depth 1 https://github.com/madler/zlib.git zlib
            cd "$ROOT_DIR"
        fi
    fi
    
    # Build zlib Debug
    if [[ ! -f "$ZLIB_BUILD_DIR_DEBUG/libz.a" ]]; then
        echo "Building zlib (Debug)..." >&2
        mkdir -p "$ZLIB_BUILD_DIR_DEBUG"
        cmake -S "$ZLIB_DIR" -B "$ZLIB_BUILD_DIR_DEBUG" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DBUILD_SHARED_LIBS=OFF
        cmake --build "$ZLIB_BUILD_DIR_DEBUG" --parallel "$(sysctl -n hw.ncpu)"
    fi
    
    # Build zlib Release
    if [[ ! -f "$ZLIB_BUILD_DIR_RELEASE/libz.a" ]]; then
        echo "Building zlib (Release)..." >&2
        mkdir -p "$ZLIB_BUILD_DIR_RELEASE"
        cmake -S "$ZLIB_DIR" -B "$ZLIB_BUILD_DIR_RELEASE" \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF
        cmake --build "$ZLIB_BUILD_DIR_RELEASE" --parallel "$(sysctl -n hw.ncpu)"
    fi
fi

# Build libpng if not using system version
if [[ "$USE_SYSTEM_PNG" == "false" ]]; then
    PNG_DIR="$ROOT_DIR/Engine/Vendor/libpng"
    PNG_BUILD_DIR_DEBUG="$PNG_DIR/build-debug"
    PNG_BUILD_DIR_RELEASE="$PNG_DIR/build-release"
    
    if [[ ! -f "$PNG_DIR/CMakeLists.txt" ]]; then
        echo "libpng not found. Cloning libpng..." >&2
        VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
        mkdir -p "$VENDOR_DIR"
        if [[ ! -d "$PNG_DIR" ]]; then
            cd "$VENDOR_DIR"
            git clone --depth 1 https://github.com/glennrp/libpng.git libpng
            cd "$ROOT_DIR"
        fi
    fi
    
    # Copy pnglibconf.h.prebuilt to pnglibconf.h if needed
    if [[ -f "$PNG_DIR/pnglibconf.h.prebuilt" && ! -f "$PNG_DIR/pnglibconf.h" ]]; then
        cp "$PNG_DIR/pnglibconf.h.prebuilt" "$PNG_DIR/pnglibconf.h"
    fi
    
    # Build libpng Debug
    if [[ ! -f "$PNG_BUILD_DIR_DEBUG/libpng.a" ]]; then
        echo "Building libpng (Debug)..." >&2
        mkdir -p "$PNG_BUILD_DIR_DEBUG"
        CMAKE_ARGS=(
            -S "$PNG_DIR"
            -B "$PNG_BUILD_DIR_DEBUG"
            -DCMAKE_BUILD_TYPE=Debug
            -DPNG_SHARED=OFF
            -DPNG_STATIC=ON
            -DPNG_TESTS=OFF
        )
        if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
            CMAKE_ARGS+=(
                -DZLIB_ROOT="$ZLIB_DIR"
                -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
                -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
                -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
            )
        fi
        cmake "${CMAKE_ARGS[@]}"
        cmake --build "$PNG_BUILD_DIR_DEBUG" --parallel "$(sysctl -n hw.ncpu)"
    fi
    
    # Build libpng Release
    if [[ ! -f "$PNG_BUILD_DIR_RELEASE/libpng.a" ]]; then
        echo "Building libpng (Release)..." >&2
        mkdir -p "$PNG_BUILD_DIR_RELEASE"
        CMAKE_ARGS=(
            -S "$PNG_DIR"
            -B "$PNG_BUILD_DIR_RELEASE"
            -DCMAKE_BUILD_TYPE=Release
            -DPNG_SHARED=OFF
            -DPNG_STATIC=ON
            -DPNG_TESTS=OFF
        )
        if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
            CMAKE_ARGS+=(
                -DZLIB_ROOT="$ZLIB_DIR"
                -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
                -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
                -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
            )
        fi
        cmake "${CMAKE_ARGS[@]}"
        cmake --build "$PNG_BUILD_DIR_RELEASE" --parallel "$(sysctl -n hw.ncpu)"
    fi
fi

# Build Freetype if not using system version
if [[ "$USE_SYSTEM_FREETYPE" == "false" ]]; then
    FREETYPE_DIR="$ROOT_DIR/Engine/Vendor/freetype"
    FREETYPE_BUILD_DIR_DEBUG="$FREETYPE_DIR/build-debug"
    FREETYPE_BUILD_DIR_RELEASE="$FREETYPE_DIR/build-release"
    
    if [[ ! -f "$FREETYPE_DIR/CMakeLists.txt" ]]; then
        echo "Freetype not found. Cloning Freetype..." >&2
        VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
        mkdir -p "$VENDOR_DIR"
        if [[ ! -d "$FREETYPE_DIR" ]]; then
            cd "$VENDOR_DIR"
            git clone --depth 1 https://github.com/freetype/freetype.git freetype
            cd "$ROOT_DIR"
        fi
    fi
    
    # Build Freetype Debug
    FREETYPE_DEBUG_LIB_FOUND=false
    if [[ -f "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" ]]; then
        FREETYPE_DEBUG_LIB_FOUND=true
    elif [[ -d "$FREETYPE_BUILD_DIR_DEBUG" ]]; then
        # Search recursively for any freetype library
        if find "$FREETYPE_BUILD_DIR_DEBUG" -name "libfreetype*.a" -type f | grep -q .; then
            FREETYPE_DEBUG_LIB_FOUND=true
        fi
    fi
    
    if [[ "$FREETYPE_DEBUG_LIB_FOUND" == "false" ]]; then
        echo "Building Freetype (Debug)..." >&2
        mkdir -p "$FREETYPE_BUILD_DIR_DEBUG"
        CMAKE_ARGS=(
            -S "$FREETYPE_DIR"
            -B "$FREETYPE_BUILD_DIR_DEBUG"
            -DCMAKE_BUILD_TYPE=Debug
            -DFT_DISABLE_BZIP2=ON
            -DFT_DISABLE_BROTLI=ON
            -DFT_DISABLE_PNG=ON
            -DFT_DISABLE_HARFBUZZ=ON
        )
        if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
            CMAKE_ARGS+=(
                -DZLIB_ROOT="$ZLIB_DIR"
                -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
                -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
                -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
            )
        else
            CMAKE_ARGS+=(-DFT_DISABLE_ZLIB=ON)
        fi
        cmake "${CMAKE_ARGS[@]}"
        if [[ $? -ne 0 ]]; then
            echo "Error: Freetype Debug CMake configuration failed!" >&2
            exit 1
        fi
        
        cmake --build "$FREETYPE_BUILD_DIR_DEBUG" --parallel "$(sysctl -n hw.ncpu)"
        if [[ $? -ne 0 ]]; then
            echo "Error: Freetype Debug build failed!" >&2
            exit 1
        fi
        
        # Verify the library was built and create expected name if needed
        if [[ ! -f "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" ]]; then
            # Search for the actual library file
            ACTUAL_LIB=$(find "$FREETYPE_BUILD_DIR_DEBUG" -name "libfreetype*.a" -type f | head -1)
            if [[ -n "$ACTUAL_LIB" && -f "$ACTUAL_LIB" ]]; then
                echo "Found freetype library at $ACTUAL_LIB, copying to expected location..." >&2
                cp "$ACTUAL_LIB" "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a"
            else
                echo "Error: Freetype Debug build completed but library not found!" >&2
                echo "Searched in: $FREETYPE_BUILD_DIR_DEBUG" >&2
                echo "Listing build directory contents:" >&2
                ls -la "$FREETYPE_BUILD_DIR_DEBUG" >&2 || true
                exit 1
            fi
        fi
        echo "Freetype Debug library verified at: $FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" >&2
    else
        echo "Freetype (Debug) already built, skipping..." >&2
    fi
    
    # Build Freetype Release
    FREETYPE_RELEASE_LIB_FOUND=false
    if [[ -f "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" ]]; then
        FREETYPE_RELEASE_LIB_FOUND=true
    elif [[ -d "$FREETYPE_BUILD_DIR_RELEASE" ]]; then
        # Search recursively for any freetype library
        if find "$FREETYPE_BUILD_DIR_RELEASE" -name "libfreetype*.a" -type f | grep -q .; then
            FREETYPE_RELEASE_LIB_FOUND=true
        fi
    fi
    
    if [[ "$FREETYPE_RELEASE_LIB_FOUND" == "false" ]]; then
        echo "Building Freetype (Release)..." >&2
        mkdir -p "$FREETYPE_BUILD_DIR_RELEASE"
        CMAKE_ARGS=(
            -S "$FREETYPE_DIR"
            -B "$FREETYPE_BUILD_DIR_RELEASE"
            -DCMAKE_BUILD_TYPE=Release
            -DFT_DISABLE_BZIP2=ON
            -DFT_DISABLE_BROTLI=ON
            -DFT_DISABLE_PNG=ON
            -DFT_DISABLE_HARFBUZZ=ON
        )
        if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
            CMAKE_ARGS+=(
                -DZLIB_ROOT="$ZLIB_DIR"
                -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
                -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
                -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
            )
        else
            CMAKE_ARGS+=(-DFT_DISABLE_ZLIB=ON)
        fi
        cmake "${CMAKE_ARGS[@]}"
        if [[ $? -ne 0 ]]; then
            echo "Error: Freetype Release CMake configuration failed!" >&2
            exit 1
        fi
        
        cmake --build "$FREETYPE_BUILD_DIR_RELEASE" --parallel "$(sysctl -n hw.ncpu)"
        if [[ $? -ne 0 ]]; then
            echo "Error: Freetype Release build failed!" >&2
            exit 1
        fi
        
        # Verify the library was built and create expected name if needed
        if [[ ! -f "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" ]]; then
            # Search for the actual library file
            ACTUAL_LIB=$(find "$FREETYPE_BUILD_DIR_RELEASE" -name "libfreetype*.a" -type f | head -1)
            if [[ -n "$ACTUAL_LIB" && -f "$ACTUAL_LIB" ]]; then
                echo "Found freetype library at $ACTUAL_LIB, copying to expected location..." >&2
                cp "$ACTUAL_LIB" "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a"
            else
                echo "Error: Freetype Release build completed but library not found!" >&2
                echo "Searched in: $FREETYPE_BUILD_DIR_RELEASE" >&2
                echo "Listing build directory contents:" >&2
                ls -la "$FREETYPE_BUILD_DIR_RELEASE" >&2 || true
                exit 1
            fi
        fi
        echo "Freetype Release library verified at: $FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" >&2
    else
        echo "Freetype (Release) already built, skipping..." >&2
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$MSDF_ATLAS_GEN_DIR/build-debug"
BUILD_DIR_RELEASE="$MSDF_ATLAS_GEN_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Create a minimal dummy vcpkg toolchain file to bypass the check
DUMMY_VCPKG_TOOLCHAIN="$MSDF_ATLAS_GEN_DIR/dummy_vcpkg.cmake"
cat > "$DUMMY_VCPKG_TOOLCHAIN" << 'EOF'
# Dummy vcpkg toolchain file to bypass vcpkg check
# This file exists only to satisfy msdf-atlas-gen's CMakeLists.txt check
set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
set(VCPKG_CRT_LINKAGE "static")
set(VCPKG_LIBRARY_LINKAGE "static")
EOF

echo "Building msdf-atlas-gen with CMake..."

# Configure and build Debug configuration
echo "Configuring msdf-atlas-gen (Debug)..."

# Set MSDFGEN_USE_SKIA as a cache variable before configuration
INITIAL_CACHE_DEBUG="$BUILD_DIR_DEBUG/initial_cache.cmake"
cat > "$INITIAL_CACHE_DEBUG" << 'EOF'
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "Build with the Skia library" FORCE)
EOF

CMAKE_ARGS_DEBUG=(
    -S "$MSDF_ATLAS_GEN_DIR"
    -B "$BUILD_DIR_DEBUG"
    -C "$INITIAL_CACHE_DEBUG"
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_CXX_STANDARD=17
    -DCMAKE_CXX_STANDARD_REQUIRED=ON
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_TOOLCHAIN_FILE="$DUMMY_VCPKG_TOOLCHAIN"
    -DVCPKG_TARGET_TRIPLET="x64-osx"
    -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF
    -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF
    -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON
    -DMSDF_ATLAS_GEN_USE_VCPKG=OFF
    -DMSDFGEN_USE_SKIA:BOOL=OFF
)

# Add dependency paths
if [[ "$USE_SYSTEM_FREETYPE" == "true" ]]; then
    if pkg-config --exists freetype2 2>/dev/null; then
        CMAKE_ARGS_DEBUG+=(
            -DFREETYPE_INCLUDE_DIRS="$(pkg-config --variable=includedir freetype2)"
        )
    else
        # Try common Homebrew paths
        if [[ -d "/opt/homebrew/include/freetype2" ]]; then
            CMAKE_ARGS_DEBUG+=(-DFREETYPE_INCLUDE_DIRS="/opt/homebrew/include/freetype2")
        elif [[ -d "/usr/local/include/freetype2" ]]; then
            CMAKE_ARGS_DEBUG+=(-DFREETYPE_INCLUDE_DIRS="/usr/local/include/freetype2")
        fi
    fi
else
    CMAKE_ARGS_DEBUG+=(
        -DFREETYPE_DIR="$FREETYPE_DIR"
        -DFREETYPE_LIBRARY_DEBUG="$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a"
        -DFREETYPE_LIBRARY_RELEASE="$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a"
        -DFREETYPE_INCLUDE_DIRS="$FREETYPE_DIR/include"
    )
fi

if [[ "$USE_SYSTEM_PNG" == "true" ]]; then
    if pkg-config --exists libpng 2>/dev/null; then
        CMAKE_ARGS_DEBUG+=(
            -DPNG_PNG_INCLUDE_DIR="$(pkg-config --variable=includedir libpng)"
        )
    else
        # Try common Homebrew paths
        if [[ -d "/opt/homebrew/include/libpng16" ]]; then
            CMAKE_ARGS_DEBUG+=(-DPNG_PNG_INCLUDE_DIR="/opt/homebrew/include/libpng16")
        elif [[ -d "/usr/local/include/libpng16" ]]; then
            CMAKE_ARGS_DEBUG+=(-DPNG_PNG_INCLUDE_DIR="/usr/local/include/libpng16")
        fi
    fi
else
    CMAKE_ARGS_DEBUG+=(
        -DPNG_DIR="$PNG_DIR"
        -DPNG_LIBRARY_DEBUG="$PNG_BUILD_DIR_DEBUG/libpng.a"
        -DPNG_LIBRARY_RELEASE="$PNG_BUILD_DIR_RELEASE/libpng.a"
        -DPNG_PNG_INCLUDE_DIR="$PNG_DIR"
    )
fi

if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
    CMAKE_ARGS_DEBUG+=(
        -DZLIB_ROOT="$ZLIB_DIR"
        -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
        -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
        -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
    )
fi

# Add CMAKE_PREFIX_PATH for dependencies
PREFIX_PATHS=()
if [[ "$USE_SYSTEM_PNG" == "false" ]]; then
    PREFIX_PATHS+=("$PNG_DIR")
fi
if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
    PREFIX_PATHS+=("$ZLIB_DIR")
fi
if [[ ${#PREFIX_PATHS[@]} -gt 0 ]]; then
    CMAKE_ARGS_DEBUG+=(-DCMAKE_PREFIX_PATH="$(IFS=';'; echo "${PREFIX_PATHS[*]}")")
fi

cmake "${CMAKE_ARGS_DEBUG[@]}"

echo "Building msdf-atlas-gen (Debug) with $(sysctl -n hw.ncpu) cores..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$(sysctl -n hw.ncpu)"

# Configure and build Release configuration
echo "Configuring msdf-atlas-gen (Release)..."

# Set MSDFGEN_USE_SKIA as a cache variable before configuration
INITIAL_CACHE_RELEASE="$BUILD_DIR_RELEASE/initial_cache.cmake"
cat > "$INITIAL_CACHE_RELEASE" << 'EOF'
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "Build with the Skia library" FORCE)
EOF

CMAKE_ARGS_RELEASE=(
    -S "$MSDF_ATLAS_GEN_DIR"
    -B "$BUILD_DIR_RELEASE"
    -C "$INITIAL_CACHE_RELEASE"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_STANDARD=17
    -DCMAKE_CXX_STANDARD_REQUIRED=ON
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_TOOLCHAIN_FILE="$DUMMY_VCPKG_TOOLCHAIN"
    -DVCPKG_TARGET_TRIPLET="x64-osx"
    -DMSDF_ATLAS_GEN_BUILD_STANDALONE=OFF
    -DMSDF_ATLAS_GEN_BUILD_SHARED_LIB=OFF
    -DMSDF_ATLAS_GEN_BUILD_STATIC_LIB=ON
    -DMSDF_ATLAS_GEN_USE_VCPKG=OFF
    -DMSDFGEN_USE_SKIA:BOOL=OFF
)

# Add dependency paths (same as Debug)
if [[ "$USE_SYSTEM_FREETYPE" == "true" ]]; then
    if pkg-config --exists freetype2 2>/dev/null; then
        CMAKE_ARGS_RELEASE+=(
            -DFREETYPE_INCLUDE_DIRS="$(pkg-config --variable=includedir freetype2)"
        )
    else
        if [[ -d "/opt/homebrew/include/freetype2" ]]; then
            CMAKE_ARGS_RELEASE+=(-DFREETYPE_INCLUDE_DIRS="/opt/homebrew/include/freetype2")
        elif [[ -d "/usr/local/include/freetype2" ]]; then
            CMAKE_ARGS_RELEASE+=(-DFREETYPE_INCLUDE_DIRS="/usr/local/include/freetype2")
        fi
    fi
else
    # Verify libraries exist before passing to CMake (should already be verified, but double-check)
    if [[ ! -f "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" ]]; then
        echo "Error: Freetype Debug library not found at expected path: $FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" >&2
        exit 1
    fi
    if [[ ! -f "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" ]]; then
        echo "Error: Freetype Release library not found at expected path: $FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" >&2
        exit 1
    fi
    CMAKE_ARGS_RELEASE+=(
        -DFREETYPE_DIR="$FREETYPE_DIR"
        -DFREETYPE_LIBRARY_DEBUG="$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a"
        -DFREETYPE_LIBRARY_RELEASE="$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a"
        -DFREETYPE_INCLUDE_DIRS="$FREETYPE_DIR/include"
    )
fi

if [[ "$USE_SYSTEM_PNG" == "true" ]]; then
    if pkg-config --exists libpng 2>/dev/null; then
        CMAKE_ARGS_RELEASE+=(
            -DPNG_PNG_INCLUDE_DIR="$(pkg-config --variable=includedir libpng)"
        )
    else
        if [[ -d "/opt/homebrew/include/libpng16" ]]; then
            CMAKE_ARGS_RELEASE+=(-DPNG_PNG_INCLUDE_DIR="/opt/homebrew/include/libpng16")
        elif [[ -d "/usr/local/include/libpng16" ]]; then
            CMAKE_ARGS_RELEASE+=(-DPNG_PNG_INCLUDE_DIR="/usr/local/include/libpng16")
        fi
    fi
else
    CMAKE_ARGS_RELEASE+=(
        -DPNG_DIR="$PNG_DIR"
        -DPNG_LIBRARY_DEBUG="$PNG_BUILD_DIR_DEBUG/libpng.a"
        -DPNG_LIBRARY_RELEASE="$PNG_BUILD_DIR_RELEASE/libpng.a"
        -DPNG_PNG_INCLUDE_DIR="$PNG_DIR"
    )
fi

if [[ "$USE_SYSTEM_ZLIB" == "false" ]]; then
    CMAKE_ARGS_RELEASE+=(
        -DZLIB_ROOT="$ZLIB_DIR"
        -DZLIB_LIBRARY_DEBUG="$ZLIB_BUILD_DIR_DEBUG/libz.a"
        -DZLIB_LIBRARY_RELEASE="$ZLIB_BUILD_DIR_RELEASE/libz.a"
        -DZLIB_INCLUDE_DIR="$ZLIB_DIR"
    )
fi

if [[ ${#PREFIX_PATHS[@]} -gt 0 ]]; then
    CMAKE_ARGS_RELEASE+=(-DCMAKE_PREFIX_PATH="$(IFS=';'; echo "${PREFIX_PATHS[*]}")")
fi

cmake "${CMAKE_ARGS_RELEASE[@]}"

echo "Building msdf-atlas-gen (Release) with $(sysctl -n hw.ncpu) cores..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$(sysctl -n hw.ncpu)"

echo "msdf-atlas-gen built successfully!"

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/msdf-atlas-gen/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Copy msdf-atlas-gen libraries
LIBS_TO_COPY=(
    "libmsdf-atlas-gen.a"
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

# Copy msdfgen-core and msdfgen-ext libraries
MSDFGEN_LIBS=("libmsdfgen-core.a" "libmsdfgen-ext.a")
for lib in "${MSDFGEN_LIBS[@]}"; do
    # Debug
    found_lib=$(find "$BUILD_DIR_DEBUG" -name "$lib" -type f | head -1)
    if [[ -n "$found_lib" && -f "$found_lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$found_lib" "$LIB_DIR/${lib_name}-debug.a"
        # Create symlink with expected name for linker (points to debug version)
        ln -sf "${lib_name}-debug.a" "$LIB_DIR/$lib"
        echo "Copied $lib (Debug) to $LIB_DIR/${lib_name}-debug.a and created symlink $lib"
    fi
    # Release
    found_lib=$(find "$BUILD_DIR_RELEASE" -name "$lib" -type f | head -1)
    if [[ -n "$found_lib" && -f "$found_lib" ]]; then
        lib_name=$(basename "$lib" .a | sed 's/^lib//')
        cp "$found_lib" "$LIB_DIR/${lib_name}-release.a"
        echo "Copied $lib (Release) to $LIB_DIR/${lib_name}-release.a"
    fi
done

# Copy Freetype libraries - we always build our own
if [[ "$USE_SYSTEM_FREETYPE" == "false" ]]; then
    # Verify Debug library exists
    if [[ ! -f "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" ]]; then
        echo "Error: Freetype Debug library not found at $FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" >&2
        echo "Freetype Debug build may have failed. Please check the build logs." >&2
        exit 1
    fi
    
    # Copy Debug library
    cp "$FREETYPE_BUILD_DIR_DEBUG/libfreetype.a" "$LIB_DIR/freetype-debug.a"
    # Create symlink with expected name for linker (points to debug version)
    ln -sf "freetype-debug.a" "$LIB_DIR/libfreetype.a"
    echo "Copied freetype (Debug) to $LIB_DIR/freetype-debug.a and created symlink libfreetype.a"
    
    # Verify Release library exists
    if [[ ! -f "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" ]]; then
        echo "Error: Freetype Release library not found at $FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" >&2
        echo "Freetype Release build may have failed. Please check the build logs." >&2
        exit 1
    fi
    
    # Copy Release library
    cp "$FREETYPE_BUILD_DIR_RELEASE/libfreetype.a" "$LIB_DIR/freetype-release.a"
    echo "Copied freetype (Release) to $LIB_DIR/freetype-release.a"
    
    # Verify symlink exists
    if [[ ! -L "$LIB_DIR/libfreetype.a" ]]; then
        echo "Error: Freetype symlink libfreetype.a not created!" >&2
        exit 1
    fi
    echo "Verified freetype symlink exists: $LIB_DIR/libfreetype.a -> $(readlink "$LIB_DIR/libfreetype.a")"
fi

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

# Generate msdfgen-config.h
MSDFGEN_CONFIG_H="$MSDF_ATLAS_GEN_DIR/msdfgen/msdfgen/msdfgen-config.h"
MSDFGEN_VCPKG_JSON="$MSDF_ATLAS_GEN_DIR/msdfgen/vcpkg.json"

if [[ ! -f "$MSDFGEN_CONFIG_H" ]]; then
    echo "Generating msdfgen-config.h..." >&2
    MSDFGEN_VERSION="1.0.0"
    if [[ -f "$MSDFGEN_VCPKG_JSON" ]]; then
        VERSION_MATCH=$(grep -o '"version"[[:space:]]*:[[:space:]]*"[^"]*"' "$MSDFGEN_VCPKG_JSON" | head -1)
        if [[ -n "$VERSION_MATCH" ]]; then
            MSDFGEN_VERSION=$(echo "$VERSION_MATCH" | sed 's/.*"version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
        fi
    fi
    VERSION_PARTS=($(echo "$MSDFGEN_VERSION" | tr '.' ' '))
    MAJOR="${VERSION_PARTS[0]:-1}"
    MINOR="${VERSION_PARTS[1]:-0}"
    REVISION="${VERSION_PARTS[2]:-0}"
    YEAR=$(date +%Y)
    
    mkdir -p "$(dirname "$MSDFGEN_CONFIG_H")"
    cat > "$MSDFGEN_CONFIG_H" << EOF
#pragma once

#define MSDFGEN_PUBLIC
#define MSDFGEN_EXT_PUBLIC

#define MSDFGEN_VERSION "$MSDFGEN_VERSION"
#define MSDFGEN_VERSION_MAJOR $MAJOR
#define MSDFGEN_VERSION_MINOR $MINOR
#define MSDFGEN_VERSION_REVISION $REVISION
#define MSDFGEN_COPYRIGHT_YEAR $YEAR

EOF
    echo "Generated msdfgen-config.h at $MSDFGEN_CONFIG_H" >&2
fi

echo "msdf-atlas-gen build complete!"
