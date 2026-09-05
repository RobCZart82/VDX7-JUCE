# VDX7-JUCE v0.6.6 Pre-Beta 2

[Magyar dokumentáció](README_HU.md)

![VDX7-JUCE v0.6.6 editor preview](docs/VDX7-v0.6.6.png)

VDX7-JUCE is a six-operator FM instrument built around the VDX7 DX7 Mk I hardware-emulation core, using the portable Retromulator dx7Lib adaptation and JUCE. It is not a Dexed-based reimplementation.

**Pre-release for testing, not a finished instrument.** The interface is substantially developed, but several functions and validation tasks remain. Back up projects and export important edited banks before upgrading.

## 1. Platform and package

The prepared binary is **macOS Apple Silicon (arm64), VST3**. Run it in a native Apple Silicon host; it is not an Intel/Universal binary. AU and Standalone targets compile locally but are not the primary distribution package. Windows and Universal build scripts exist; they are not verified release binaries.

The binary is ad-hoc signed, not Developer ID signed or notarised. macOS may require approval. Do not disable system-wide security protections. macOS 11 is the build-script deployment target, not a claim that every supported OS/host combination has been tested.

## 2. Installation and first sound

1. Close the host and back up any existing VDX7 plug-in and projects.
2. Extract the VST3 ZIP and copy the complete VDX7.vst3 bundle to:
   `~/Library/Audio/Plug-Ins/VST3/`
3. In REAPER, rescan under Preferences → Plug-ins → VST, then insert VDX7 as a virtual instrument.
4. Supply your own compatible firmware using LOAD ROM, or an automatic search location below.
5. Select a bank/program on the LCD, or import a compatible .syx file with LOAD SYX.
6. Play MIDI notes or use the on-screen keyboard.

If there is no sound, check firmware status, MIDI routing, track monitoring and the OUTPUT volume. Avoid duplicate VDX7 installations in user/system plug-in folders.

## 3. Firmware and banks

**No Yamaha firmware or factory voice data is included.** Supply files you are entitled to use; project saving does not bundle the firmware.

Accepted ROM layouts:

- 16,384-byte DX7 Mk I firmware, optionally beside `dx7_factory_voices_32KB.bin`.
- 49,152-byte combined `dx7.bin`: 16 KB firmware plus 32 KB factory data.

Automatic search folders:

