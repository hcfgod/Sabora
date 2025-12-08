#!/usr/bin/env bash
set -euo pipefail

VERSION="${PREMAKE_VERSION:-5.0.0-beta7}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
TOOLS_DIR="$ROOT_DIR/Tools/Premake"
EXE_PATH="$TOOLS_DIR/premake5"

mkdir -p "$TOOLS_DIR"

if [[ -f "$EXE_PATH" && "${1:-}" != "--force" ]]; then
  echo "Premake already installed at $EXE_PATH"
  exit 0
fi

URL="https://github.com/premake/premake-core/releases/download/v${VERSION}/premake-${VERSION}-macosx.tar.gz"
TMP_DIR="$(mktemp -d)"
TAR_FILE="$TMP_DIR/premake.tar.gz"

echo "Downloading Premake $VERSION from $URL ..."
if command -v curl >/dev/null 2>&1; then
  curl -L "$URL" -o "$TAR_FILE"
else
  wget -O "$TAR_FILE" "$URL"
fi

tar -xzf "$TAR_FILE" -C "$TMP_DIR"

FOUND="$(find "$TMP_DIR" -type f -name premake5 | head -n1)"
if [[ -z "$FOUND" ]]; then
  # Fallback: try "premake" without the "5"
  FOUND="$(find "$TMP_DIR" -type f -name premake | head -n1)"
fi
if [[ -z "$FOUND" ]]; then
  echo "Error: premake5 or premake not found in archive." >&2
  exit 1
fi

mv -f "$FOUND" "$EXE_PATH"
chmod +x "$EXE_PATH"
rm -rf "$TMP_DIR"

echo "Premake installed at $EXE_PATH"

