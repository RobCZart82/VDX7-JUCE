# VDX7-JUCE — 1.0.0 development

This branch builds **1.0.0-dev**, not the final release. See the
[current development status](DEVELOPMENT_1.0_HU_EN.md) and
[1.0 release gates](ROADMAP_1.0.md).

This documentation describes current 1.0.0-dev functionality. The image below is a historical v0.6.6 preview, not the current GUI.

[Magyar dokumentáció](README_HU.md)

![VDX7-JUCE v0.6.6 editor preview](docs/VDX7-v0.6.6.png)

VDX7-JUCE is a six-operator FM instrument built around the VDX7 DX7 Mk I hardware-emulation core, using the portable Retromulator dx7Lib adaptation and JUCE. It is not a Dexed-based reimplementation.

**Pre-release for testing, not a finished instrument.** The interface is substantially developed, but several functions and validation tasks remain. Back up projects and export important edited banks before upgrading.

## 1. Platform and package

CI and exact-commit candidates build **macOS Universal (arm64 + x86_64) VST3** and **Windows x64 VST3**. Local arm64 builds remain available. These are development artifacts, not accepted final release packages. Intel Mac and Windows host acceptance remain separate release gates. AU and Standalone are not the primary distribution formats.

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
- GLOBAL: four-stage pitch envelope, feedback, oscillator key sync, transpose and LFO controls. Algorithm 1–32 is selected with the dropdown beside its diagram, using the same host automation parameter.
- LFO: speed, delay, pitch/amplitude modulation depth, key sync, six waveforms and pitch-modulation sensitivity.
- Graphs show envelope shape; they are not calibrated time/semitone plots.

## 5. Algorithm diagram and performance controls

All 32 algorithm routings are drawn. Click an operator node to select its editor; tabs and diagram selection follow each other. Selection does not mute an operator or edit its sound. OUT identifies carriers; F0–F7 indicates feedback, not a live signal level.

The on-screen keyboard, spring-centred pitch wheel, position-holding modulation wheel and master fader are functional. Wheel ribs move with their values. Stereo meters show output levels; the core's mono signal is sent to both channels.

The footer CPU percentage is a smoothed audio-callback load estimate, not total computer CPU usage and not necessarily identical to REAPER's meter.

## 6. Utility and SysEx

UTILITY provides voice renaming (1–10 printable ASCII characters), single-voice export, bank export and operator copy/paste. Copy/paste includes all 21 operator fields. Its clipboard is local to the plug-in instance and is not stored in projects.

SAVE AS... defaults to saving a captured patch into one of 32 persistent USER bank
slots, with overwrite confirmation and conflict protection. Select USER (load copy)
on the LCD to copy that bank into the editable bank. Multiple named USER banks are
not yet supported. The same dialog offers Export Patch (.syx) and Export Bank (.syx).
Factory ROM is never overwritten. Back up the USER file as well as your projects.
Voice SysEx contains voice data, not the complete project or global performance state.
Previous/next program buttons now sit beside the LCD; the duplicated header
preset display has been removed. Program navigation wraps within the current bank.

LOAD SYX accepts one complete DX7 single voice (163-byte VCED) or bank (4104-byte VMEM). A single voice replaces the current slot; a bank replaces the editable bank. Concatenated dumps and other instrument formats are unsupported. Imports validate message structure and checksum. Export uses device/channel nibble 0; import accepts 0–15.

## 7. Saving and automation

There are 148 host parameters: 145 voice values plus Master Volume, Pitch and Mod. Previous parameter IDs/order are preserved.

Projects store editable RAM, bank/program state, ROM path and control state. Keep the external ROM available after moving a project. Program changes synchronise editing parameters even with the editor closed.

A star beside the name indicates unexported edits. Saving the DAW project and exporting SysEx are separate operations: project saving does not clear the export marker. Single export acknowledges one voice; bank export acknowledges all slots.

Manual bank/ROM/SYX replacement warns about unexported edits. MIDI-driven bank changes do not open dialogs and can replace the bank: export important edits first. The marker is not undo history.

## 8. What changed in this interface milestone

The v0.6 series adds clickable algorithm diagrams, mechanical wheel graphics, an OUTPUT fader, integrated LCD bank/program selectors, two-position sync switches, improved keyboard contrast/red felt, full-frame envelope grids and refined spacing. In v0.6.6, knob hover is subtler, OSC MODE becomes horizontal with a combined readout, and preset navigation is tightened.

## 9. Known limitations

- PERFORMANCE now exposes controller range (0–99) and pitch/amplitude/EG-bias
  assignments for mod wheel, foot (CC4), breath (CC2) and channel aftertouch.
  These global settings are recalled by the DAW project, not voice/bank SysEx;
  they are not additional host automation parameters. Changes apply during
  rendering, including held controller input. Existing 148 parameters are unchanged.
- PERFORMANCE also exposes firmware-backed POLY/MONO, pitch-bend range/step and
  portamento controls. Mode changes reset sounding voices. SETTINGS provides master
  tuning (-256 to +255 firmware units, not cents) and OMNI/channel 1–16 input filtering.
  These settings are stored in the project, not voice SysEx. Input filtering is a
  wrapper feature, not multitimbral/MPE operation.
