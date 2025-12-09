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

# Function to install SDL3 system dependencies
install_sdl3_dependencies() {
    echo "Installing SDL3 system dependencies..." >&2
    
    local pkg_manager=$(detect_package_manager)
    case $pkg_manager in
        "apt")
            sudo apt-get update
            sudo apt-get install -y \
                libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
                libxi-dev libxinerama-dev libxxf86vm-dev libxss-dev libxtst-dev \
                libwayland-dev libxkbcommon-dev wayland-protocols \
                libegl1-mesa-dev libgles2-mesa-dev \
                libasound2-dev libpulse-dev libjack-dev libpipewire-0.3-dev \
                libdbus-1-dev libibus-1.0-dev \
                libudev-dev libusb-1.0-0-dev \
                libdrm-dev libgbm-dev
            ;;
        "dnf")
            sudo dnf install -y \
                libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXfixes-devel \
                libXi-devel libXinerama-devel libXxf86vm-devel libXScrnSaver-devel libXtst-devel \
                wayland-devel libxkbcommon-devel wayland-protocols-devel \
                mesa-libEGL-devel mesa-libGLES-devel \
                alsa-lib-devel pulseaudio-libs-devel jack-audio-connection-kit-devel pipewire-devel \
                dbus-devel ibus-devel \
                systemd-devel libusb1-devel \
                libdrm-devel mesa-libgbm-devel
            ;;
        "yum")
            sudo yum install -y \
                libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXfixes-devel \
                libXi-devel libXinerama-devel libXxf86vm-devel libXScrnSaver-devel libXtst-devel \
                wayland-devel libxkbcommon-devel \
                mesa-libEGL-devel mesa-libGLES-devel \
                alsa-lib-devel pulseaudio-libs-devel \
                dbus-devel ibus-devel \
                systemd-devel libusb1-devel \
                libdrm-devel
            ;;
        "pacman")
            sudo pacman -S --noconfirm \
                libx11 libxext libxrandr libxcursor libxfixes libxi libxinerama libxxf86vm libxss libxtst \
                wayland wayland-protocols libxkbcommon \
                mesa \
                alsa-lib libpulse jack2 pipewire \
                dbus \
                systemd libusb \
                libdrm
            ;;
        "zypper")
            sudo zypper install -y \
                libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXfixes-devel \
                libXi-devel libXinerama-devel libXxf86vm-devel libXss-devel libXtst-devel \
                wayland-devel libxkbcommon-devel wayland-protocols-devel \
                Mesa-libEGL-devel Mesa-libGLES-devel \
                alsa-devel libpulse-devel libjack-devel pipewire-devel \
                dbus-1-devel \
                systemd-devel libusb-1_0-devel \
                libdrm-devel
            ;;
        *)
            echo "Warning: Unsupported package manager. SDL3 dependencies may need to be installed manually." >&2
            echo "See: https://wiki.libsdl.org/SDL3/README-linux#build-dependencies" >&2
            ;;
    esac
    
    echo "SDL3 system dependencies installed!" >&2
}

# Check and install Git if needed
if ! command_exists git; then
    install_git
fi

# Check and install CMake if needed
if ! command_exists cmake; then
    install_cmake
fi

# Install SDL3 system dependencies
install_sdl3_dependencies

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
  chmod +x "$SCRIPT_DIR/build_sdl3_linux.sh" || true
  bash "$SCRIPT_DIR/build_sdl3_linux.sh"
}

clone_or_update "https://github.com/gabime/spdlog.git"    "$VENDOR_DIR/spdlog"
clone_or_update "https://github.com/doctest/doctest.git"   "$VENDOR_DIR/doctest"
clone_or_update "https://github.com/g-truc/glm.git"        "$VENDOR_DIR/glm"
clone_or_update "https://github.com/nlohmann/json.git"     "$VENDOR_DIR/json"

# Install SDL3 with custom premake5.lua
install_sdl3 "$VENDOR_DIR"

echo "Dependencies are ready under $VENDOR_DIR"