| System | Locations |
| --- | --- |
| macOS | `~/Library/Application Support/VDX7-JUCE/ROM/`, `~/Library/Application Support/discoDSP/Retromulator/ROM/` |
| Windows source build | `%USERPROFILE%\Documents\VDX7-JUCE\ROM\`, `%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\` |

Factory data enables ROM1A–ROM4B: eight banks of 32 programs. Without it, supply compatible voice/bank SysEx data. LOAD ROM also allows manual file selection.

## 4. Voice editing

- Six operator tabs: output level, coarse/fine tuning, detune, rate scaling, velocity sensitivity and amplitude-modulation sensitivity.
- OSC MODE horizontal switch: RATIO or FIXED, with nominal ratio/Hz below it. The value excludes detune and modulation.
- Operator envelopes: four rates and four levels per operator.
- Keyboard scaling: breakpoint, left/right depth and four curve types.
- GLOBAL: four-stage pitch envelope, algorithm 1–32, feedback, oscillator key sync, transpose and LFO controls.
- LFO: speed, delay, pitch/amplitude modulation depth, key sync, six waveforms and pitch-modulation sensitivity.
- Graphs show envelope shape; they are not calibrated time/semitone plots.

## 5. Algorithm diagram and performance controls

All 32 algorithm routings are drawn. Click an operator node to select its editor; tabs and diagram selection follow each other. Selection does not mute an operator or edit its sound. OUT identifies carriers; F0–F7 indicates feedback, not a live signal level.

The on-screen keyboard, spring-centred pitch wheel, position-holding modulation wheel and master fader are functional. Wheel ribs move with their values. Stereo meters show output levels; the core's mono signal is sent to both channels.

The footer CPU percentage is a smoothed audio-callback load estimate, not total computer CPU usage and not necessarily identical to REAPER's meter.

## 6. Utility and SysEx

UTILITY provides voice renaming (1–10 printable ASCII characters), single-voice export, bank export and operator copy/paste. Copy/paste includes all 21 operator fields. Its clipboard is local to the plug-in instance and is not stored in projects.

LOAD SYX accepts one complete DX7 single voice (163-byte VCED) or bank (4104-byte VMEM). A single voice replaces the current slot; a bank replaces the editable bank. Concatenated dumps and other instrument formats are unsupported. Imports validate message structure and checksum. Export uses device/channel nibble 0; import accepts 0–15.

## 7. Saving and automation

There are 148 host parameters: 145 voice values plus Master Volume, Pitch and Mod. Previous parameter IDs/order are preserved.

Projects store editable RAM, bank/program state, ROM path and control state. Keep the external ROM available after moving a project. Program changes synchronise editing parameters even with the editor closed.

A star beside the name indicates unexported edits. Saving the DAW project and exporting SysEx are separate operations: project saving does not clear the export marker. Single export acknowledges one voice; bank export acknowledges all slots.

Manual bank/ROM/SYX replacement warns about unexported edits. MIDI-driven bank changes do not open dialogs and can replace the bank: export important edits first. The marker is not undo history.

## 8. What changed in this interface milestone

The v0.6 series adds clickable algorithm diagrams, mechanical wheel graphics, an OUTPUT fader, integrated LCD bank/program selectors, two-position sync switches, improved keyboard contrast/red felt, full-frame envelope grids and refined spacing. In v0.6.6, knob hover is subtler, OSC MODE becomes horizontal with a combined readout, and preset navigation is tightened.

## 9. Known limitations

- PERFORMANCE and SETTINGS pages are not implemented.
- No live MIDI Out/SysEx transmission; SysEx file import/export is available.
- Sample-rate conversion currently uses linear interpolation; quality improvements remain planned.
- Voice edits reload the active program. Dense automation and held-note editing need further host testing.
- Hardware/third-party SysEx interoperability is not comprehensively verified.
- No claim of complete DX7 feature parity, calibrated envelope timing or universal host compatibility.
- Back up valuable work; this pre-beta has no production-stability guarantee.

## 10. Validation and reporting

Local Apple Silicon VST3/AU/Standalone builds and ad-hoc signature checks passed. Automated checks cover voice data, SysEx, state round trips, legacy parameter ordering, algorithm routing, switch bindings and editor bounds at three sizes. Offline rendering was finite and non-silent at 44.1/48/96 kHz with 64/128/256-sample buffers.

Earlier iterations received user REAPER testing. These checks do not establish complete v0.6.6 host certification. Please test preset recall after restart, automation, held notes, switches and resizing.

Report issues at [GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues), including version, OS, CPU architecture, host/version, sample rate/buffer, steps and expected/actual behaviour. Attach screenshots or a minimal project if useful; do not upload proprietary ROMs.

## 11. Building from source

Requirements: C++20 compiler, CMake 3.22+, Xcode/Command Line Tools on macOS. Dependency revisions are pinned; the complete corresponding-source ZIP includes JUCE and dx7Lib for offline builds. A plain Git checkout fetches these dependencies. See [source dependencies](SOURCE_DEPENDENCIES.md).

A build that does not install over your existing plug-in:

```sh
cmake -S . -B build-local -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-local --config Release --target VDX7_VST3
```

Output: `build-local/VDX7_artefacts/Release/VST3/VDX7.vst3`.

The convenience script `scripts/build-macos-arm64.command` also replaces the installed user VST3; back it up first. Universal and Windows helpers: `scripts/build-macos-universal.command`, `scripts/build-windows.bat`. The macOS GitHub Actions workflow builds Universal artifacts; a successful build is not host validation.

Tests:

```sh
cmake --build build-local --config Release --target vdx7_voice_data_tests vdx7_algorithm_tests vdx7_processor_tests
ctest --test-dir build-local -C Release --output-on-failure
./build-local/Release/vdx7_processor_tests
```

The processor runner needs a locally discoverable ROM (exit 77 if absent), opens no audio device and optionally accepts an existing absolute directory for PNG snapshots.

## 12. Licensing and release status

This release uses [GNU AGPLv3](LICENSE.txt). The wrapper and original GUI resources are AGPL-3.0-only; the DX7 core retains GPL-3.0-or-later and its original notices. JUCE is used under AGPLv3. See [NOTICE.md](NOTICE.md) for the combined-work and third-party notices.

The release provides complete corresponding source including pinned JUCE and dx7Lib, build scripts and license notices alongside the binary. This software comes without warranty. Firmware is excluded from the software license. Yamaha branding in descriptive text identifies compatibility, not endorsement; no Yamaha logo is included.

## 13. Credits and next steps

Thanks to [VDX7/chiaccona](https://github.com/chiaccona/VDX7), [Retromulator/dx7Lib](https://github.com/reales/retromulator) and [JUCE](https://github.com/juce-framework/JUCE). Only the portable DX7 core is integrated, not the complete Retromulator application.

Next priorities are wider host testing, Performance/Settings functionality, held-note editing behaviour and resampling/audio-quality evaluation.
