#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
OPUSFILE_DIR="$ROOT_DIR/Engine/Vendor/opusfile"
LIBOPUS_DIR="$ROOT_DIR/Engine/Vendor/libopus"
LIBOGG_DIR="$ROOT_DIR/Engine/Vendor/libogg"

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
LIBOGG_LIB_DIR="$LIBOGG_DIR/lib"

if [[ ! -f "$LIBOPUS_LIB_DIR/opus-debug.a" ]] && [[ ! -f "$LIBOPUS_LIB_DIR/opus-release.a" ]]; then
    echo "Error: libopus must be built before opusfile. Please run build_libopus_linux.sh first." >&2
    exit 1
fi

if [[ ! -f "$LIBOGG_LIB_DIR/ogg-debug.a" ]] && [[ ! -f "$LIBOGG_LIB_DIR/ogg-release.a" ]]; then
    echo "Error: libogg must be built before opusfile. Please run build_libogg_linux.sh first." >&2
    exit 1
fi

# Check if opusfile directory exists and has CMakeLists.txt
if [[ ! -f "$OPUSFILE_DIR/CMakeLists.txt" ]]; then
    echo "opusfile not found. Cloning opusfile..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$OPUSFILE_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/xiph/opusfile.git opusfile
        cd "$ROOT_DIR"
    else
        # If directory exists but headers are missing, try to update
        if [[ ! -d "$OPUSFILE_DIR/include" ]]; then
            echo "Headers not found, updating repository..." >&2
            cd "$OPUSFILE_DIR"
            git pull || echo "Warning: git pull failed, but continuing..." >&2
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$OPUSFILE_DIR/CMakeLists.txt" ]]; then
        echo "Error: opusfile CMakeLists.txt not found after cloning." >&2
        exit 1
    fi
fi

# Patch opusfile's FindOgg.cmake to work without PkgConfig
FIND_OGG_PATH="$OPUSFILE_DIR/cmake/FindOgg.cmake"
if [[ -f "$FIND_OGG_PATH" ]]; then
    if ! grep -q "OGG_INCLUDE_DIR AND OGG_LIBRARY" "$FIND_OGG_PATH"; then
        cat > "$FIND_OGG_PATH" << 'EOF'
# Skip CONFIG mode since OggConfig.cmake may not be available
# find_package(Ogg CONFIG QUIET)
if(NOT TARGET Ogg::ogg)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(Ogg REQUIRED IMPORTED_TARGET ogg)
    set_target_properties(PkgConfig::Ogg PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(Ogg::ogg ALIAS PkgConfig::Ogg)
  else()
    # Fallback: use manual variables if PkgConfig is not available
    if(OGG_INCLUDE_DIR AND OGG_LIBRARY)
      if(NOT TARGET Ogg::ogg)
        add_library(Ogg::ogg UNKNOWN IMPORTED)
        set_target_properties(Ogg::ogg PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}"
          IMPORTED_LOCATION "${OGG_LIBRARY}"
        )
      endif()
      set(Ogg_FOUND TRUE)
    else()
      message(FATAL_ERROR "Ogg library not found. Please set OGG_INCLUDE_DIR and OGG_LIBRARY, or install PkgConfig.")
    endif()
  endif()
endif()
EOF
        echo "Patched opusfile FindOgg.cmake to work without PkgConfig" >&2
    fi
fi

# Patch opusfile's FindOpus.cmake to work without PkgConfig and incomplete OpusConfig.cmake
FIND_OPUS_PATH="$OPUSFILE_DIR/cmake/FindOpus.cmake"
if [[ -f "$FIND_OPUS_PATH" ]]; then
    if ! grep -q "OPUS_INCLUDE_DIR AND OPUS_LIBRARY" "$FIND_OPUS_PATH"; then
        cat > "$FIND_OPUS_PATH" << 'EOF'
