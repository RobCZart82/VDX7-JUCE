# 1.0 release-preparation checklist — not a publication authorization

## Current correction checkpoint — 2026-10-02

1.0.0 is already published; the old preparation checklist below is historical.
Use [the active 1.0.1 plan](EXECUTION_PLAN_1.0.md) for A1–A10 dispositions.
Do not carry earlier PASS results forward as exact-1.0.1 evidence.

- [x] Pending-project A1/A2 local 36/36 regressions and required remote PR checks accepted; #104 merged.
- [x] A3 test source archive verifies with its documented bundled checker, offline build and 11/11 tests; #105 merged. Final 1.0.1 artifact remains a separate gate.
- [x] #114 merged at `9d53e9ee578ae614f76025d36ef7a258659de3f6`; exact final-head Windows/macOS/sanitizer checks PASS. Bank validation, mandatory source contents and non-destructive checksums implemented. Local valid-control 43/43 CTests and 72 executed Python tests PASS; original invalid-ROM failures retained, not relabeled as passes.
- [x] Post-merge Windows/macOS checks PASS at `9d53e9ee578ae614f76025d36ef7a258659de3f6` (runs `36992554669`, `36992554670`).
- [x] Non-publishing preparation dispatched as run `36993835698` from main for that exact source SHA, acceptance disabled. Local version/context/approval preflight PASS; approval JSON remains null.
- [x] #115 compiler-engine correction merged at `6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14`; all final-head checks and post-merge Windows/macOS PASS. Fresh unaccepted preparation `36998278475` authorization, both platforms and assembly PASS. Full local packaging suite: 76 PASS, one capability SKIP.
- [x] HU/EN guides explicitly document pending legacy-project risk; [1.0.1 release-note draft](RELEASE_NOTES_1.0.1_DRAFT_HU_EN.md) prepared without final acceptance/hash claims.
- [x] Repeat preparation with corrected compiler-engine verification: run `36998278475` PASS for both platforms, Windows installer/upgrade and combined four-download/two-file source staging. Prior `36993835698` FAIL remains historical. Green preparation is not publication acceptance.
- [ ] Inspect Windows EXE/manual ZIP and macOS PKG/manual ZIP, architecture/version/identity, payload/licenses, toolchain provenance, source self-verification and all hashes.
- [x] Actual 1.0.0 -> 1.0.1 Windows upgrade/uninstall and synthetic user-file preservation PASS on hosted runner in `36998278475`. No local installed plugin replaced. [Evidence](../validation/VALIDATION_20261002_PACKAGE_PREPARATION.md).
- [ ] Review strict invalid-combined-ROM rejection and pending legacy-project risk. A constructed valid test copy is not a certified replacement or matching-ROM migration; original ROM remains unchanged.
- [ ] Published/next-candidate asset provenance and HU/EN docs reconciled.
- [ ] Exact 1.0.1 source/platform tests, payload and installer upgrade checked.
- [ ] Remaining host tests explicitly executed or deferred, never assumed PASS.
- [ ] Separate approval for new tag/release/assets (not granted by merge approval).

## Historical 1.0.0 preparation record

This checklist distinguishes completed evidence from items the owner has
deferred. An unchecked item is not a pass. The owner has chosen to close
open-ended exploratory testing and handle any later confirmed issue after
1.0.0; that decision does not authorize publication. The v0.6.6 checklist is
historical and must not be used to approve 1.0. No release or tag is created by
this checklist. No proprietary firmware belongs in source, CI or artifacts.

Current work order and audit disposition: [consolidated plan](EXECUTION_PLAN_1.0.md).
GUI appearance approval does not close the technical/platform gates below.

