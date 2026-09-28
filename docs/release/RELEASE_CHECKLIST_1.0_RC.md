# 1.0 RC acceptance checklist — not a publication authorization

Keep builds marked development until these gates close. The v0.6.6 checklist
is historical and must not be used to approve 1.0. No release or tag is created
by this checklist. No proprietary firmware belongs in source, CI or artifacts.

Current work order and audit disposition: [consolidated plan](EXECUTION_PLAN_1.0.md).
GUI appearance approval does not close the technical/platform gates below.

Merged non-host checkpoint: #89 / `fbea5ea`. PR #89 Windows/macOS and
ASan/UBSan checks passed; post-merge main Windows/macOS builds passed. The
[hardening report](../validation/VALIDATION_20260928_NONHOST_HARDENING.md)
retains failing-baseline/fixed evidence. N6 retains keyboard PITCH values by
owner decision. New [source-package verification](../validation/VALIDATION_20260928_SOURCE_PACKAGING.md)
and post-merge local checks are separate from exact-RC acceptance.

Owner-reported decision (2026-09-28 handoff): the owner reports REAPER testing
with no known issue and considers the program releasable. The plan records
macOS REAPER checks and Windows 10 x64 using build #219 / `29ab5e3`; the macOS
tested SHA, binary hashes, REAPER application versions and full rate/block/
instance matrix are not recorded. Keep this marked OWNER-REPORTED; do not
convert unverified matrix entries to PASS. The owner accepts Windows distribution
without publisher signing and macOS with ad-hoc signing only, without Developer
ID/notarization. Clearly warn about
possible OS security prompts. This is not publication authorization.

Additional owner-reported installation evidence (2026-09-28): the owner installed
the Windows VST3 from **Build Windows VST3 and Standalone #247**, built from
`fbea5ea167598f9b625eab9145f53b8700d9eb15`. Reported VDX7.vst3 SHA-256:
`1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`.
This records installation and artifact identity only; no new functional test
result was stated, so it does not close the exact-RC REAPER acceptance matrix.
[Identity/package review](IDENTITY_AND_PACKAGE_1.0.md) distinguishes development
artifacts from final binary packaging, signing and acceptance gates.

## Correctness and realtime gates

- [x] Earlier F3/F4 packed-input, F1 wheel-input, F5 ROM-identity, F6 pre-ROM-edit
  and F10 Settings fixes are implemented and merged; F8/F9 have a source-review
  disposition. This records implementation only, not acceptance of a final RC.
- [ ] Repeat relevant regressions on the frozen RC and attach exact-SHA evidence.
- [x] Resolve/disposition the NEW `AUDIT-20260928-N1` through `N7` ledger entries
  in the consolidated plan. Do not confuse their IDs with older roadmap audits.
  Missing coverage or a policy question is not automatically a product defect.
  Merged #87/#88 implement the fixes/contracts; N6 is accepted policy. Re-run
  relevant evidence on the frozen RC before closing the separate release gates.
- [ ] Validate all five actual GUI size presets and required About assets;
  preserve owner-approved graphics during any resource cleanup.

- [ ] MIDI product-range acceptance: Notes 12–120 inclusive reach and release
  correctly; Note On, Note Off, and velocity-zero Note On outside that range are
  rejected in both Settings modes before queueing or engine delivery. Local
  processor/deferred tests are added; run `vdx7_supported_note_range_acceptance`
  and perform REAPER boundary checks at 11/12 and 120/121 before release.
  The raw native MONO Note 0 firmware issue remains documented and characterized,
  but is intentionally unreachable from supported plugin MIDI. The selectable
  correction remains a separate compatibility option; do not claim the raw
  firmware issue itself was fixed. Native firmware is the recommended default;
  Correct MONO Note 0 is an advanced option, and both modes share the same range.
  See `docs/design/MIDI_RANGE_v0.7.0.md` and
  `docs/design/DESIGN_MONO_NOTE_ZERO_POLICY.md`.
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
- [ ] 44.1/48/96 kHz × 64/128/256/512/1024 samples where host/device configurable;
  record unsupported combinations explicitly rather than silently omitting them.
- [ ] Production SRC frequency response, aliasing and latency accepted on the
  exact candidate; retain the synthetic resampling regression and separate
  listening/host acceptance. Historical linear-SRC wording is not a claim that
  the current implementation is still the original linear resampler.

## Packaging (only after correctness gates)

- [ ] Explicit 1.0.0-rcN identity matched to exact source SHA and dependency revisions.
- [ ] Matching corresponding-source archive, licenses and checksum manifest.
- [ ] HU/EN installation/readme, release notes and honest platform/host support matrix.
- [ ] Package inspected for absence of firmware, local paths, secrets and build caches.
- [ ] Separate user authorization for any public release/tag/assets.

## Repository policy

During the owner-authorized non-REAPER round (2026-09-28), host gates stay
NOT RUN for the new candidate. Record implementation and automated-test progress
in the consolidated plan; do not mark these final gates from historical results.

Main protection now requires a Pull Request and successful `build-macos` and
`build-windows` checks. Never disable or bypass protection, force-push, or
rewrite main. Source backup and release publication are separate actions.
