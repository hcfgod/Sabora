#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

"$SCRIPT_DIR/Scripts/install_premake_macos.sh"
"$SCRIPT_DIR/Scripts/install_deps_macos.sh"

exec "$SCRIPT_DIR/Scripts/build_macos.sh" "$@"