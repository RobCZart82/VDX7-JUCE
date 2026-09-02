#!/bin/zsh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-macos-universal"

cmake -S "$ROOT" -B "$BUILD" -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build "$BUILD" --config Release --target VDX7_VST3

PLUGIN="$BUILD/VDX7_artefacts/Release/VST3/VDX7.vst3"
codesign --force --deep --sign - "$PLUGIN" || true

echo "Universal VST3: $PLUGIN"
