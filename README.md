# Wiltiz-Saturator 1.0

An original 3-band saturation effect for macOS, designed for FL Studio and other AU/VST3 hosts.

## Important
Wiltiz-Saturator is an original plugin. It does not contain FabFilter source code, proprietary DSP, branding, presets, graphics, or copied UI assets.

## Features
- 3-band complementary split: Low / Mid / High
- Global Drive: -6 to +30 dB
- Per-band trim/drive: -12 to +24 dB
- Adjustable low/mid and mid/high crossovers
- Six saturation curves: Warm, Tape, Tube, Soft Clip, Hard Clip, Fold
- Wet/Dry Mix
- Output trim
- HQ 2x mode (lightweight midpoint oversampling approximation)
- Input/output peak meters
- Dark/black interface
- State/preset recall through the DAW
- VST3 and Audio Unit targets
- Native Apple Silicon build when compiled on an M1/M2/M3/M4 Mac

## Build requirements on Mac
- macOS 12 or newer
- Xcode + Command Line Tools
- CMake 3.22 or newer
- Internet connection on first build (CMake fetches JUCE 9.0.2)

## Fast build + install
Double-click `scripts/build_and_install.command` in Finder, or run it from Terminal.

The script builds a Release arm64 version and copies:
- `Wiltiz-Saturator.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
- `Wiltiz-Saturator.component` to `~/Library/Audio/Plug-Ins/Components/`

Then open FL Studio > Options > Manage plugins > Find installed plugins / Verify plugins.

## Notes
This package contains source code and a build/install script. A signed/notarized macOS binary cannot be produced from a non-macOS build machine. The script creates the native plugin directly on your Mac.