- No live MIDI Out/SysEx transmission; SysEx file import/export is available.
- Sample-rate conversion uses a Blackman-windowed sinc filter with reported host latency.
  Final host/audio-quality acceptance remains outstanding.
- Voice edits reload the active program. Dense automation and held-note editing need further host testing.
- Hardware/third-party SysEx interoperability is not comprehensively verified.
- No claim of complete DX7 feature parity, calibrated envelope timing or universal host compatibility.
- Back up valuable work; this pre-beta has no production-stability guarantee.

## 10. Validation and reporting

Local Apple Silicon VST3/AU/Standalone builds and ad-hoc signature checks passed. Automated checks cover voice data, SysEx, state round trips, legacy parameter ordering, algorithm routing, switch bindings and editor bounds at three sizes. Offline rendering was finite and non-silent at 44.1/48/96 kHz with 64/128/256-sample buffers.

Earlier iterations received user REAPER testing. These historical checks do not establish current 1.0.0 host certification. Please test preset recall after restart, automation, held notes, switches and resizing.

Report issues at [GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues), including version, OS, CPU architecture, host/version, sample rate/buffer, steps and expected/actual behaviour. Attach screenshots or a minimal project if useful; do not upload proprietary ROMs.

## 11. Building from source

Requirements: C++20 compiler, CMake 3.22+, Xcode/Command Line Tools on macOS. Dependency revisions are pinned; the complete corresponding-source ZIP includes JUCE and dx7Lib for offline builds. A plain Git checkout fetches these dependencies. See [source dependencies](SOURCE_DEPENDENCIES.md).

A build that does not install over your existing plug-in:

```sh
cmake -S . -B build-local -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-local --config Release --target VDX7_VST3
```

Output: `build-local/VDX7_artefacts/Release/VST3/VDX7.vst3`.

The convenience script `scripts/build-macos-arm64.command` builds, ad-hoc signs and strictly verifies the bundle. It does not install or replace any installed VST3. Installation is a separate manual step. Universal and Windows helpers: `scripts/build-macos-universal.command`, `scripts/build-windows.bat`. The macOS GitHub Actions workflow builds Universal artifacts; a successful build is not host validation.

Tests:

```sh
cmake --build build-local --config Release --target vdx7_all_tests
ctest --test-dir build-local -C Release --output-on-failure
```

By default CTest runs five ROM-free tests. Both `vdx7_ci_checks` and
`vdx7_all_tests` also compile all six integration runners (processor, stability,
MIDI range, timing, stress and host reset) without executing them or needing a
ROM. For the full local suite, configure with
`-DVDX7_ENABLE_ROM_TESTS=ON -DVDX7_TEST_ROM_FILE=/absolute/path/to/your/dx7.bin`,
then rebuild `vdx7_all_tests` and rerun CTest. Never upload the ROM. The processor
runner opens no audio device and optionally accepts an existing absolute directory
for PNG snapshots.

The host-reset/ownership groups inspect the validated v1.8 firmware memory map.
They are labelled `local-rom;firmware-v1_8` and require the `vdx7_v18_profile`
CTest fixture. It verifies the firmware identity before running those groups;
an incompatible image fails the prerequisite, rather than silently passing or
being interpreted as a broken unknown-ROM fallback. Other-ROM runtime coverage
remains separate acceptance work. Select these groups with
`ctest --test-dir build-local -C Release -L firmware-v1_8 --output-on-failure`.

`vdx7_mono_boundary_characterization` documents a known native MONO pitch-zero
edge by comparing the raw emulator with the processor. Its PASS means the
documented behavior and explicit mode-cycle recovery were reproduced, **not
that the edge is fixed**. The original `vdx7_host_reset_tests /path/to/dx7.bin
--mono-note-zero-only` diagnostic remains intentionally failing and outside the
passing CTest set. See [validation scope](VALIDATION_MONO_BOUNDARY_AND_CI.md).
The follow-up `vdx7_mono_trace_characterization` (`--mono-trace-only`) verifies
the loaded image's relevant instructions, observes the actual failing branches,
and tests subsequent notes **without** recovery. It documents retained output
and rejected native allocation, rather than declaring them fixed. See
[instruction trace and continuation](VALIDATION_MONO_INSTRUCTION_TRACE.md).

## 12. Licensing and release status

This release uses [GNU AGPLv3](LICENSE.txt). The wrapper and original GUI resources are AGPL-3.0-only; the DX7 core retains GPL-3.0-or-later and its original notices. JUCE is used under AGPLv3. See [NOTICE.md](NOTICE.md) for the combined-work and third-party notices.

The release provides complete corresponding source including pinned JUCE and dx7Lib, build scripts and license notices alongside the binary. This software comes without warranty. Firmware is excluded from the software license. Yamaha branding in descriptive text identifies compatibility, not endorsement; no Yamaha logo is included.

## 13. Credits and next steps

Thanks to [VDX7/chiaccona](https://github.com/chiaccona/VDX7), [Retromulator/dx7Lib](https://github.com/reales/retromulator) and [JUCE](https://github.com/juce-framework/JUCE). Only the portable DX7 core is integrated, not the complete Retromulator application.

Next priorities are transaction/concurrency regression tests, held-note MIDI behaviour,
GUI finishing, and real-host/platform acceptance. See ROADMAP_1.0.md.
