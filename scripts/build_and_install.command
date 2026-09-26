#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-mac"

echo "=== Wiltiz-Saturator: Apple Silicon build/install ==="

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This installer must be run on macOS."
  exit 1
fi

if ! xcode-select -p >/dev/null 2>&1; then
  echo "Xcode Command Line Tools are required. Install Xcode or run: xcode-select --install"
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake was not found. If you use Homebrew, install it with: brew install cmake"
  exit 1
fi

mkdir -p "$BUILD"
cmake -S "$ROOT" -B "$BUILD" -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --config Release --parallel

VST3="$(find "$BUILD" -type d -name 'Wiltiz-Saturator.vst3' -print -quit)"
AU="$(find "$BUILD" -type d -name 'Wiltiz-Saturator.component' -print -quit)"

if [[ -z "${VST3:-}" || -z "${AU:-}" ]]; then
  echo "Build completed, but plugin bundles were not found. Check the build output above."
  exit 1
fi

mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3" "$HOME/Library/Audio/Plug-Ins/Components"
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Wiltiz-Saturator.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Wiltiz-Saturator.component"
cp -R "$VST3" "$HOME/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU" "$HOME/Library/Audio/Plug-Ins/Components/"

# Local ad-hoc signing is sufficient for a plugin built and used on this Mac.
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/Wiltiz-Saturator.vst3" || true
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/Components/Wiltiz-Saturator.component" || true

killall -9 AudioComponentRegistrar 2>/dev/null || true

echo
echo "Installed successfully."
echo "VST3: $HOME/Library/Audio/Plug-Ins/VST3/Wiltiz-Saturator.vst3"
echo "AU:   $HOME/Library/Audio/Plug-Ins/Components/Wiltiz-Saturator.component"
echo
echo "Open FL Studio > Options > Manage plugins > Find installed plugins, with Verify plugins enabled."
read -n 1 -s -r -p "Press any key to close..."