# Skip CONFIG mode since OpusConfig.cmake from libopus is incomplete
# find_package(Opus CONFIG QUIET)
if(NOT TARGET Opus::opus)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(Opus REQUIRED IMPORTED_TARGET opus)
    set_target_properties(PkgConfig::Opus PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(Opus::opus ALIAS PkgConfig::Opus)
  else()
    # Fallback: use manual variables if PkgConfig is not available
    if(OPUS_INCLUDE_DIR AND OPUS_LIBRARY)
      if(NOT TARGET Opus::opus)
        add_library(Opus::opus UNKNOWN IMPORTED)
        set_target_properties(Opus::opus PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OPUS_INCLUDE_DIR}"
          IMPORTED_LOCATION "${OPUS_LIBRARY}"
        )
      endif()
      set(Opus_FOUND TRUE)
    else()
      message(FATAL_ERROR "Opus library not found. Please set OPUS_INCLUDE_DIR and OPUS_LIBRARY, or install PkgConfig.")
    endif()
  endif()
endif()
EOF
        echo "Patched opusfile FindOpus.cmake to work without PkgConfig" >&2
    fi
fi

# Create build directories for Debug and Release separately
BUILD_DIR_DEBUG="$OPUSFILE_DIR/build-debug"
BUILD_DIR_RELEASE="$OPUSFILE_DIR/build-release"
mkdir -p "$BUILD_DIR_DEBUG"
mkdir -p "$BUILD_DIR_RELEASE"

# Find library files for CMake configuration
OPUS_DEBUG_LIB="$LIBOPUS_LIB_DIR/opus-debug.a"
OPUS_RELEASE_LIB="$LIBOPUS_LIB_DIR/opus-release.a"
OGG_DEBUG_LIB="$LIBOGG_LIB_DIR/ogg-debug.a"
OGG_RELEASE_LIB="$LIBOGG_LIB_DIR/ogg-release.a"

LIBOPUS_INCLUDE_DIR="$LIBOPUS_DIR/include"
LIBOGG_INCLUDE_DIR="$LIBOGG_DIR/include"

# Get number of cores for parallel builds
NUM_CORES=$(nproc 2>/dev/null || echo 4)

echo "Building opusfile with CMake..."

# Configure and build Debug configuration
echo "Configuring opusfile (Debug)..."
cmake_args_debug=(
    "-S" "$OPUSFILE_DIR"
    "-B" "$BUILD_DIR_DEBUG"
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DOP_DISABLE_DOCS=ON"
    "-DOP_DISABLE_EXAMPLES=ON"
    "-DOP_DISABLE_TESTS=ON"
    "-DOP_DISABLE_HTTP=ON"
    "-DCMAKE_PREFIX_PATH=$LIBOGG_DIR:$LIBOPUS_DIR"
    "-DOGG_ROOT=$LIBOGG_DIR"
    "-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR"
    "-DOGG_LIBRARY=$OGG_DEBUG_LIB"
    "-DOPUS_ROOT=$LIBOPUS_DIR"
    "-DOPUS_INCLUDE_DIR=$LIBOPUS_INCLUDE_DIR"
    "-DOPUS_LIBRARY=$OPUS_DEBUG_LIB"
)

cmake "${cmake_args_debug[@]}"

echo "Building opusfile (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --parallel "$NUM_CORES"

# Configure and build Release configuration
echo "Configuring opusfile (Release)..."
cmake_args_release=(
    "-S" "$OPUSFILE_DIR"
    "-B" "$BUILD_DIR_RELEASE"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DOP_DISABLE_DOCS=ON"
    "-DOP_DISABLE_EXAMPLES=ON"
    "-DOP_DISABLE_TESTS=ON"
    "-DOP_DISABLE_HTTP=ON"
    "-DCMAKE_PREFIX_PATH=$LIBOGG_DIR:$LIBOPUS_DIR"
    "-DOGG_ROOT=$LIBOGG_DIR"
    "-DOGG_INCLUDE_DIR=$LIBOGG_INCLUDE_DIR"
    "-DOGG_LIBRARY=$OGG_RELEASE_LIB"
    "-DOPUS_ROOT=$LIBOPUS_DIR"
    "-DOPUS_INCLUDE_DIR=$LIBOPUS_INCLUDE_DIR"
    "-DOPUS_LIBRARY=$OPUS_RELEASE_LIB"
)

cmake "${cmake_args_release[@]}"

echo "Building opusfile (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --parallel "$NUM_CORES"

echo "opusfile built successfully!" >&2

# Copy the built libraries and headers
LIB_DIR="$ROOT_DIR/Engine/Vendor/opusfile/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/opusfile/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy Debug libraries
OPUSFILE_DEBUG_LIB=$(find "$BUILD_DIR_DEBUG" -name "libopusfile.a" -o -name "libopusfiled.a" | head -1)

