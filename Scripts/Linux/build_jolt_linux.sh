#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VENDOR_DIR="$ROOT_DIR/Engine/Vendor"
SRC_DIR="$VENDOR_DIR/Jolt/Build"
BUILD_DIR_DEBUG="$SRC_DIR/build-debug"
BUILD_DIR_RELEASE="$SRC_DIR/build-release"
LIB_DIR="$VENDOR_DIR/Jolt/lib"

mkdir -p "$BUILD_DIR_DEBUG" "$BUILD_DIR_RELEASE" "$LIB_DIR"

COMMON_FLAGS=(
  -DJPH_BUILD_SHARED_LIBRARY=OFF
  -DJPH_BUILD_EXAMPLES=OFF
  -DJPH_BUILD_TESTS=OFF
  -DFLOATING_POINT_EXCEPTIONS_ENABLED=OFF
  -DPROFILER_IN_DEBUG_AND_RELEASE=OFF
  -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF
  -DENABLE_OBJECT_STREAM=OFF
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$LIB_DIR"
  -DCMAKE_DEBUG_POSTFIX=_debug
)

echo "Configuring Jolt (Debug)..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR_DEBUG" -DCMAKE_BUILD_TYPE=Debug "${COMMON_FLAGS[@]}"
echo "Building Jolt (Debug)..."
cmake --build "$BUILD_DIR_DEBUG" --config Debug

echo "Configuring Jolt (Release)..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR_RELEASE" -DCMAKE_BUILD_TYPE=Release "${COMMON_FLAGS[@]}"
echo "Building Jolt (Release)..."
cmake --build "$BUILD_DIR_RELEASE" --config Release

echo "Jolt built successfully. Libraries are in $LIB_DIR"
