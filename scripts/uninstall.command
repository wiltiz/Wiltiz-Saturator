#!/bin/bash
set -e
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Wiltiz-Saturator.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Wiltiz-Saturator.component"
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "Wiltiz-Saturator removed."
