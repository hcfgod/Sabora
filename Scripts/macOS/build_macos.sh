#!/usr/bin/env bash
set -euo pipefail

# Usage: ./Scripts/build_macos.sh [--config Debug|Release] [--platforms x64,ARM64]

CONFIG="Debug"
PLATFORMS="x64,ARM64"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config)
      CONFIG="$2"; shift 2 ;;
    --platforms)
      PLATFORMS="$2"; shift 2 ;;
    *) echo "Unknown arg: $1" >&2; exit 1 ;;
  esac
done

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

"$SCRIPT_DIR/install_premake_macos.sh" >/dev/null || true

PREMAKE="$ROOT_DIR/Tools/Premake/premake5"
if [[ ! -x "$PREMAKE" ]]; then
  echo "Premake not found at $PREMAKE" >&2
  exit 1
fi

pushd "$ROOT_DIR" >/dev/null
"$PREMAKE" gmake
popd >/dev/null

IFS=',' read -r -a PLATS <<< "$PLATFORMS"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

for plat in "${PLATS[@]}"; do
  PLAT_LOWER="$(echo "$plat" | tr '[:upper:]' '[:lower:]')"
  MAKE_CFG="$(echo "$CONFIG" | tr '[:upper:]' '[:lower:]')_${PLAT_LOWER}"
  echo "Building config=${MAKE_CFG}"
  make -C "$ROOT_DIR/Build" -j"$JOBS" config="${MAKE_CFG}"
done

echo "Build completed."