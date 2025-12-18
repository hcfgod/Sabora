#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
GLAD_DIR="$ROOT_DIR/Engine/Vendor/glad"
GLAD_INCLUDE_DIR="$GLAD_DIR/include"
GLAD_SOURCE_DIR="$GLAD_DIR/Source"

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

# Function to install Python
install_python() {
    echo "Python not found. Installing Python..." >&2
    
    local pkg_manager=$(detect_package_manager)
    case $pkg_manager in
        "apt")
            sudo apt-get update
            sudo apt-get install -y python3 python3-pip
            ;;
        "dnf"|"yum")
            sudo $pkg_manager install -y python3 python3-pip
            ;;
        "pacman")
            sudo pacman -S --noconfirm python python-pip
            ;;
        "zypper")
            sudo zypper install -y python3 python3-pip
            ;;
        *)
            echo "Unsupported package manager. Please install Python manually." >&2
            exit 1
            ;;
    esac
}

# Check and install Python if needed
if ! command_exists python3; then
    install_python
fi

echo "Python is available!" >&2

# Install GLAD dependencies (jinja2 is required for GLAD 2.0)
echo "Installing GLAD Python dependencies..." >&2
python3 -m pip install --upgrade pip --quiet >&2
python3 -m pip install jinja2 --quiet >&2

if [[ $? -ne 0 ]]; then
    echo "Error: Failed to install GLAD Python dependencies" >&2
    exit 1
fi

echo "GLAD Python dependencies installed successfully" >&2

# Check if GLAD directory exists
if [[ ! -d "$GLAD_DIR" ]] || [[ ! -f "$GLAD_DIR/glad/__main__.py" ]]; then
    echo "GLAD not found. Cloning GLAD..." >&2
    
    VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
    mkdir -p "$VENDOR_DIR"
    
    if [[ ! -d "$GLAD_DIR" ]]; then
        cd "$VENDOR_DIR"
        git clone --depth 1 https://github.com/Dav1dde/glad.git glad
        cd "$ROOT_DIR"
    else
        # If directory exists but generator is missing, re-clone
        if [[ ! -f "$GLAD_DIR/glad/__main__.py" ]]; then
            echo "GLAD generator not found, re-cloning..." >&2
            rm -rf "$GLAD_DIR"
            cd "$VENDOR_DIR"
            git clone --depth 1 https://github.com/Dav1dde/glad.git glad
            cd "$ROOT_DIR"
        fi
    fi
    
    if [[ ! -f "$GLAD_DIR/glad/__main__.py" ]]; then
        echo "Error: GLAD generator not found after cloning." >&2
        exit 1
    fi
fi

# Create output directories
mkdir -p "$GLAD_INCLUDE_DIR"
mkdir -p "$GLAD_SOURCE_DIR"

# Generate GLAD loader for OpenGL 4.6 Core profile
echo "Generating GLAD loader for OpenGL 4.6 Core..." >&2

OUTPUT_DIR="$GLAD_DIR/generated"

# Run GLAD generator
cd "$GLAD_DIR"
python3 -m glad --api="gl:core=4.6" --out-path="$OUTPUT_DIR" c

if [[ $? -ne 0 ]]; then
    echo "Error: GLAD generator failed" >&2
    exit 1
fi

echo "GLAD loader generated successfully" >&2

# Copy generated files to include and Source directories
GENERATED_INCLUDE="$OUTPUT_DIR/include"
GENERATED_SOURCE="$OUTPUT_DIR/src"

# Debug: List what was generated
echo "Checking generated output structure..." >&2
if [[ -d "$OUTPUT_DIR" ]]; then
    echo "Generated directory contents:" >&2
    find "$OUTPUT_DIR" -type f | head -10 | while read -r file; do
        echo "  $file" >&2
    done
fi

