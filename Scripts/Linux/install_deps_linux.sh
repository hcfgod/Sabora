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

install_stb_image() {
  local vendor_dir="$1"
  local stb_dir="$vendor_dir/stb"

  mkdir -p "$stb_dir"
  echo "Fetching stb_image (header-only)..." >&2
  curl -L "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" -o "$stb_dir/stb_image.h"
  curl -L "https://raw.githubusercontent.com/nothings/stb/master/LICENSE" -o "$stb_dir/LICENSE"
}

install_imgui_docking() {
  local vendor_dir="$1"
  local imgui_dir="$vendor_dir/ImGui"

  rm -rf "$imgui_dir"
  echo "Cloning ImGui (docking branch)..." >&2
  git clone --branch docking --depth 1 "https://github.com/ocornut/imgui.git" "$imgui_dir"
}

write_stb_image_translation_unit() {
  local root_dir="$1"
  local stb_dir="$root_dir/Engine/Vendor/stb"
  local tu_path="$stb_dir/stb_image.cpp"

  mkdir -p "$stb_dir"
  cat > "$tu_path" <<'EOF'
// stb_image implementation translation unit
// Keep this as the single place where STB_IMAGE_IMPLEMENTATION is defined.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
EOF
  echo "Ensured stb_image translation unit at $tu_path" >&2
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
clone_or_update "https://github.com/lieff/minimp3.git"     "$VENDOR_DIR/minimp3"
clone_or_update "https://github.com/erincatto/box2d.git"   "$VENDOR_DIR/Box2D"
clone_or_update "https://github.com/jrouwe/JoltPhysics.git" "$VENDOR_DIR/Jolt"
install_stb_image "$VENDOR_DIR"
install_imgui_docking "$VENDOR_DIR"
write_stb_image_translation_unit "$ROOT_DIR"
echo "Building Box2D (static)..."
chmod +x "$SCRIPT_DIR/build_box2d_linux.sh" || true
bash "$SCRIPT_DIR/build_box2d_linux.sh"
echo "Building Jolt (static)..."
chmod +x "$SCRIPT_DIR/build_jolt_linux.sh" || true
bash "$SCRIPT_DIR/build_jolt_linux.sh"

# Install SDL3 with custom premake5.lua
install_sdl3 "$VENDOR_DIR"

# Build shaderc (includes SPIRV-Tools and SPIRV-Headers)
install_shaderc() {
  local vendor_dir="$1"
  
  echo "Building shaderc..."
  chmod +x "$SCRIPT_DIR/build_shaderc_linux.sh" || true
  bash "$SCRIPT_DIR/build_shaderc_linux.sh"
}

# Build SPIRV-Cross
install_spirv_cross() {
  local vendor_dir="$1"
  
  echo "Building SPIRV-Cross..."
  chmod +x "$SCRIPT_DIR/build_spirv_cross_linux.sh" || true
  bash "$SCRIPT_DIR/build_spirv_cross_linux.sh"
}

# Build msdf-atlas-gen
install_msdf_atlas_gen() {
  local vendor_dir="$1"
  
  echo "Building msdf-atlas-gen..."
  chmod +x "$SCRIPT_DIR/build_msdf_atlas_gen_linux.sh" || true
  bash "$SCRIPT_DIR/build_msdf_atlas_gen_linux.sh"
}

# Build OpenAL Soft
install_openal_soft() {
  local vendor_dir="$1"
  
  echo "Building OpenAL Soft..."
  chmod +x "$SCRIPT_DIR/build_openal_soft_linux.sh" || true
  bash "$SCRIPT_DIR/build_openal_soft_linux.sh"
}

# Build libsndfile
install_libsndfile() {
  local vendor_dir="$1"
  
  echo "Building libsndfile..."
  chmod +x "$SCRIPT_DIR/build_libsndfile_linux.sh" || true
  bash "$SCRIPT_DIR/build_libsndfile_linux.sh"
}

# Build libogg
install_libogg() {
  local vendor_dir="$1"
  
  echo "Building libogg..."
  chmod +x "$SCRIPT_DIR/build_libogg_linux.sh" || true
  bash "$SCRIPT_DIR/build_libogg_linux.sh"
}

# Build libvorbis (depends on libogg)
install_libvorbis() {
  local vendor_dir="$1"
  
  echo "Building libvorbis..."
  chmod +x "$SCRIPT_DIR/build_libvorbis_linux.sh" || true
  bash "$SCRIPT_DIR/build_libvorbis_linux.sh"
}

# Build libFLAC (optional dependency on libogg for OGG FLAC)
install_libflac() {
  local vendor_dir="$1"
  
  echo "Building libFLAC..."
  chmod +x "$SCRIPT_DIR/build_libflac_linux.sh" || true
  bash "$SCRIPT_DIR/build_libflac_linux.sh"
}

# Build libopus (standalone, no dependencies)
install_libopus() {
  local vendor_dir="$1"
  echo "Building libopus..."
  chmod +x "$SCRIPT_DIR/build_libopus_linux.sh" || true
  bash "$SCRIPT_DIR/build_libopus_linux.sh"
}

# Build opusfile (depends on libopus + libogg)
install_opusfile() {
  local vendor_dir="$1"
  echo "Building opusfile..."
  chmod +x "$SCRIPT_DIR/build_opusfile_linux.sh" || true
  bash "$SCRIPT_DIR/build_opusfile_linux.sh"
}

# Build libopusenc (depends on libopus)
install_libopusenc() {
  local vendor_dir="$1"
  echo "Building libopusenc..."
  chmod +x "$SCRIPT_DIR/build_libopusenc_linux.sh" || true
  bash "$SCRIPT_DIR/build_libopusenc_linux.sh"
}

# Build GLAD (OpenGL loader generator)
install_glad() {
  local vendor_dir="$1"
  echo "Building GLAD..."
  chmod +x "$SCRIPT_DIR/build_glad_linux.sh" || true
  bash "$SCRIPT_DIR/build_glad_linux.sh"
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
install_libopus "$VENDOR_DIR"
install_libopusenc "$VENDOR_DIR"
install_opusfile "$VENDOR_DIR"
install_glad "$VENDOR_DIR"

echo "Dependencies are ready under $VENDOR_DIR"


