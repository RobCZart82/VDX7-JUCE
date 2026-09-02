# VDX7-JUCE prototype

Minimal JUCE VST3/AU/Standalone wrapper around the maintained Retromulator `dx7Lib` port of the VDX7 Yamaha DX7 Mk I hardware-emulation core.

**Status:** first buildable prototype; source-level validation performed, but the macOS VST3 bundle has not been compiled in this Linux sandbox because Xcode/macOS SDK are unavailable.

For complete Hungarian build/install/test instructions see **README_HU.md**.

## Build macOS ARM64

```bash
chmod +x scripts/build-macos-arm64.command
./scripts/build-macos-arm64.command
```

The script installs the local test build to:

```text
~/Library/Audio/Plug-Ins/VST3/VDX7.vst3
```

## ROM

No Yamaha firmware is included. The wrapper accepts either:

- 16,384-byte DX7 Mk I firmware, optionally with sibling `dx7_factory_voices_32KB.bin`; or
- 49,152-byte combined `dx7.bin` (16 KB firmware + 32 KB factory voice data).

macOS auto-search:

```text
~/Library/Application Support/VDX7-JUCE/ROM/
~/Library/Application Support/discoDSP/Retromulator/ROM/
```

Windows auto-search:

```text
%USERPROFILE%\Documents\VDX7-JUCE\ROM\
%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\
```

## Upstream

- VDX7: https://github.com/chiaccona/VDX7
- Retromulator: https://github.com/reales/retromulator
- JUCE: https://github.com/juce-framework/JUCE

See `README_HU.md` for limitations and licensing notes.
