# VDX7 Mk1.

**6-Operator FM Synthesizer · Hardware Emulation**

An open-source instrument built around the VDX7 DX7 Mk I hardware-emulation
core, its portable Retromulator dx7Lib adaptation and JUCE.
Original firmware, a hardware-inspired interface and hands-on voice editing.

[Magyar](README_HU.md)

> **[Stable 1.0.1 downloads](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1)**
> are available. Development builds remain separate and visibly marked.
> A legally obtained original DX7 Mk I v1.8 user-supplied ROM is required. No Yamaha firmware
> or factory voice data is included.

**1.0.0 known limitation:** when a restored project reports a mismatched/missing
ROM, load its matching ROM before import/export or bank/program/performance
changes. The pending-project protection and source-package verifier corrections
are included in 1.0.1, not retroactively installed in 1.0.0.
See the [active corrective plan](docs/development/DEVELOPMENT_PLAN.md).

![VDX7 Mk1. EDIT — operator controls, envelopes and algorithm display](docs/screenshots/vdx7-edit.png)

![VDX7 Mk1. PERFORMANCE — play mode, pitch bend, portamento and controller assignments](docs/screenshots/vdx7-performance.png)

*Actual 1.0.0-dev screenshots supplied by the project owner. The owner approved
this GUI after testing the local VST3 in REAPER and the Standalone app.
The screenshots retain their development labels; they are not a stable-release certification.*

## Download

The [1.0.1 release](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1) has **four user-download
assets**: Windows x64 EXE installer and Manual Install ZIP, plus macOS Universal
PKG installer and Manual Install ZIP. Checksums and signing limitations are in
the release description. [Complete matching source with pinned dependencies](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source)
is provided separately. See the
[four-download policy](docs/release/PUBLIC_DOWNLOADS.md).

