# VDX7 Mk1. — User Guide

[Back to overview](../../README.md) · [Magyar útmutató](GUIDE_HU.md)

The [published 1.0.0 release](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.0)
provides Windows x64 and macOS Universal VST3 installers/manual ZIPs. Main is
now preparing 1.0.1; sections marked 1.0.1 development describe unreleased
corrections. Actions artifacts remain development builds, not new stable releases.

## 1. Platform and package

### Pending-project protection (1.0.1 development)

If a restored project is waiting for its matching ROM, load that ROM before
import/export, USER capture, renaming, operator copy/paste, bank/program
selection or performance changes. These operations are rejected even if a
different ROM is loaded. Live SysEx/program/bank and persistent MIDI settings
are blocked too; notes, releases and transient expression remain available.
Host voice/operator parameter edits and project save/reopen remain preserved.
The recovery warning takes priority over the ordinary dirty-bank export advice.
This correction is not present in the already published 1.0.0 binaries.

CI builds **macOS Universal (arm64 + x86_64) VST3** and **Windows x64 VST3**
targets. VST3 is the planned distribution format; AU and Standalone are build
targets, not promised downloads. The owner reports macOS REAPER and Windows 10
x64 REAPER use, but this does not verify every platform/sample-rate/buffer
combination on the final RC. Physical Intel Mac acceptance is not documented.

**Signing:** the Windows VST3 has no publisher signature. The macOS Universal
VST3 receives a technical ad-hoc signature, but is not Developer ID signed or
notarised. The OS may show a warning or prevent loading. Do not disable
system-wide security protections. macOS 11 is the build-script deployment target,
not a claim that every supported OS/host combination has been tested.

## 2. Installation and first sound

1. Close the DAW and back up the existing VDX7 plug-in, projects and edited USER
   bank.
