#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VENDOR_DIR="$ROOT_DIR/Engine/Vendor"

mkdir -p "$VENDOR_DIR"

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

# Function to install Git
install_git() {
    echo "Git not found. Installing Git..." >&2
    
    if has_homebrew; then
        brew install git
    else
        # Try to install Homebrew first
        install_homebrew
        brew install git
    fi
    
    if command_exists git; then
        echo "Git installed successfully!" >&2
    else
        echo "Failed to install Git. Please install manually." >&2
        exit 1
    fi
}

# Function to install CMake
install_cmake() {
    echo "CMake not found. Installing CMake..." >&2
    
    if has_homebrew; then
        brew install cmake
    else
        # Try to install Homebrew first
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

# Check and install Git if needed
if ! command_exists git; then
    install_git
fi

# Check and install CMake if needed
if ! command_exists cmake; then
    install_cmake
fi

echo "All required tools are available!" >&2

clone_or_update() {
  local url="$1"; shift
  local target="$1"; shift
  if [[ -d "$target/.git" ]]; then
    if [[ "${FORCE:-}" == "1" ]]; then
      echo "Resetting $target ..."
      git -C "$target" fetch --depth 1 origin
      git -C "$target" reset --hard FETCH_HEAD
    else
      echo "Updating $target ..."
      git -C "$target" pull --ff-only
    fi
  else
    echo "Cloning $url -> $target ..."
    git clone --depth 1 "$url" "$target"
  fi
}

install_sdl3() {
  local vendor_dir="$1"
  local sdl_dir="$vendor_dir/SDL"
  
  # Remove existing SDL directory if it exists
  if [[ -d "$sdl_dir" ]]; then
    echo "Removing existing SDL directory..."
    rm -rf "$sdl_dir"
  fi
  
  # Clone SDL3
  echo "Cloning SDL3..."
  git clone --depth 1 "https://github.com/libsdl-org/SDL.git" "$sdl_dir"
  
  # Build SDL3 with CMake to generate build config files
  echo "Building SDL3 with CMake to generate build configuration..."
  chmod +x "$SCRIPT_DIR/build_sdl3_macos.sh" || true
  bash "$SCRIPT_DIR/build_sdl3_macos.sh"
}

clone_or_update "https://github.com/gabime/spdlog.git"    "$VENDOR_DIR/spdlog"
clone_or_update "https://github.com/doctest/doctest.git"   "$VENDOR_DIR/doctest"
clone_or_update "https://github.com/g-truc/glm.git"        "$VENDOR_DIR/glm"
clone_or_update "https://github.com/nlohmann/json.git"     "$VENDOR_DIR/json"
clone_or_update "https://github.com/lieff/minimp3.git"     "$VENDOR_DIR/minimp3"

# Install SDL3 with custom premake5.lua
install_sdl3 "$VENDOR_DIR"

# Build shaderc (includes SPIRV-Tools and SPIRV-Headers)
install_shaderc() {
  local vendor_dir="$1"
  
  echo "Building shaderc..."
  chmod +x "$SCRIPT_DIR/build_shaderc_macos.sh" || true
  bash "$SCRIPT_DIR/build_shaderc_macos.sh"
}

# Build SPIRV-Cross
install_spirv_cross() {
  local vendor_dir="$1"
  
  echo "Building SPIRV-Cross..."
  chmod +x "$SCRIPT_DIR/build_spirv_cross_macos.sh" || true
  bash "$SCRIPT_DIR/build_spirv_cross_macos.sh"
}

# Build msdf-atlas-gen
install_msdf_atlas_gen() {
  local vendor_dir="$1"
  
  echo "Building msdf-atlas-gen..."
  chmod +x "$SCRIPT_DIR/build_msdf_atlas_gen_macos.sh" || true
  bash "$SCRIPT_DIR/build_msdf_atlas_gen_macos.sh"
}

# Build OpenAL Soft
install_openal_soft() {
  local vendor_dir="$1"
  
  echo "Building OpenAL Soft..."
  chmod +x "$SCRIPT_DIR/build_openal_soft_macos.sh" || true
  bash "$SCRIPT_DIR/build_openal_soft_macos.sh"
}

# Build libsndfile
install_libsndfile() {
  local vendor_dir="$1"
  
  echo "Building libsndfile..."
  chmod +x "$SCRIPT_DIR/build_libsndfile_macos.sh" || true
  bash "$SCRIPT_DIR/build_libsndfile_macos.sh"
}

# Build libogg
install_libogg() {
  local vendor_dir="$1"
  
  echo "Building libogg..."
  chmod +x "$SCRIPT_DIR/build_libogg_macos.sh" || true
  bash "$SCRIPT_DIR/build_libogg_macos.sh"
}

# Build libvorbis (depends on libogg)
install_libvorbis() {
  local vendor_dir="$1"
  
  echo "Building libvorbis..."
  chmod +x "$SCRIPT_DIR/build_libvorbis_macos.sh" || true
  bash "$SCRIPT_DIR/build_libvorbis_macos.sh"
}

# Build libFLAC (optional dependency on libogg for OGG FLAC)
install_libflac() {
  local vendor_dir="$1"
  
  echo "Building libFLAC..."
  chmod +x "$SCRIPT_DIR/build_libflac_macos.sh" || true
  bash "$SCRIPT_DIR/build_libflac_macos.sh"
}

# Install shaderc and SPIRV-Cross
install_shaderc "$VENDOR_DIR"
install_spirv_cross "$VENDOR_DIR"

# Install new libraries
install_msdf_atlas_gen "$VENDOR_DIR"
install_openal_soft "$VENDOR_DIR"
install_libsndfile "$VENDOR_DIR"
install_libogg "$VENDOR_DIR"
install_libvorbis "$VENDOR_DIR"
install_libflac "$VENDOR_DIR"

echo "Dependencies are ready under $VENDOR_DIR"