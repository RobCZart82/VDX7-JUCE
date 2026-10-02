# Factory-bank and source-integrity fixes — 2026-10-02

Baseline: main `928c8b7dbea9816df8e5cc3700354859aaa4d8d1`, after PR #113.
Branch: `codex/factory-bank-source-integrity`. Owner-approved implementation
of three reproduced audit findings; no tag, release or asset publication.

## BH-20261002-01 — factory-bank validation (P2)

The old engine accepted a synthetic 32 KB bank containing detune code 15
(+8, outside -7..+7), even though the shared packed-voice validator and SysEx
export rejected it. A valid same-size control loaded/exported successfully.

The engine now validates all 256 voices and all bytes, including names, as
seven-bit data before replacing firmware or live state. Semantic validation
reuses the existing packed-voice rules; reserved-bit compatibility of that
single-voice API is unchanged. A bad optional companion is ignored with the
existing visible warning; a bad 48 KB combined image is rejected.

ROM-free tests cover empty/null/partial buffers, slots 0/31/32/255, invalid
detune and high-bit names, with valid controls. Private local-firmware tests
cover optional and combined rejection, engine RAM preservation, processor
warning, SysEx export, project reopen and valid-companion loading. Firmware
fixtures remain local and are cleaned up; no firmware is embedded in tests.

## BH-20261002-02 — mandatory source contents (P2)

The baseline full archive passed verification. Removing LICENSE.txt without
changing the manifest correctly failed; removing both the file and inventory
entry and regenerating the outer hash incorrectly passed verification and
preparation staging. This models a defective/rehashed package, not defeating
a trusted immutable external checksum or publisher authentication.

Creation and verification now share a mandatory minimum: CMakeLists.txt,
LICENSE.txt, NOTICE.md, THIRD_PARTY.md, dependency licenses and dx7.cpp.
1.0.1 also requires the generated checker and source-package README. Historical
1.0.0 archives without both generated files remain supported. Presence, payload
hashes and modes are checked; this is not an independent comparison of the
entire archive against authenticated Git objects. Exact-candidate source review
and committed release approval remain separate requirements.

Regression fixtures independently enumerate required paths. Each required-file
removal with a matching regenerated inventory must fail; staging must leave no
output when the license is missing. Valid preparation/accepted fixtures and
existing approval checks remain covered.

## BH-20261002-03 — checksum output preservation (P3)

The baseline overwrote a synthetic input asset when it was also the output.
The writer now rejects equal/resolved paths, existing hardlink aliases and
symbolic-link outputs before opening the destination. Distinct normal output
still produces sorted portable checksum entries. This is local file safety,
not a defense against hostile concurrent filesystem races.

## Executed evidence

- PASS: before production fixes, new Python negative regressions exposed the
  expected rejection/preservation failures (14 failure/subtest results).
- PASS: after fixes, Python discovery completed 73 tests: 72 executed, one
  skipped due to Windows symbolic-link creation privilege (WinError 1314).
  Existing approval, staging, download-policy and packaging tests also passed.
- PASS: fresh Windows/MSVC x64 Release configuration using the pinned JUCE and
  retromulator dependencies, with private ROM tests enabled.
- PASS: fresh Windows/MSVC x64 Release VST3 and regression-target build.
- PASS: focused factory-bank test with private compatible firmware: validation,
  live RAM preservation, processor warning, export and project reopen.
- PASS: development and non-accepted stable-preparation corresponding-source
  creation at source/tooling commit `7f654b5643d695312085581ccb23ffe51dc17c01`,
  5,114 files. Preparation ZIP SHA-256:
  `b4aa81157167fa43f49d5ed9d3b99110bdd905f38195e95f9b9535a5249439cb`.
  Its bundled standalone checker passes. Separate preparation staging produces
  exactly ZIP + checksums. The same full archive with LICENSE.txt and its
  inventory entry removed, then rehashed, fails both checkers and staging;
  rejection leaves no staging output. These are diagnostic fixtures, not final
  candidate assets or accepted publication metadata.
- PASS: 44 registered CTests, registration/label/fixture/timeout/failure-policy
  contract and seven negative controls.
- FAIL: full original-fixture CTest run, 15/43 PASS and 28/43 FAIL. All 13
  ROM-free tests passed. The new focused bank test passes within the state
  transition executable, then later combined-ROM loading fails. This is not
  acceptable full-suite verification and the PR must remain Draft.

## Compatibility finding and required decision

The previously used local 48 KB combined fixture has no high-bit bytes or
invalid detune codes, but four factory voices in later banks contain fields
outside the existing single-voice validator's ranges: slots 147, 161, 174 and
199; three offending values are 127 and one is 100. The new whole-bank scan
rejects it before engine mutation, explaining the 28 ROM-loading failures.
Earlier tests exercised the firmware and selected voices, not validation of
every factory voice. The original fixture was not modified or normalized.

This does not establish whether those bytes are accidental corruption or
intentional legacy factory data. Do not claim that the original fixture is now
compatible, or that all firmware tests pass. The owner must resolve the public
combined-image behavior (strict rejection versus firmware-only fallback with
a visible warning), and acceptance must include a valid full-bank fixture.
Do not silently clamp factory patch values or weaken the shared SysEx/state
validator. Source/checksum fixes are independently verified; bank compatibility
and the complete native suite remain open before merge.

## NOT RUN / acceptance boundaries

Windows symlink capability test: NOT RUN, not a logic failure. Final-head remote
Windows/macOS/ASan-UBSan gates: NOT RUN at this checkpoint. Interactive desktop
save-dialog test, REAPER, installed-plugin replacement, native installers,
upgrade/uninstall and exact 1.0.1 binary acceptance: NOT RUN in this round.
The approval JSON remains unchanged and no final product/tooling tuple is
accepted. Parameter identities, approved GUI/PITCH policy and DSP are unchanged.
The [single execution plan](../release/EXECUTION_PLAN_1.0.md) is authoritative.
