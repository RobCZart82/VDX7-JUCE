# 1.0 RC acceptance checklist — not a publication authorization

Keep builds marked development until these gates close. The v0.6.6 checklist
is historical and must not be used to approve 1.0. No release or tag is created
by this checklist. No proprietary firmware belongs in source, CI or artifacts.

## Correctness and realtime gates

- [ ] MONO Note 0 product acceptance: real Note 0 pitch/audio and release,
  repeated/overlapping notes, subsequent notes, legato, sustain and portamento.
  `vdx7_mono_note_zero_acceptance` is a currently failing, non-inverted local
  CTest release blocker. Passing characterization/candidate experiments or
  ROM-free CI are NOT a replacement. See `VALIDATION_MONO_CANDIDATE.md`.
  Initial engine-opt-in processor coverage is in `VALIDATION_MONO_ENGINE_OPTIN.md`;
  state persistence/new-instance restore is now covered in
  `VALIDATION_MONO_PERSISTENCE.md`; `VALIDATION_MONO_SETTINGS.md` adds UI
  selection/status. Broader corrected-mode transition/host acceptance remains.
- [ ] Program/edit and bank/edit ordering in both directions, including stopped transport/save.
- [ ] Audio callback has no direct host parameter notification; ownership audit of all call sites.
- [ ] Deferred MIDI preserves an explicit multi-block timeline policy; overflow reconciliation.
- [ ] Missing-ROM re-save retains performance and explicit voice edits without losing original RAM.
- [ ] Invalid/unreadable optional factory image falls back to valid firmware with a warning.
- [ ] Audio-device restart clears stale notes, serial events and deferred MIDI.
- [ ] Program Change 0/31/32/127 metadata agrees with firmware input.
- [ ] Dense MIDI and automation, held notes, program/bank changes, live SysEx.
- [ ] Compatibility: 148 parameter IDs/indices, plugin identity, legacy state, SysEx.

## Acceptance and evidence

- [ ] Local opt-in ROM integration and ROM-free tests pass on the candidate source.
- [ ] macOS and Windows CI pass on that exact source SHA.
- [ ] Exact-commit release-candidate workflow passes on that SHA.
- [ ] M1 REAPER and Windows REAPER: 1/4/8 instances, UI, save/restore, CPU.
- [ ] Transport play/stop/seek/loop/offline; device/sample-rate/buffer restart.
- [ ] 44.1/48/96 kHz × 64/128/256/512 samples.
- [ ] Linear SRC frequency response and aliasing measured; quality decision documented.

## Packaging (only after correctness gates)

- [ ] Explicit 1.0.0-rcN identity matched to exact source SHA and dependency revisions.
- [ ] Matching corresponding-source archive, licenses and checksum manifest.
- [ ] HU/EN installation/readme, release notes and honest platform/host support matrix.
- [ ] Package inspected for absence of firmware, local paths, secrets and build caches.
- [ ] Separate user authorization for any public release/tag/assets.

## Repository policy

Main protection now requires a Pull Request and successful `build-macos` and
`build-windows` checks. Never disable or bypass protection, force-push, or
rewrite main. Source backup and release publication are separate actions.
