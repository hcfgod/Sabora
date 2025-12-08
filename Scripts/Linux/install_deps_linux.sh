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

# Function to install Git
install_git() {
    echo "Git not found. Installing Git..." >&2
    
    local pkg_manager=$(detect_package_manager)
    case $pkg_manager in
        "apt")
            sudo apt-get update
            sudo apt-get install -y git
            ;;
        "dnf"|"yum")
            sudo $pkg_manager install -y git
            ;;
        "pacman")
            sudo pacman -S --noconfirm git
            ;;
        "zypper")
            sudo zypper install -y git
            ;;
        *)
            echo "Unsupported package manager. Please install Git manually." >&2
            exit 1
            ;;
    esac
    
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
  "$SCRIPT_DIR/build_sdl3_linux.sh"
}

clone_or_update "https://github.com/gabime/spdlog.git"    "$VENDOR_DIR/spdlog"
clone_or_update "https://github.com/doctest/doctest.git"   "$VENDOR_DIR/doctest"
clone_or_update "https://github.com/g-truc/glm.git"        "$VENDOR_DIR/glm"
clone_or_update "https://github.com/nlohmann/json.git"     "$VENDOR_DIR/json"

# Install SDL3 with custom premake5.lua
install_sdl3 "$VENDOR_DIR"

echo "Dependencies are ready under $VENDOR_DIR"


