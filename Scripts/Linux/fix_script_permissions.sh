#!/usr/bin/env bash
set -euo pipefail

# Script to fix execute permissions for all shell scripts
# This is needed because Git on Windows doesn't preserve execute permissions
# when files are checked out on Linux systems.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Fixing execute permissions for shell scripts..."

# Find and fix permissions for all shell scripts in the Scripts directory
find "$ROOT_DIR/Scripts" -type f -name "*.sh" -exec chmod +x {} \;

# Fix permissions for setup scripts in the root directory
find "$ROOT_DIR" -maxdepth 1 -type f -name "Setup_*.sh" -exec chmod +x {} \;

# Fix permissions for premake5 executable if it exists
if [[ -f "$ROOT_DIR/Tools/Premake/premake5" ]]; then
    chmod +x "$ROOT_DIR/Tools/Premake/premake5"
fi

echo "Execute permissions fixed for all shell scripts."