2. Download and extract the VST3 ZIP. If it contains another ZIP, extract that
   too. Copy the complete `VDX7.vst3` bundle to:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\` (or `%COMMONPROGRAMFILES%\VST3\`)
3. Avoid leaving another VDX7 copy in a second plug-in folder; the host may load
   the older copy.
4. Restart the host. In REAPER, if needed, rescan under Preferences → Plug-ins →
   VST, then insert VDX7 as a virtual instrument.
5. Supply your own legally obtained original DX7 Mk I v1.8 firmware using LOAD ROM or an automatic search location below.
6. Select a bank/program on the LCD or import a compatible `.syx` file with LOAD
   SYX. Enable MIDI monitoring and play notes.

If the OS warns about the missing publisher signature, verify that the package
came from the project's official GitHub page. Do not disable system-wide
protections; follow the OS's documented per-app approval process instead.

If there is no sound, check firmware status, MIDI routing, track monitoring and the OUTPUT volume. Avoid duplicate VDX7 installations in user/system plug-in folders.

## 3. Firmware and banks

**No Yamaha firmware or factory voice data is included.** Supply files you are entitled to use; project saving does not bundle the firmware.

**Supported firmware: original Yamaha DX7 Mk I v1.8 (IG11469). Special Edition
/ SER-7 firmware is not supported.** Its runtime behaviour, GUI controls and
project recall have not been validated for VDX7-JUCE. Successful ROM loading
does not establish supported compatibility; the loader does not enforce a
v1.8-only firmware whitelist.

To identify the supported original v1.8 firmware, compare the SHA-1 of the
16,384-byte firmware image with `715dbb8e96a4df2a7f096b368334a7654860bb26`.
This matches the [MAME DX7 ROM identification](https://github.com/mamedev/mame/blob/master/src/mame/yamaha/ymdx7.cpp#L296-L300).
The checksum identifies firmware content, not download provenance or usage rights.
For a combined image, this reference applies only to its first 16,384 bytes,
not to the complete firmware-plus-bank file. Factory banks are separate data.

Supported layouts using the v1.8 firmware:

- 16,384-byte DX7 Mk I v1.8 firmware, optionally beside `dx7_factory_voices_32KB.bin`.
- 49,152-byte combined `dx7.bin`: 16 KB v1.8 firmware plus 32 KB factory data.

The 1.0.1 development version validates all 256 factory voices, not only the
file size. A combined ROM containing invalid factory data is rejected in full;
the previously loaded instrument/project state is preserved. An invalid
separate optional 32 KB file is ignored with a warning, while the 16 KB firmware
can still load. The program does not automatically repair or clamp invalid
voice data; supply a valid bank.

Upgrade caution: an older project bound to an invalid combined ROM may remain
pending in 1.0.1. A different or manually edited ROM is not a matching-ROM
recovery guarantee. Keep the original ROM, plug-in and project backups; verify
project recall on a copy before overwriting your working project.

Automatic search folders:

| System | Locations |
| --- | --- |
| macOS | `~/Library/Application Support/VDX7-JUCE/ROM/`, `~/Library/Application Support/discoDSP/Retromulator/ROM/` |
| Windows source build | `%USERPROFILE%\Documents\VDX7-JUCE\ROM\`, `%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\` |

Factory data enables ROM1A–ROM4B: eight banks of 32 programs. Without it, supply compatible voice/bank SysEx data. LOAD ROM also allows manual file selection.

### Factory bank folder

The new development build can use separate, user-supplied factory SysEx banks
with firmware alone; a combined firmware/bank ROM is not required. Open
**SETTINGS → Bank folder**, then copy your `.syx` files directly into that folder.
Choose **Refresh banks** in SETTINGS to update an existing instance. New
instances scan the folder when firmware loads. The main GUI layout is unchanged.

- macOS: `~/Library/Application Support/VDX7-JUCE/Factory Banks/`
- Windows: `%APPDATA%\VDX7-JUCE\Factory Banks\`

One to eight banks can be present. Only available ROM1A–ROM4B slots are enabled.
Recognition uses the SHA-256 fingerprint of the complete validated 4096-byte
bank payload, not filenames or patch names. Renaming an identical file does not
change its slot. Reference fingerprints identify known dumps; they do not certify
Yamaha authorship or grant redistribution permission. No bank bytes are bundled.

Files must be complete 4104-byte DX7 VMEM bank messages with valid structure,
checksum, seven-bit values and supported parameter values. Unknown or edited
banks do not receive a factory slot: use **LOAD SYX** to load them as CUSTOM.
Single voices, concatenated messages and nested folders are not scanned.
Scanning is limited to 128 SysEx files; remove duplicates/extraneous files if
the limit warning appears. An incomplete scan does not replace the existing
catalog. Identical duplicates do not create additional slots.
Source files are never rewritten.

Refreshing replaces the available catalog using the current folder and any
valid legacy combined-ROM/companion banks, but does not replace the working
sound or erase its unexported-edit indicator. Removing a bank disables its slot
unless the legacy ROM still supplies it. If the current bank's catalog entry
disappears/changes, the current sound stays in RAM and is labelled CUSTOM.
Refreshing is blocked while a project awaits its matching firmware. SETTINGS
bank-folder buttons do not apply other unfinished settings in that dialog.

DAW projects capture the current working bank, latest edited voice/name,
selected program and a private copy of the available catalog. Reopening a
project uses that saved sound/catalog even if the local bank folder has changed
or vanished. Folder refresh never silently substitutes a different sound during
project restore. Firmware is not embedded and the matching original ROM is still
required. Old projects without a catalog property retain their legacy behavior.
Each instance takes its own snapshot; refresh other open instances explicitly.
Project storage is recall, not a licence to redistribute Yamaha data.

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

The on-screen keyboard, pitch wheel, position-holding modulation wheel and master fader are functional. The pitch wheel springs to centre after a mouse drag; keyboard adjustment intentionally retains the selected value (owner-approved policy). Wheel ribs move with their values. Stereo meters show output levels; the core's mono signal is sent to both channels.

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

Some archived banks contain operator envelope rate/level 127 or fine-frequency
100, beyond the editor's normal 0–99 range. These specific legacy values are
accepted and retained in imported voices, USER storage, project RAM and SysEx
export; you do not need to modify the downloaded files. The editor still uses
0–99: merely viewing a value does not rewrite the stored byte, while explicitly
editing that parameter replaces it with the chosen normal value. Other semantic
limits and seven-bit, size, header and checksum checks remain enforced. Complete
VMEM bank export preserves reserved bits; single-voice VCED has no fields for
those VMEM reserved bits, but retains the supported legacy parameter values.

## 7. Saving and automation

There are 148 host parameters: 145 voice values plus Master Volume, Pitch and Mod. Previous parameter IDs/order are preserved.

Projects store editable RAM, bank/program state, ROM path and control state. Keep the external ROM available after moving a project. Program changes synchronise editing parameters even with the editor closed.

A star beside the name indicates unexported edits. Saving the DAW project and exporting SysEx are separate operations: project saving does not clear the export marker. Single export acknowledges one voice; bank export acknowledges all slots.

Manual bank/ROM/SYX replacement warns about unexported edits. MIDI-driven bank changes do not open dialogs and can replace the bank: export important edits first. The marker is not undo history.

## 8. Interface and scaling

The owner approved the final 1.0 GUI appearance after local macOS REAPER VST3,
Standalone and Retina visual checks. In Settings, choose one of the fixed sizes:
50%, 75%, 100%, 125% or 150%; dragging a window corner does not freely resize
the editor. Settings also contains master tuning, MIDI-channel filtering and the
advanced MONO compatibility mode. Visual approval is not full cross-platform
release acceptance.
See the [screenshots](../../README.md) and [release checklist](../release/ROADMAP_1.0.md).

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
- Supported MIDI note range is 12–120 inclusive (C0–C9 with REAPER's default
  octave labels). Notes outside this range are filtered in both MONO
  compatibility Settings modes and are not transposed.
- No live MIDI Out/SysEx transmission; SysEx file import/export is available.
- Sample-rate conversion uses a Blackman-windowed sinc filter with reported host latency.
  The owner reports excellent, DX7-faithful RC1 sound on macOS and Windows; the
  broader rate/buffer and transport/render matrix was not run and is deferred.
- Voice edits reload the active program. Dense automation and held-note editing need further host testing.
- Hardware/third-party SysEx interoperability is not comprehensively verified.
- No claim of complete DX7 feature parity, calibrated envelope timing or universal host compatibility.
- Back up valuable work. Development builds do not imply a production-stability guarantee.

## 10. Validation and reporting

Public CI runs ROM-free regressions. The exact RC1 source passed the local
ROM-backed suite (36/36 CTests; one desktop-dialog test was excluded), and the
owner reports focused REAPER acceptance on macOS/Windows. Broader host/audio/GUI
matrix items were not run and are deferred; see the
[release checklist](../release/RELEASE_CHECKLIST_1.0_RC.md).

Report issues at [GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues)
with build/commit, OS, architecture, host version, sample rate/buffer, reproduction
steps and expected/actual results. Never upload proprietary ROMs.

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

The current CMake configuration registers ROM-free CTest tests. The
`vdx7_ci_checks` and `vdx7_all_tests` targets also compile the firmware-dependent
integration runners and isolated MONO candidate experiment without executing
them or needing a ROM. For the full local suite, configure with
`-DVDX7_ENABLE_ROM_TESTS=ON -DVDX7_TEST_ROM_FILE=/absolute/path/to/your/dx7.bin`,
then rebuild `vdx7_all_tests` and rerun CTest. Never upload the ROM. The processor
runner opens no audio device and optionally accepts an existing absolute directory
for PNG snapshots.

`VDX7_TEST_ROM_FILE` for the complete opt-in suite must be a 49,152-byte combined
v1.8 firmware-plus-factory-voices image. Direct-engine tests do not load sibling
files, and identity tests need factory-bank bytes. The shared profile fixture
checks the image before all dependent local-ROM tests. The plugin itself still
supports 16,384-byte firmware with optional `dx7_factory_voices_32KB.bin`;
that product capability is not a promise that the complete suite accepts the
same fixture layout. The private fixture is separate from public ROM-free CI.

The host-reset/ownership groups inspect the validated v1.8 firmware memory map.
They are labelled `local-rom;firmware-v1_8` and require the `vdx7_v18_profile`
CTest fixture. It verifies the firmware identity before running those groups;
an incompatible image fails the prerequisite, rather than silently passing or
being interpreted as a broken unknown-ROM fallback. Other-ROM runtime coverage
remains separate acceptance work. Select these groups with
`ctest --test-dir build-local -C Release -L firmware-v1_8 --output-on-failure`.

`vdx7_mono_boundary_characterization` documents a known native MONO pitch-zero
edge by comparing the raw emulator with the processor. The current product
filters Note 0–11 and 121–127 before either MONO mode, so this raw firmware issue
is not reachable through supported plugin MIDI. The local
`vdx7_supported_note_range_acceptance` CTest verifies both settings modes and
the 12–120 inclusive boundary. Actual REAPER boundary acceptance remains a
release check. Older reports below are historical evidence for the raw firmware
behavior and earlier test naming.
The follow-up `vdx7_mono_trace_characterization` (`--mono-trace-only`) verifies
the loaded image's relevant instructions, observes the actual failing branches,
and tests subsequent notes **without** recovery. It documents retained output
and rejected native allocation, rather than declaring them fixed. See
[instruction trace and continuation](../validation/VALIDATION_MONO_INSTRUCTION_TRACE.md).
`vdx7_mono_candidate_experiment` evaluates explicitly changed branch decisions
in a separate raw-core test machine. It checks real Note 0 playback as well as
lookup, cleanup and legato; **it is not linked into the plugin**. Its PASS cannot
close production acceptance. See [experiment and remaining work](../validation/VALIDATION_MONO_CANDIDATE.md).
The experiment includes a pitch-oracle sensitivity control: unchanged Note 0
passes; deliberate test-only Note 1 input while expecting Note 0 fails the same
audio-frequency check. `--pitch-oracle-only` runs the pair;
`--pitch-oracle-note-one-mutant` exposes the mutant's failure directly (exit 1).
This expected negative control does not invert the real plugin acceptance gate.
The complete experiment shares `VDX7MonoCorrection.h` with a ROM-free six-site
decision-policy test (`vdx7_mono_correction`). The optional correction remains
selectable in SETTINGS and persisted with project state as an advanced compatibility
option; Native firmware is the recommended default and routine users should
normally leave it unchanged. Both modes accept only Note 12–120, so the option
does not enable the excluded low octave. Raw-core tests continue to characterize
native Note 0 separately. See [engine integration scope](../validation/VALIDATION_MONO_ENGINE_OPTIN.md)
and [the product range policy](../design/MIDI_RANGE_v0.7.0.md).
The separate `vdx7_deferred_partition` (`--deferred-partition-only`) regression
checks real-processor note playback/release across small and oversized successful
blocks, plus true-delay expiry under contention and reset. Its queue-only portion
also executes in public ROM-free CI. See [Q1 validation](../validation/VALIDATION_DEFERRED_PARTITION.md).
Native MONO compatibility choices remain a separate
[design decision](../design/DESIGN_MONO_NOTE_ZERO_POLICY.md), not a fix implied by green CI.
The local `vdx7_portamento` (`vdx7_stress_tests <private-ROM> --portamento-only`)
checks accepted time intent, immediate save during recovery, restore, physical
CC5 precedence and actual native time/rate across six rate/block configurations.
It requires the validated v1.8 firmware fixture; see `docs/validation/VALIDATION_PORTAMENTO_INTENT.md`.

The ROM-free `vdx7_latest_display` and local stress runner's `--publication-only`
cover stale PERFORMANCE/tuning publication versus newer UI edits, including
same-value/ABA races. See [Q2 validation and limits](../validation/VALIDATION_PERFORMANCE_PUBLICATION.md).

## 12. Licensing and release status

This project uses [GNU AGPLv3](../../LICENSE.txt). The wrapper and original GUI resources are AGPL-3.0-only; the DX7 core retains GPL-3.0-or-later and its original notices. JUCE is used under AGPLv3. See [NOTICE.md](../../NOTICE.md) for the combined-work and third-party notices.

The v1.0.0 stable release and its matching corresponding-source archive are
published on the [GitHub Releases page](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.0).
The release includes macOS universal `.pkg` and manual ZIP, Windows x64 `.exe`
and manual ZIP, checksums, build information and the source archive. It is
VST3-only; no AU or Standalone package is included. Fixes discovered after
publication are intended for a later maintenance release and do not alter the
published v1.0.0 tag or assets. This software comes without warranty. Firmware
is excluded from the software license. Yamaha branding in descriptive text
identifies compatibility, not endorsement; no Yamaha logo is included.

## 13. Credits and next steps

Thanks to [VDX7/chiaccona](https://github.com/chiaccona/VDX7), [Retromulator/dx7Lib](https://github.com/reales/retromulator) and [JUCE](https://github.com/juce-framework/JUCE). Only the portable DX7 core is integrated, not the complete Retromulator application.

The published 1.0.0 release has passed the project's release process and owner
acceptance on Windows and macOS in REAPER. Current post-release maintenance
work is tracked separately; it does not change the published tag or installer
assets. See ROADMAP_1.0.md for the release and maintenance record.