Development VST3 builds are available from [GitHub Actions](https://github.com/RobCZart82/VDX7-JUCE/actions).
Choose a successful run for the desired branch and commit, then download its artifact:

- **Windows x64:** `VDX7-Windows-x64-VST3`
- **macOS Universal (Apple Silicon + Intel):** `VDX7-macOS-universal-VST3`

GitHub sign-in may be required to download artifacts. Use the latest successful
**main** run for the merged version; a pull-request build can contain changes
that are not yet on main. Artifacts are temporary development downloads, not
a published stable release.

[Published releases](https://github.com/RobCZart82/VDX7-JUCE/releases) are separate
from these test builds. Standalone and macOS AU are source-build targets;
the public workflows currently distribute VST3.

## Features

- Six FM operators, 32 algorithms and a selectable operator-routing diagram.
- Per-operator four-stage envelopes, ratio/fixed frequency, detune and keyboard scaling.
- Global pitch envelope, feedback, oscillator sync and LFO controls.
- PERFORMANCE page with POLY/MONO, pitch-bend range/step and portamento.
- Mod wheel, foot controller, breath controller and aftertouch assignments.
- DX7 single-voice/bank SysEx import/export and a persistent 32-slot USER bank.
- 148 host automation parameters and DAW project-state recall.
- EDIT/PERFORMANCE views, on-screen keyboard, pitch/mod wheels, output meters
  and audio-callback CPU display.
- Proportional GUI size choices: 50%, 75%, 100%, 125% and 150%.

## System requirements

A matching VST3 host is required for the plug-in. Build targets are **Windows x64**
and **macOS Universal**; macOS 11 is the deployment target, not a guarantee that
every OS/host combination has been tested. The owner reports macOS REAPER and
Windows 10 x64 REAPER use; this is not the complete final-RC test matrix. Physical
Intel Mac acceptance and exact-candidate checks remain separate. Standalone runs
without a DAW when built locally.

**Recommended and supported firmware: original Yamaha DX7 Mk I v1.8 (IG11469).** A legally
obtained user-supplied ROM is required for sound. **Special Edition / SER-7
firmware is not supported and is not recommended for VDX7.** Successful file
loading alone does not establish working MIDI, audio or project recall. Supported layouts and
optional external factory-bank data are described in the
[firmware guide](docs/guides/GUIDE_EN.md#3-firmware-and-banks).
Supported MIDI notes are **12–120** in both Native and Correct MONO modes.

Users of prebuilt artifacts do not need a compiler or CMake. **Signing notice:**
the Windows package has no publisher signature. The macOS Universal VST3 has a
technical ad-hoc signature, not Developer ID signing or notarisation. The OS may
show a security warning or make loading more difficult. Do not disable system-wide
protections; read the installation guide before proceeding.

## Installation

1. Close your host and back up the existing plug-in, projects and edited banks.
2. For the installer download, run the Windows `.exe` or macOS `.pkg` and
   follow its prompts. The macOS PKG installs to `/Library/Audio/Plug-Ins/VST3/`;
   the Windows EXE installs to `C:\Program Files\Common Files\VST3\`.
   For **Manual Install ZIP**, extract it and copy the complete `VDX7.vst3`
   bundle to your VST3 location:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\`
3. Rescan plug-ins in your host and load VDX7 as an instrument.
4. Use **LOAD ROM** to select your legally obtained original DX7 Mk I v1.8 firmware.
5. Select a program or load a compatible SysEx file, enable MIDI monitoring and play.

Avoid duplicate plug-in installations. Never disable system-wide security protections.
See [first sound and firmware setup](docs/guides/GUIDE_EN.md#2-installation-and-first-sound).

Archived DX7 banks can contain operator envelope values of 127 or fine-frequency
values of 100. VDX7 accepts these specific legacy values without rewriting the
bank data. Other parameter limits, seven-bit data, file structure and checksum
checks remain enforced; this is not unrestricted import of malformed banks.

Version 1.0.1 also supports your own factory SysEx files in
**SETTINGS → Bank folder**. Copy recognised ROM1A–ROM4B bank files there, then
choose **Refresh banks**, or start a new instance with compatible firmware.
Only available banks are enabled; complete bank content, not filenames, determines
their slots. Refresh keeps the current sound. Projects save the edited sound and
their bank catalog independently of these files. Firmware remains external.
See the [bank-folder instructions](docs/guides/GUIDE_EN.md#factory-bank-folder).

## Documentation

- [Documentation index and archive](docs/README.md)

- [Detailed English guide](docs/guides/GUIDE_EN.md)
- [Magyar útmutató](docs/guides/GUIDE_HU.md)
- [1.0 release checklist and remaining acceptance work](docs/release/ROADMAP_1.0.md)
- [Source dependencies](docs/guides/SOURCE_DEPENDENCIES.md)
- [License and third-party notices](NOTICE.md)

No live MIDI Out/SysEx transmission is implemented. Envelope graphs illustrate
shape rather than calibrated timing. Full firmware, offline-render, automation
and cross-platform acceptance must not be inferred from GUI approval or green CI.
See [known limitations](docs/guides/GUIDE_EN.md#9-known-limitations).

## Building from source

See [Building from source](docs/guides/GUIDE_EN.md#11-building-from-source) for C++20,
CMake 3.22+, platform tools, pinned dependencies and test commands.
Public CI runs ROM-free regressions; firmware-dependent tests require a legally
available local ROM that must never be committed or packaged.

For bug reports, include the build/commit, OS, architecture, host version,
sample rate/buffer and reproduction steps in
[GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues).

## About

![VDX7 Mk1. About — developer signature and component credits](docs/screenshots/vdx7-about.png)

*Actual 1.0.0-dev About window supplied by the project owner.*

Developed by **RobCZart82**. Thanks to
[chiaccona / VDX7](https://github.com/chiaccona/VDX7),
[Retromulator / dx7Lib](https://github.com/reales/retromulator)
and [JUCE](https://github.com/juce-framework/JUCE).
VDX7-JUCE is not a Dexed-based reimplementation.

## License

The wrapper and original GUI resources use [GNU AGPL-3.0-only](LICENSE.txt).
The DX7 core retains GPL-3.0-or-later and JUCE is used under AGPLv3;
see [notices](NOTICE.md). Provided without warranty.

Yamaha firmware is excluded. Yamaha is referenced only to identify compatibility;
this project is not an official Yamaha product and includes no Yamaha logo.
