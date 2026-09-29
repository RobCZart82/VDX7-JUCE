# 1.0 build identity and package boundaries

Preparation checkpoint (2026-09-29): current main is
`60ee843aaefb3033e36dbbd12c45a2944a6a1723` after documentation-only PR #99;
the stable packages were built from product SHA
`f7a1248b2cfff0b6fb159c189ca6a5b2cff766ec`. The frozen RC1 product source is
`aeb4d5ee8439ba6a7346bfe7caba54ad90b21684`. Non-publishing stable package
workflow `36484917908` passed on the exact product SHA above for Windows x64 and macOS
Universal VST3 and generated matching source archives, build identity and
SHA-256 manifests. Artifact IDs and outer Actions ZIP digests are recorded in
the [execution plan](EXECUTION_PLAN_1.0.md); they are not the inner package
checksums. The artifacts remain test downloads, not a published Release or
approval to publish. This is a preparation record, not approval to rename
installed plugins or change their identity.

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
  names artifacts with the RC label, platform and full source SHA. The separate
  non-publishing stable preparation workflow was merged in PR #98 and passed
  as run `36484917908` on the exact product SHA above. It generated Windows x64
  and macOS Universal packages and corresponding source/checksum material; it
  does not publish a release. The outer artifact hashes are not the inner
  package checksums. Record SHA, build options, platform/architecture,
  dependency revisions and package checksums with each test; never describe a
  dev binary as an RC or stable build.
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
not stable release assets. Their independent content review is still pending;
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
The stable packages are built, and the owner reports that the Windows and
macOS Actions packages work. Their independent archive/content review and
separate publication approval remain outstanding; package hashes and host/test
details were not supplied.

On 2026-09-28, the owner additionally reported installing the Windows VST3 from
**Build Windows VST3 and Standalone #247**, source commit
`fbea5ea167598f9b625eab9145f53b8700d9eb15`. Reported VDX7.vst3 SHA-256:
`1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`.
This is owner-reported installation evidence, not a new functional-test result
or final-RC acceptance.
