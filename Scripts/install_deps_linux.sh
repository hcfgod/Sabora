#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
VENDOR_DIR="$ROOT_DIR/Engine/Vendor"

mkdir -p "$VENDOR_DIR"

if ! command -v git >/dev/null 2>&1; then
  echo "git is required to download dependencies. Please install git." >&2
  exit 1
fi

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


