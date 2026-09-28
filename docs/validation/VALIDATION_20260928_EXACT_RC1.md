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
The owner has separately reported an RC1 REAPER smoke test on Windows and
macOS; details and its limited scope are recorded below. No private-ROM suite,
full host/audio matrix, final release acceptance, or publication authorization
is claimed. The exact candidate workflow builds and uploads VST3 artifacts
only; AU/Standalone runtime or release distribution is not claimed.

## Owner-reported exact-RC1 REAPER smoke test — 2026-09-28

The owner reports installing and trying the exact `1.0.0-rc1` VST3 on both
Windows and macOS and says it works well on both. The supplied screenshot shows
the plugin in REAPER **7.80** on macOS, including the `1.0.0-rc1` GUI footer.
The owner subsequently supplied the macOS “About This Mac” details: **Mac mini
(M1, 2020), 16 GB RAM, macOS Tahoe 26.7**. This identifies the reported macOS
test machine, but does not independently identify the installed VST3 binary by
hash. The visible device serial number is intentionally not retained.
This is recorded as an **OWNER-REPORTED CROSS-PLATFORM REAPER SMOKE PASS** for
the reported RC1 binaries, not as a test performed by the assistant.

The Windows REAPER version/OS build, exact binary hash used on each machine,
ROM/voice, feature-by-feature procedure, number of instances, audio rates/buffer
sizes, project save/restore, transport/offline render and stress results were
not supplied. Do not infer that the full host matrix, local-ROM suite, audio
acceptance or all GUI presets have passed from this concise report.

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

## Still required before acceptance/publication

- Owner downloads and installs this exact artifact in REAPER on macOS and
  Windows; record application/OS versions and the exact binary hash used.
- Run the required REAPER functional, multi-instance, MIDI boundary, automation,
  save/restore, transport/offline-render and audio-rate/buffer matrix; mark
  unsupported combinations explicitly. Older owner reports remain scoped to
  the builds they tested.
- Run the exact-SHA local opt-in ROM suite using the owner's local ROM. No ROM
  was accessed or placed in CI/artifacts.
- Close the remaining F7 offline-render, F15 envelope/release, F16 dense
  MIDI/contention recovery and GUI Settings/About/size-preset checks as listed
  in the release plan.
- Finalize bilingual installation/release notes and confirm accepted signing
  warnings and package contents. Then present version, SHA, asset list and
  checksums for separate explicit publication authorization. No stable tag or
  GitHub release has been created.
