#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SHADERC_DIR="$ROOT_DIR/Engine/Vendor/shaderc"
BUILD_DIR="$SHADERC_DIR/build"

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

# Function to install Python (required for shaderc build)
install_python() {
    echo "Python not found. Installing Python..." >&2
    
    local pkg_manager=$(detect_package_manager)
    case $pkg_manager in
        "apt")
            sudo apt-get update
            sudo apt-get install -y python3
            ;;
        "dnf"|"yum")
            sudo $pkg_manager install -y python3
            ;;
        "pacman")
            sudo pacman -S --noconfirm python
            ;;
        "zypper")
            sudo zypper install -y python3
            ;;
        *)
            echo "Unsupported package manager. Please install Python manually." >&2
            exit 1
            ;;
    esac
}

# Check and install CMake if needed
if ! command_exists cmake; then
    install_cmake
fi

# Check and install Python if needed
if ! command_exists python && ! command_exists python3; then
    install_python
fi

# Use python3 if python is not available
if ! command_exists python && command_exists python3; then
    alias python=python3
    PYTHON_CMD="python3"
else
    PYTHON_CMD="python"
fi

echo "Required tools are available!" >&2

# Check if shaderc directory exists and has CMakeLists.txt
if [[ ! -f "$SHADERC_DIR/CMakeLists.txt" ]]; then
    echo "shaderc not found. Cloning shaderc..." >&2
    
    # Ensure Vendor directory exists
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    # Clone shaderc if it doesn't exist
    if [[ ! -d "$SHADERC_DIR" ]]; then
        cd "$VENDOR_DIR"
        # Clone shaderc (includes SPIRV-Tools, SPIRV-Headers, glslang as submodules)
        git clone https://github.com/google/shaderc.git shaderc
        
        # Sync dependencies using shaderc's script
        cd "$SHADERC_DIR"
        echo "Syncing shaderc dependencies..."
        $PYTHON_CMD utils/git-sync-deps
        
        # Patch glslang CMakeLists.txt to fix export issue with SPIRV-Tools-opt
        echo "Patching glslang CMakeLists.txt to fix export issue..."
        GLSLANG_CMAKE="$SHADERC_DIR/third_party/glslang/CMakeLists.txt"
        if [[ -f "$GLSLANG_CMAKE" ]]; then
            # Use Python to properly handle multi-line install(EXPORT) blocks
            $PYTHON_CMD -c "
import re
import sys