Current main checkpoint (2026-09-29 after PR #101): `d79ed5214d82caf70e3941e5a620bab137d3f9ca`. The non-publishing installer workflow [36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919) passed on this exact SHA for Windows x64 and macOS Universal. Both builds and ROM-free CTests passed; Windows install/uninstall smoke test, macOS package payload checks, and combined `SHA256SUMS.txt` validation passed. Combined artifact ID `11059321610`, outer ZIP SHA-256 `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`.

The owner reports that both Windows and macOS installers and REAPER plugins work; the macOS install required the per-app Gatekeeper “Open Anyway” flow. Exact installed package hashes and detailed host/rate/buffer matrix were not supplied. This is OWNER-REPORTED evidence; see [stable installer acceptance](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md). Windows installer is unsigned; macOS `.pkg` is unsigned and the VST3 has only an ad-hoc signature.

Independent review of the validation artifact is complete: the downloaded outer digest matched GitHub; all seven inner asset hashes passed; the 5,082-file source archive verified against its manifest and exact source/dependency pins; VST3 payloads and package paths were inspected; no firmware, secrets or local paths were found. See [the detailed acceptance record](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md). The artifact remains preparation-only: BUILD-INFO says it is not approved for publication and the source manifest has `release_accepted: false`. Final release metadata and checksum regeneration, bilingual release notes and deferred-test review remain outstanding. The broader matrix remains NOT RUN / DEFERRED; see
[the RC1 validation report](../validation/VALIDATION_20260928_EXACT_RC1.md).
GitHub has no stable 1.0.0 release. No tag or publication was created by this workflow.

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

**Owner decision (2026-09-28):** wind down open-ended exploratory testing.
No reproducible defect has been found; confirmed defects found after 1.0.0 are
expected to be handled in a later release. This does not turn unrun checks into
passes. Keep the remaining host/audio/GUI matrix items below unchecked and
identify them as not run/deferred; review and explicitly accept/defer any
remaining release gates before publication.

Recent targeted evidence (not closure of the broader gates): held-state restore,
ignored-CC admission, staged install boundaries, active bypass releases and wheel
delivery recovery are documented in their `VALIDATION_*.md` reports. Full host
suspension, public concurrent state calls and mixed controller timing still need
acceptance. Desktop-unavailable GUI tests must not be counted as passing.

- [x] Exact-source local opt-in ROM integration suite passes: 36/36 CTests;
  desktop-dependent `vdx7_processor` save-dialog test intentionally excluded.
- [x] macOS and Windows CI pass on exact source SHA
  `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684`.
- [x] Exact-commit release-candidate workflow `36451459751` passes on that SHA;
  this is a non-publishing test workflow.
- [ ] M1 REAPER and Windows REAPER: 1/4/8 instances, UI, save/restore, CPU.
- [x] Owner-reported RC1 check: five VDX7 instances in one REAPER project;
  save and reopen succeeded. Platform, duration and CPU details not supplied;
  this does not complete the full instance/CPU/platform matrix.
- [x] Owner-reported RC1 listening-quality check on macOS and Windows; owner
  reports excellent, DX7-faithful sound. This subjective pass does not close
  the measured rate/buffer, transport, render, or multi-instance matrix.
- [ ] Transport play/stop/seek/loop/offline; device/sample-rate/buffer restart.
- [ ] 44.1/48/96 kHz × 64/128/256/512/1024 samples where host/device configurable;
  record unsupported combinations explicitly rather than silently omitting them.
- [ ] Production SRC frequency response, aliasing and latency accepted on the
  exact candidate; retain the synthetic resampling regression and separate
  listening/host acceptance. Historical linear-SRC wording is not a claim that
  the current implementation is still the original linear resampler.

## Packaging (only after correctness gates)

- [ ] Explicit 1.0.0-rcN identity matched to exact source SHA and dependency revisions.
- [x] Candidate VST3 labels, artifact names and exact source SHA agree; binary
  hashes were computed and recorded in the RC1 validation report.
- [x] Matching corresponding-source archives, embedded manifests, pinned
  dependencies, licenses/notices and source checksum manifest verified. The
  archives use the packager's `1.0.0-dev` filename convention but identify the
  exact candidate SHA; the candidate artifacts themselves are labeled `rc1`.
- [x] Draft HU/EN release notes and owner-reported/current CI evidence matrix created;
  finalize the matrix against the frozen RC before acceptance.
- [x] Source archive checker and manifest inspection found no firmware, local
  paths, secrets or build caches; only the VST3 bundle and source/docs are in
  the tested artifacts. Human review is recorded in the RC1 validation report.
- [x] Stable `1.0.0` Windows x64 and macOS Universal VST3 installer
  preparation passed on exact main SHA
  `d79ed5214d82caf70e3941e5a620bab137d3f9ca`; see workflow run
  `36621909919`, combined artifact ID `11059321610`, and the execution plan.
  This is CI preparation evidence only, not a public release.
- [x] Independently inspect the downloaded validation artifact: verify its
  embedded `SHA256SUMS.txt`, VST3/source archive contents, build identity and
  record inner asset hashes. Results and hashes are in
  `docs/validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md`.
- [x] Owner reports the Windows and macOS installers and installed VST3s work
  in REAPER; macOS installation required per-app Gatekeeper “Open Anyway”.
  Exact installed package hashes, host versions and the broader test matrix
  were not supplied.
- [ ] Separate user authorization for any public release/tag/assets.

## Repository policy

The exact RC1 host reports are recorded above and in the validation report.
Unrun rate/buffer, transport/render, boundary, dense-MIDI, GUI-preset, and
broader instance/CPU checks remain NOT RUN / DEFERRED—not passed. Do not reopen
open-ended exploration absent a reproducible issue; do not misstate the deferred
cells. Recheck current main and required Actions before freezing a stable source
SHA. Never disable or bypass branch protection, force-push, or rewrite main.
Source backup and release publication are separate actions.

Main protection now requires a Pull Request and successful `build-macos` and
`build-windows` checks. Never disable or bypass protection, force-push, or
rewrite main. Source backup and release publication are separate actions.
