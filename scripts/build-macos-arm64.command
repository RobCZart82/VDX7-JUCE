#!/bin/zsh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-macos-arm64"

cmake -S "$ROOT" -B "$BUILD" -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build "$BUILD" --config Release --target VDX7_VST3

PLUGIN="$BUILD/VDX7_artefacts/Release/VST3/VDX7.vst3"
if [[ ! -d "$PLUGIN" ]]; then
  echo "ERROR: VST3 bundle was not found at: $PLUGIN"
  exit 1
fi

codesign --force --deep --sign - "$PLUGIN" || true

DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$DEST"
rm -rf "$DEST/VDX7.vst3"
ditto "$PLUGIN" "$DEST/VDX7.vst3"

printf '\nBuilt and installed:\n  %s\n\n' "$DEST/VDX7.vst3"
printf 'In REAPER: Preferences > Plug-ins > VST > Re-scan.\n'
