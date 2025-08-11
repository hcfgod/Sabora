#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

"$SCRIPT_DIR/Scripts/install_premake_linux.sh"
"$SCRIPT_DIR/Scripts/install_deps_linux.sh"

exec "$SCRIPT_DIR/Scripts/build_linux.sh" "$@"