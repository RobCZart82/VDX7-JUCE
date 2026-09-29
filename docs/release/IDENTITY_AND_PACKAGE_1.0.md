# 1.0 build identity and package boundaries

Preparation checkpoint (2026-09-29): main is `d79ed5214d82caf70e3941e5a620bab137d3f9ca`, after PR #101. Non-publishing workflow [36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919) passed on this exact source SHA for Windows x64 and macOS Universal VST3. Both builds and ROM-free tests, platform package checks, and combined SHA-256 verification passed. The owner reports that the Windows/macOS installers and REAPER plugins work; the macOS package installed after the per-app Gatekeeper “Open Anyway” flow. This is owner-reported evidence; exact installed hashes, host versions and the full test matrix were not supplied. Full record: [stable installer acceptance](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md).

The combined Actions artifact (ID `11059321610`, outer ZIP digest `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`) is validation-only. Its BUILD-INFO files explicitly say not accepted/approved for publication. Do not publish unchanged. Independent review of validation artifact `11059321610` is complete: all seven inner hashes, the 5,082-file corresponding-source manifest/dependency pins, VST3 architecture/signature/payload and no-firmware/local-secret/path conditions passed. See [the detailed record](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md). Accurate final BUILD-INFO/source acceptance metadata and regenerated manifest, final HU/EN notes and review of deferred-test disclosures remain outstanding. Windows is unsigned; macOS package is unsigned and plugin bundle ad-hoc signed only. No stable tag or Release was created by this workflow.

## Identity (F17)

- CMake project/host numeric version is `1.0.0`. A host displaying that number
  alone does not prove that the binary is an accepted stable release.
- Default `VDX7_RELEASE_BUILD=OFF` with no candidate label displays
  `1.0.0-dev`. Keep this setting for development.
- Exact candidates use `VDX7_RELEASE_BUILD=OFF` and
  `VDX7_RELEASE_CANDIDATE=rcN` (for example `rc1`) and display `1.0.0-rc1`.
  RC labels are checked by a ROM-free regression test; combining a candidate
  label with stable mode, or supplying a malformed label, is rejected.
- A stable package uses `VDX7_RELEASE_BUILD=ON` with an empty candidate label
  and displays `1.0.0`. The exact-candidate workflow is non-publishing and
  names artifacts with the RC label, platform and full source SHA. The
  non-publishing stable installer workflow run `36621909919` passed on exact
  product SHA `d79ed5214d82caf70e3941e5a620bab137d3f9ca` (current main after PR
  #101), producing Windows x64 and macOS Universal packages plus corresponding
  source/checksum material. It does not publish a release. The independently
  inspected validation artifact and its inner hashes are recorded in the
  acceptance report; its preparation-only BUILD-INFO and source manifest are
  not suitable for direct publication. Record SHA, build options,
  platform/architecture, dependency revisions and package checksums with each
  test; never describe a dev binary as an RC or stable build.
- Keep `org.vdx7.prototype`, manufacturer `VdxP`, plugin code `VdX7`, product
  `VDX7`, and all 148 parameter IDs/order compatible. The historical bundle ID
  is an identity, not permission to rename it during release polish.
- Plugin DESCRIPTION still contains “prototype”; the top-level project
  description does not. This is cosmetic metadata, not an audio defect. Review
  removal separately on the frozen candidate and revalidate affected formats;
  no identity or metadata change is hidden in this hardening round.

## Product limits already documented

The HU/EN guides describe one 32-slot persistent USER bank (not a named-library
manager), supported single-voice/bank SysEx, external ROM requirements and
primary VST3 distribution. Windows/macOS builds also cover Standalone and
macOS AU compilation, but that does not establish their host/runtime support.
Keep owner-reported exact-RC1 REAPER/listening and five-instance project
save/reopen evidence distinct from executed checks. The full host/audio/GUI
matrix was not run and is deferred by owner decision; see the current
[acceptance checklist](RELEASE_CHECKLIST_1.0_RC.md) and
[RC1 validation](../validation/VALIDATION_20260928_EXACT_RC1.md).

## Package review and remaining blockers

The candidate and stable-package workflows do not publish tags or Releases.
The exact stable workflow completed successfully on the exact product SHA above and
produced Windows/macOS VST3 artifacts plus corresponding source ZIPs, build
identity and embedded checksum manifests. These remain Actions test artifacts,
not stable release assets. The latest validation artifact's independent content review is recorded in the acceptance report;
verify archive contents, versions, dependency revisions, embedded SHA-256 sums
and absence of ROMs, credentials, local paths, caches and unrelated files.
See [source packaging](SOURCE_PACKAGING_1.0.md). The owner reports that the exact Windows and macOS stable Actions packages
work. This is owner-reported runtime acceptance; installed package hashes,
host/version details and the full test matrix were not supplied. Independent
archive review and separate publication authorization remain outstanding.

- Owner decision (handoff dated 2026-09-28): distribute Windows without publisher
  signing and macOS with the existing ad-hoc signature, without Developer ID or
  notarisation. This is accepted risk, not a security guarantee. Make possible
  OS warnings/load friction prominent in both installation guides. The SHA-256
  manifest/checksum detects changes; it does not authenticate the publisher.
- Pinned source dependencies remain JUCE
  `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator
  `d5473776a0449d60a997b91bdc888598a33265ac`. Offline layout and prior build
  evidence are in [SOURCE_DEPENDENCIES](../guides/SOURCE_DEPENDENCIES.md).
- `NOTICE.md` now describes the 1.0 development source-package contract without
  changing upstream terms. New HU/EN candidate instructions are available;
  `release/README_HU_EN.txt` remains explicitly historical, not the 1.0 guide.
- [x] Stable workflow generated matching source archives, dependency notices,
  build identity and embedded checksum manifests for the exact SHA.
- [ ] Independently inspect both platform artifacts and archives: verify
  versions, dependency revisions and embedded hashes; scan for ROMs,
  credentials, local paths, caches and unrelated files. Where the source archive
  is available, repeat extraction/build checks on that exact archive, not merely
  an equivalent working tree.
- Keep installation instructions and supported platform/host claims aligned
  with the actual artifacts and recorded tests. The no-publisher-signature /
  macOS ad-hoc policy above remains in effect unless the owner explicitly changes it.

The owner reports successful exact-RC1 REAPER use and excellent sound on
macOS/Windows, and a five-instance project save/reopen success. The macOS
installed binary hash matches its candidate artifact; Windows hash and some
host/test details remain unrecorded. Preserve these as OWNER-REPORTED. The
owner deferred the wider host/audio/GUI matrix; do not infer unrun cells passed.
The owner reports that the Windows and macOS installer/REAPER tests work. Independent content review of validation artifact `11059321610` passed; it remains prep-only because BUILD-INFO and SOURCE_MANIFEST mark it unaccepted. Final release metadata/manifest regeneration and release review remain outstanding.

On 2026-09-28, the owner additionally reported installing the Windows VST3 from
**Build Windows VST3 and Standalone #247**, source commit
`fbea5ea167598f9b625eab9145f53b8700d9eb15`. Reported VDX7.vst3 SHA-256:
`1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`.
This is owner-reported installation evidence, not a new functional-test result
or final-RC acceptance.
