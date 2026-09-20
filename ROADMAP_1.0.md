# VDX7 1.0.0 release gate

Target: stable 1.0.0, not another public pre-beta. Work-in-progress builds are
not final releases. Do not publish or replace existing release assets until
the acceptance checklist is complete. Keep plugin IDs and existing parameter
indices compatible with saved projects.

## Stabilization

- [x] Missing-ROM deferred project restore, including re-save/restart (local automated tests).
- [x] Invalid-ROM rejection preserves RAM and pending UI edits; firmware-only reload tested.
- [x] CC64/65 thresholds and CC11 expression regression tests.
- [x] Preserve MIDI note-offs during engine transactions; bounded overflow recovery tested.
- [x] Validated live bank SysEx with coherent GUI/host state (local tests).
- [x] Remove 512-sample lookahead; measure onset and host-block invariance.
- [ ] Physical MIDI timing, dense chords and automation acceptance in hosts.
- [ ] Audio-thread ownership/notification audit and contention stress test.
- [x] Full 0–127 note range and pitch/release regressions (local ROM).
- [x] Configure PR ROM-free CI and opt-in local ROM integration CTest gate.
- [x] Run the development CI configuration on GitHub: macOS and Windows passed
  on `5164fabd36c8fdd745e272fc1f493c0c352c1ced`. Every subsequent change needs
  fresh checks; this does not constitute RC or host acceptance.

## Features and quality

- [ ] PERFORMANCE/SETTINGS scope implemented and tested: pitch range, controller
      assignments, MIDI input channel and tuning, preserving state compatibility.
- [ ] Decide mono/portamento scope from firmware capabilities and host tests.
- [ ] Measure resampling/aliasing; quality implementation accepted against references.
- [ ] Current documentation matches all shipping features and limitations.

## Publication gate

- [ ] M1 REAPER acceptance: multiple instances, automation, transport stop,
      missing ROM, state restore, SysEx, offline render, 44.1/48/96 kHz,
      64/128/256/512 sample buffers.
- [ ] Windows REAPER acceptance and runtime prerequisites verified.
- [ ] Physical Intel Mac acceptance, or explicitly do not claim Intel support.
- [ ] All distributed plugin formats have host validation.
- [ ] Version 1.0.0 in one source of truth; unique exact source commit/tag.
- [ ] Verified signatures, corresponding sources, notices, checksum manifest.
- [ ] No Yamaha firmware/factory ROM in source or binary release archives.
- [ ] Final release notes and package installation instructions approved.
- [ ] Publish stable release only after the preceding gates; never clobber old assets.

The installed plugin and the original local development checkout are preserved.
Local full-range development commit 80ebf54 and reviewed main 483daf7 have been
merged into the isolated codex/1.0-stabilization branch.