file_path = sys.argv[1]
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Find and comment out install(EXPORT \"glslang-targets\" blocks
# This fixes the issue where SPIRV-Tools-opt isn't in the export set
# Try to match the full install(EXPORT) statement including nested parentheses
pattern = r'(install\s*\(\s*EXPORT\s+\"glslang-targets\"[^)]*(?:\([^)]*\)[^)]*)*\))'
replacement = r'# Patched to fix SPIRV-Tools-opt export issue - install(EXPORT) disabled\n# \1'
new_content = re.sub(pattern, replacement, content, flags=re.DOTALL)

# If the above didn't match, try a simpler approach - just comment out the line
if '# Patched to fix SPIRV-Tools-opt' not in new_content:
    new_content = re.sub(r'(install\s*\(\s*EXPORT\s+\"glslang-targets\")', r'# Patched: \1', new_content)

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(new_content)
" "$GLSLANG_CMAKE"
            echo "Patched glslang CMakeLists.txt"
        fi
        cd "$ROOT_DIR"
    fi
    
    # Verify CMakeLists.txt exists now
    if [[ ! -f "$SHADERC_DIR/CMakeLists.txt" ]]; then
        echo "Error: shaderc CMakeLists.txt not found after cloning. shaderc directory: $SHADERC_DIR" >&2
        exit 1
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Building shaderc with CMake..."

# Configure shaderc with CMake
# Use CMAKE_SKIP_INSTALL_RULES to avoid glslang export issues with SPIRV-Tools
cmake_args=(
    "-S" "$SHADERC_DIR"
    "-B" "$BUILD_DIR"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    "-DCMAKE_SKIP_INSTALL_RULES=ON"
    "-DSHADERC_SKIP_TESTS=ON"
    "-DSHADERC_SKIP_EXAMPLES=ON"
    "-DSHADERC_SKIP_INSTALL=ON"
    "-DSHADERC_SKIP_COPYRIGHT_CHECK=ON"
    "-DSPIRV_SKIP_TESTS=ON"
    "-DSPIRV_SKIP_EXECUTABLES=OFF"
    "-DENABLE_GLSLANG_BINARIES=OFF"
    "-DBUILD_SHARED_LIBS=OFF"
)

echo "Configuring shaderc..."
cmake "${cmake_args[@]}"

# Build shaderc
echo "Building shaderc..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(nproc)"

echo "shaderc built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
LIB_DIR="$ROOT_DIR/Engine/Vendor/shaderc/lib"
INCLUDE_DIR="$ROOT_DIR/Engine/Vendor/shaderc/include"

mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

# Find and copy shaderc library
# The combined library is what we want for static linking
SHADERC_LIB=""
if [[ -f "$BUILD_DIR/libshaderc/libshaderc_combined.a" ]]; then
    SHADERC_LIB="$BUILD_DIR/libshaderc/libshaderc_combined.a"
elif [[ -f "$BUILD_DIR/libshaderc/libshaderc.a" ]]; then
    SHADERC_LIB="$BUILD_DIR/libshaderc/libshaderc.a"
else
    # Search for any shaderc library
    SHADERC_LIB=$(find "$BUILD_DIR" -name "libshaderc*.a" -type f | head -1)
fi

if [[ -n "$SHADERC_LIB" && -f "$SHADERC_LIB" ]]; then
    cp "$SHADERC_LIB" "$LIB_DIR/libshaderc.a"
    echo "Copied $(basename "$SHADERC_LIB") to $LIB_DIR/libshaderc.a"
else
    echo "Warning: shaderc library not found"
    find "$BUILD_DIR" -name "*.a" -type f | head -20
fi

# Find and copy SPIRV-Tools library
SPIRV_TOOLS_LIB=""
if [[ -f "$BUILD_DIR/third_party/spirv-tools/source/libSPIRV-Tools.a" ]]; then
    SPIRV_TOOLS_LIB="$BUILD_DIR/third_party/spirv-tools/source/libSPIRV-Tools.a"
else
    SPIRV_TOOLS_LIB=$(find "$BUILD_DIR" -name "libSPIRV-Tools.a" -type f | head -1)
fi

if [[ -n "$SPIRV_TOOLS_LIB" && -f "$SPIRV_TOOLS_LIB" ]]; then
    cp "$SPIRV_TOOLS_LIB" "$LIB_DIR/libSPIRV-Tools.a"
    echo "Copied $(basename "$SPIRV_TOOLS_LIB") to $LIB_DIR/libSPIRV-Tools.a"
fi

# Find and copy SPIRV-Tools-opt library
SPIRV_TOOLS_OPT_LIB=""
if [[ -f "$BUILD_DIR/third_party/spirv-tools/source/opt/libSPIRV-Tools-opt.a" ]]; then
    SPIRV_TOOLS_OPT_LIB="$BUILD_DIR/third_party/spirv-tools/source/opt/libSPIRV-Tools-opt.a"
else
    SPIRV_TOOLS_OPT_LIB=$(find "$BUILD_DIR" -name "libSPIRV-Tools-opt.a" -type f | head -1)
fi

if [[ -n "$SPIRV_TOOLS_OPT_LIB" && -f "$SPIRV_TOOLS_OPT_LIB" ]]; then
    cp "$SPIRV_TOOLS_OPT_LIB" "$LIB_DIR/libSPIRV-Tools-opt.a"
    echo "Copied $(basename "$SPIRV_TOOLS_OPT_LIB") to $LIB_DIR/libSPIRV-Tools-opt.a"
fi

# Copy headers
SHADERC_INCLUDE_DIR="$SHADERC_DIR/libshaderc/include"
if [[ -d "$SHADERC_INCLUDE_DIR" ]]; then
    cp -r "$SHADERC_INCLUDE_DIR"/* "$INCLUDE_DIR/"
    echo "Copied shaderc headers to $INCLUDE_DIR"
fi

# Copy SPIRV-Tools headers
SPIRV_TOOLS_INCLUDE_DIR="$SHADERC_DIR/third_party/spirv-tools/include"
if [[ -d "$SPIRV_TOOLS_INCLUDE_DIR" ]]; then
    cp -r "$SPIRV_TOOLS_INCLUDE_DIR"/* "$INCLUDE_DIR/"
    echo "Copied SPIRV-Tools headers to $INCLUDE_DIR"
fi

# Copy SPIRV-Headers
SPIRV_HEADERS_INCLUDE_DIR="$SHADERC_DIR/third_party/spirv-headers/include"
if [[ -d "$SPIRV_HEADERS_INCLUDE_DIR" ]]; then
    cp -r "$SPIRV_HEADERS_INCLUDE_DIR"/* "$INCLUDE_DIR/"
    echo "Copied SPIRV-Headers to $INCLUDE_DIR"
fi

echo "shaderc build complete!"

