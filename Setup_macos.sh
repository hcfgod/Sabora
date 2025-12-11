#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

# Fix execute permissions for all shell scripts first
# This handles the case where files were created/committed on Windows
# and checked out on macOS (Git on Windows doesn't preserve execute bits)
if [[ -f "$SCRIPT_DIR/Scripts/macOS/fix_script_permissions.sh" ]]; then
    # Use bash to run the script even if it doesn't have execute permissions yet
    bash "$SCRIPT_DIR/Scripts/macOS/fix_script_permissions.sh"
fi

"$SCRIPT_DIR/Scripts/macOS/install_premake_macos.sh"
"$SCRIPT_DIR/Scripts/macOS/install_deps_macos.sh"

exec "$SCRIPT_DIR/Scripts/macOS/build_macos.sh" "$@"