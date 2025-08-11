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

clone_or_update "https://github.com/gabime/spdlog.git"    "$VENDOR_DIR/spdlog"
clone_or_update "https://github.com/doctest/doctest.git"   "$VENDOR_DIR/doctest"
clone_or_update "https://github.com/g-truc/glm.git"        "$VENDOR_DIR/glm"
clone_or_update "https://github.com/nlohmann/json.git"     "$VENDOR_DIR/json"

echo "Dependencies are ready under $VENDOR_DIR"


