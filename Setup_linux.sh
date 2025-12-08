#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

"$SCRIPT_DIR/Scripts/Linux/install_premake_linux.sh"
"$SCRIPT_DIR/Scripts/Linux/install_deps_linux.sh"

exec "$SCRIPT_DIR/Scripts/Linux/build_linux.sh" "$@"