if [[ -n "$OPUSFILE_DEBUG_LIB" && -f "$OPUSFILE_DEBUG_LIB" ]]; then
    cp "$OPUSFILE_DEBUG_LIB" "$LIB_DIR/opusfile-debug.a"
    # Create symlink with expected name for linker (points to debug version)
    # This allows the linker to find libopusfile.a when using -lopusfile
    rm -f "$LIB_DIR/libopusfile.a"
    ln -sf "opusfile-debug.a" "$LIB_DIR/libopusfile.a"
    echo "Copied opusfile (Debug) to $LIB_DIR/opusfile-debug.a" >&2
    echo "Created symlink: libopusfile.a -> opusfile-debug.a" >&2
else
    echo "Error: opusfile Debug library not found!" >&2
    exit 1
fi

# Find and copy Release libraries
OPUSFILE_RELEASE_LIB=$(find "$BUILD_DIR_RELEASE" -name "libopusfile.a" | head -1)

if [[ -n "$OPUSFILE_RELEASE_LIB" && -f "$OPUSFILE_RELEASE_LIB" ]]; then
    cp "$OPUSFILE_RELEASE_LIB" "$LIB_DIR/opusfile-release.a"
    echo "Copied opusfile (Release) to $LIB_DIR/opusfile-release.a" >&2
else
    echo "Error: opusfile Release library not found!" >&2
    exit 1
fi

# Verify symlink exists and is correct
if [[ ! -L "$LIB_DIR/libopusfile.a" ]] || [[ ! -f "$LIB_DIR/libopusfile.a" ]]; then
    echo "Error: libopusfile.a symlink is missing or broken!" >&2
    exit 1
fi

# Verify symlink points to the debug version
SYMLINK_TARGET=$(readlink -f "$LIB_DIR/libopusfile.a")
if [[ "$SYMLINK_TARGET" != "$(cd "$LIB_DIR" && pwd)/opusfile-debug.a" ]]; then
    echo "Error: libopusfile.a symlink points to wrong target!" >&2
    exit 1
fi

echo "Verified opusfile symlink is correct (points to opusfile-debug.a)" >&2

# Copy headers
HEADER_SOURCE_DIR="$OPUSFILE_DIR/include"
OPUSFILE_HEADER_DEST_DIR="$INCLUDE_DIR/opus"

# Create opus subdirectory in include
mkdir -p "$OPUSFILE_HEADER_DEST_DIR"

# Copy all header files from include to include/opus (preserving directory structure)
if [[ -d "$HEADER_SOURCE_DIR" ]]; then
    find "$HEADER_SOURCE_DIR" -name "*.h" -type f | while read -r headerFile; do
        relativePath="${headerFile#$HEADER_SOURCE_DIR/}"
        if [[ "$relativePath" == "$(basename "$headerFile")" ]]; then
            # File is directly in include, copy to opus subdirectory
            destFile="$OPUSFILE_HEADER_DEST_DIR/$(basename "$headerFile")"
        else
            destFile="$OPUSFILE_HEADER_DEST_DIR/$relativePath"
        fi
        destDir=$(dirname "$destFile")
        mkdir -p "$destDir"
        cp "$headerFile" "$destFile"
    done
    echo "Copied opusfile headers to $OPUSFILE_HEADER_DEST_DIR" >&2
else
    # Try alternative location - headers might be directly in source root
    if [[ -f "$OPUSFILE_DIR/include/opusfile.h" ]] || [[ -f "$OPUSFILE_DIR/opusfile.h" ]]; then
        find "$OPUSFILE_DIR" -maxdepth 2 -name "*.h" -type f ! -path "*/build*/*" | while read -r headerFile; do
            cp "$headerFile" "$OPUSFILE_HEADER_DEST_DIR/"
        done
        echo "Copied opusfile headers to $OPUSFILE_HEADER_DEST_DIR" >&2
    else
        echo "Warning: opusfile header directory not found at $HEADER_SOURCE_DIR" >&2
        echo "You may need to manually copy the headers or rebuild opusfile." >&2
    fi
fi

echo "opusfile build complete!" >&2
