# 1.0 RC acceptance checklist — not a publication authorization

Keep builds marked development until these gates close. The v0.6.6 checklist
is historical and must not be used to approve 1.0. No release or tag is created
by this checklist. No proprietary firmware belongs in source, CI or artifacts.

## Correctness and realtime gates

- [ ] MIDI product-range acceptance: Notes 12–120 inclusive reach and release
  correctly; Note On, Note Off, and velocity-zero Note On outside that range are
  rejected in both Settings modes before queueing or engine delivery. Local
  processor/deferred tests are added; run `vdx7_supported_note_range_acceptance`
  and perform REAPER boundary checks at 11/12 and 120/121 before release.
  The raw native MONO Note 0 firmware issue remains documented and characterized,
  but is intentionally unreachable from supported plugin MIDI. The selectable
  correction remains a separate compatibility option; do not claim the raw
  firmware issue itself was fixed. See `MIDI_RANGE_v0.7.0.md` and
  `DESIGN_MONO_NOTE_ZERO_POLICY.md`.
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

Recent targeted evidence (not closure of the broader gates): held-state restore,
ignored-CC admission, staged install boundaries, active bypass releases and wheel
delivery recovery are documented in their `VALIDATION_*.md` reports. Full host
suspension, public concurrent state calls and mixed controller timing still need
acceptance. Desktop-unavailable GUI tests must not be counted as passing.

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