if [[ -d "$GENERATED_INCLUDE" ]]; then
    # Copy headers - preserve directory structure
    cp -r "$GENERATED_INCLUDE"/* "$GLAD_INCLUDE_DIR/" 2>/dev/null || {
        # If direct copy fails, try preserving structure
        find "$GENERATED_INCLUDE" -type f | while read -r file; do
            rel_path="${file#$GENERATED_INCLUDE/}"
            dest_file="$GLAD_INCLUDE_DIR/$rel_path"
            dest_dir=$(dirname "$dest_file")
            mkdir -p "$dest_dir"
            cp "$file" "$dest_file"
        done
    }
    echo "Copied GLAD headers to $GLAD_INCLUDE_DIR" >&2
else
    echo "Error: Generated include directory not found: $GENERATED_INCLUDE" >&2
    exit 1
fi

if [[ -d "$GENERATED_SOURCE" ]]; then
    # Copy source files - preserve directory structure
    cp -r "$GENERATED_SOURCE"/* "$GLAD_SOURCE_DIR/" 2>/dev/null || {
        # If direct copy fails, try preserving structure
        find "$GENERATED_SOURCE" -type f | while read -r file; do
            rel_path="${file#$GENERATED_SOURCE/}"
            dest_file="$GLAD_SOURCE_DIR/$rel_path"
            dest_dir=$(dirname "$dest_file")
            mkdir -p "$dest_dir"
            cp "$file" "$dest_file"
        done
    }
    echo "Copied GLAD source files to $GLAD_SOURCE_DIR" >&2
else
    echo "Error: Generated source directory not found: $GENERATED_SOURCE" >&2
    exit 1
fi

# Verify essential files exist - GLAD generates gl.h and gl.c (not glad.h/glad.c)
GLAD_HEADER_FOUND=false
for path in "$GLAD_INCLUDE_DIR/glad/gl.h" "$GLAD_INCLUDE_DIR/glad/glad.h" "$GLAD_INCLUDE_DIR/gl.h" "$GLAD_INCLUDE_DIR/glad.h" "$OUTPUT_DIR/include/glad/gl.h" "$OUTPUT_DIR/include/glad/glad.h"; do
    if [[ -f "$path" ]]; then
        echo "Found GLAD header at: $path" >&2
        GLAD_HEADER_FOUND=true
        break
    fi
done

KHR_HEADER_FOUND=false
for path in "$GLAD_INCLUDE_DIR/KHR/khrplatform.h" "$GLAD_INCLUDE_DIR/khrplatform.h" "$OUTPUT_DIR/include/KHR/khrplatform.h" "$OUTPUT_DIR/include/khrplatform.h"; do
    if [[ -f "$path" ]]; then
        echo "Found khrplatform.h at: $path" >&2
        KHR_HEADER_FOUND=true
        break
    fi
done

GLAD_SOURCE_FOUND=false
for path in "$GLAD_SOURCE_DIR/gl.c" "$GLAD_SOURCE_DIR/glad.c" "$GLAD_SOURCE_DIR/src/gl.c" "$GLAD_SOURCE_DIR/src/glad.c" "$OUTPUT_DIR/src/gl.c" "$OUTPUT_DIR/src/glad.c"; do
    if [[ -f "$path" ]]; then
        echo "Found GLAD source at: $path" >&2
        GLAD_SOURCE_FOUND=true
        break
    fi
done

# Search if not found in expected locations
if [[ "$GLAD_HEADER_FOUND" == "false" ]]; then
    echo "Warning: GLAD header (gl.h or glad.h) not found in expected locations. Searching..." >&2
    FOUND=$(find "$GLAD_INCLUDE_DIR" \( -name "gl.h" -o -name "glad.h" \) -type f 2>/dev/null | head -1)
    if [[ -n "$FOUND" ]]; then
        echo "Found GLAD header at: $FOUND" >&2
        GLAD_HEADER_FOUND=true
    else
        echo "Error: Required GLAD header not found: gl.h or glad.h (searched in $GLAD_INCLUDE_DIR)" >&2
        exit 1
    fi
fi

if [[ "$KHR_HEADER_FOUND" == "false" ]]; then
    echo "Warning: khrplatform.h not found in expected locations. Searching..." >&2
    FOUND=$(find "$GLAD_INCLUDE_DIR" -name "khrplatform.h" -type f 2>/dev/null | head -1)
    if [[ -n "$FOUND" ]]; then
        echo "Found khrplatform.h at: $FOUND" >&2
        KHR_HEADER_FOUND=true
    else
        echo "Error: Required GLAD file not found: khrplatform.h (searched in $GLAD_INCLUDE_DIR)" >&2
        exit 1
    fi
fi

if [[ "$GLAD_SOURCE_FOUND" == "false" ]]; then
    echo "Warning: GLAD source (gl.c or glad.c) not found in expected locations. Searching..." >&2
    FOUND=$(find "$GLAD_SOURCE_DIR" \( -name "gl.c" -o -name "glad.c" \) -type f 2>/dev/null | head -1)
    if [[ -n "$FOUND" ]]; then
        echo "Found GLAD source at: $FOUND" >&2
        GLAD_SOURCE_FOUND=true
    else
        echo "Error: Required GLAD source not found: gl.c or glad.c (searched in $GLAD_SOURCE_DIR)" >&2
        exit 1
    fi
fi

echo "GLAD build complete!" >&2
echo "GLAD headers: $GLAD_INCLUDE_DIR" >&2
echo "GLAD source: $GLAD_SOURCE_DIR" >&2
