#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
SRC_DIR="$VENDOR_DIR/Box2D"
BUILD_DIR_DEBUG="$SRC_DIR/build-debug"
BUILD_DIR_RELEASE="$SRC_DIR/build-release"
LIB_DIR="$VENDOR_DIR/Box2D/lib"

mkdir -p "$BUILD_DIR_DEBUG" "$BUILD_DIR_RELEASE" "$LIB_DIR"

COMMON_FLAGS=(
  -DBUILD_SHARED_LIBS=OFF
  -DBOX2D_BUILD_UNIT_TESTS=OFF
  -DBOX2D_BUILD_TESTBED=OFF
  -DBOX2D_BUILD_DOCS=OFF
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$LIB_DIR"
  -DCMAKE_ARCHIVE_OUTPUT_NAME_DEBUG=box2d-debug
  -DCMAKE_ARCHIVE_OUTPUT_NAME_RELEASE=box2d-release
)

echo "Configuring Box2D (Debug)..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR_DEBUG" -DCMAKE_BUILD_TYPE=Debug "${COMMON_FLAGS[@]}"
echo "Building Box2D (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug

echo "Configuring Box2D (Release)..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR_RELEASE" -DCMAKE_BUILD_TYPE=Release "${COMMON_FLAGS[@]}"
echo "Building Box2D (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release

echo "Box2D built successfully. Libraries are in $LIB_DIR"