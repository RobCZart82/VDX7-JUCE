# Exact 1.0.0-rc1 CI candidate validation — 2026-09-28

## Identity and scope

- Exact source commit: `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` (PR #93 merge).
- Candidate label: `1.0.0-rc1`.
- Exact-candidate workflow: [run 36451459751](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36451459751) — **PASS**.
- Post-merge main workflows on the same SHA: Windows `36450182898` and macOS
  `36450184841` — **PASS**.
- PR #93 Windows, macOS and ASan/UBSan checks — **PASS**. PR #93 fixed the
  macOS `lipo` invocation so the input binary precedes `-verify_arch`.

The CI/package results below do not imply runtime acceptance by themselves.
The owner has separately reported RC1 REAPER and listening tests on Windows
and macOS; their limited scope is recorded below. The local opt-in ROM suite
was also run on the exact product source (the checkout differed from the
frozen RC only in documentation): 36/36 CTests passed. The desktop-dependent
`vdx7_processor` test was deliberately excluded because it opens a save dialog.
The full host/audio matrix, final release acceptance, and publication
authorization are still open. The exact candidate workflow builds and uploads
VST3 artifacts only; AU/Standalone runtime or release distribution is not
claimed.

## Owner-reported exact-RC1 REAPER smoke test — 2026-09-28

The owner reports installing and trying the exact `1.0.0-rc1` VST3 on both
Windows and macOS and says it works well on both. The supplied screenshot shows
the plugin in REAPER **7.80** on macOS, including the `1.0.0-rc1` GUI footer.
The owner subsequently supplied the macOS “About This Mac” details: **Mac mini
(M1, 2020), 16 GB RAM, macOS Tahoe 26.7**. This identifies the reported macOS
test machine, but does not independently identify the installed VST3 binary by
hash. The visible device serial number is intentionally not retained.
The owner also supplied Windows system details: **Windows 10 Pro 22H2, build
19045.7663; Intel Core i7-6600U; 8 GB RAM**. This identifies the reported
Windows test environment, but the REAPER version and installed VST3 binary hash
remain unconfirmed. Device and product identifiers visible in the screenshot
are intentionally not retained.
This is recorded as an **OWNER-REPORTED CROSS-PLATFORM REAPER SMOKE PASS** for
the reported RC1 binaries, not as a test performed by the assistant.

The owner additionally reports listening to the exact RC1 on both macOS and
Windows and considers the sound quality excellent and faithful to the original
Yamaha DX7. Record this as an **OWNER-REPORTED CROSS-PLATFORM LISTENING PASS**;
the listening procedure, patch/ROM, monitoring chain, sample rate, buffer size,
and Windows binary hash were not supplied. The assistant independently
verified that the installed macOS VST3 hash matches the downloaded macOS RC1
candidate artifact. In REAPER 7.80 on macOS, the plugin displayed
`v1.0.0-rc1`, loaded the local ROM/factory banks, and a MIDI-keyboard note
produced visible host output-meter activity. This was a bounded smoke check,
not the full audio matrix or an independent subjective listening judgment.

The owner also reports running five VDX7 instances in one REAPER project, then
saving and reopening that project successfully. Record this as an
**OWNER-REPORTED FIVE-INSTANCE AND PROJECT SAVE/REOPEN PASS**. The tested OS,
REAPER version, project contents, duration/CPU behavior, and installed Windows
binary hash were not specified. This is positive bounded evidence, but does
not close the planned 1/4/8-instance, CPU, multi-platform, or broader state
matrix.

The Windows REAPER version and installed VST3 hash, ROM/voice, feature-by-feature
procedure, number of instances, audio rates/buffer sizes, project save/restore,
transport/offline render and stress results were not supplied. Do not infer
that the full host matrix or all GUI presets have passed from these reports.

## Candidate CI results

- Windows x64: exact SHA verified; candidate configured; source-packager unit
  tests and deterministic corresponding-source archive passed; VST3 and
  ROM-free CI targets built; all configured ROM-free CTests passed; artifact
  uploaded — **PASS**.
- macOS universal: same checks passed; the binary contains both `arm64` and
  `x86_64`; ad-hoc codesign and strict signature verification passed; artifact
  uploaded — **PASS**.
- Binary strings from both VST3s show `VDX7 Mk I     v1.0.0-rc1` and
  `Version 1.0.0-rc1` — **PASS**.
- The two downloaded corresponding-source archives passed
  `scripts/package_source.py verify`: **5,077 files** each, exact manifest and
  pinned dependency checks. Both platform copies have the same SHA-256:
  `db87e46d533d293b2812a93bcf1a2384c55a56a8d0eef22f0a650fce1c9d82fe`.
- The source packager labels these corresponding-source archives with its
  established `1.0.0-dev-<SHA>` filename convention. The manifest identifies
  exact source SHA `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684`, marks the snapshot
  not release-accepted, and pins JUCE/Retromulator. The surrounding CI artifacts
  are labeled `1.0.0-rc1` and include the same SHA. This naming distinction is
  intentional under the current development-source packager; do not mistake it
  for an RC binary mismatch.

## Downloaded artifact identities

| Platform artifact | Size | VST3 binary SHA-256 |
| --- | ---: | --- |
| `VDX7-1.0.0-rc1-Windows-x64-aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` | 32,556,234 bytes | `ce0136dc7ccc481690ee5d73a72376e91406c864925efe63c56a26bd260be507` |
| `VDX7-1.0.0-rc1-macOS-universal-aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` | 39,713,164 bytes | `586336823e5dffad470453990e943d11240228d9a11c2eedf6aba01c3c5b9386` |

Artifacts are available from the Actions run above while GitHub retains them.
Windows binaries have no publisher signature; macOS is ad-hoc signed, not
Developer ID signed or notarized. Checksums establish byte identity, not
publisher identity. The inspected artifact paths contained the VST3 bundle,
project documentation/notices and corresponding-source archive; the verified
source package rejected firmware, local absolute paths, build caches and
credential-like content. Do not describe these as public release assets.

## Owner test-phase decision — 2026-09-28

The owner elects to wind down open-ended exploratory testing because no
reproducible defect has been found. Any subsequently confirmed issue is
expected to be handled in a release after 1.0.0. This is a testing-scope
decision, not evidence that unrun checks passed or authorization to publish.
Keep the broader host/audio/GUI matrix below and in the release checklist
marked not run/deferred; review and explicitly accept or defer the remaining
gates before publication. A serious release-blocking defect would still stop
publication pending a fix.

## Remaining release-preparation items and deferred test scope

- The Windows installed-binary hash and REAPER version remain unrecorded; keep
  that evidence OWNER-REPORTED and incomplete rather than inferring an exact
  binary match.
- The broader REAPER functional, MIDI-boundary, automation, transport/offline,
  audio-rate/buffer, full instance/CPU and complete GUI-preset matrix was not
  run. The owner chose to close exploratory testing and defer those cells; they
  remain NOT RUN, not passes. See the release checklist for the enumerated scope.
- The exact-source local opt-in ROM suite passed 36/36 CTests; the desktop UI
  save-dialog test remains not run. No ROM was placed in CI/artifacts.
- Bilingual release notes and guides are being reconciled with these results.
  A non-publishing stable-package workflow is added in the current worktree;
  it must be merged and run on the final main SHA, then its binaries, source
  archive and checksums must be inspected. No stable tag or GitHub release has
  been created or authorized.
