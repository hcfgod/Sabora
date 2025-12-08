#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

"$SCRIPT_DIR/Scripts/macOS/install_premake_macos.sh"
"$SCRIPT_DIR/Scripts/macOS/install_deps_macos.sh"

exec "$SCRIPT_DIR/Scripts/macOS/build_macos.sh" "$@"