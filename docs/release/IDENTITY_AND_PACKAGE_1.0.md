# 1.0 build identity and package boundaries

Source review: 2026-09-28, post-#89 main baseline `fbea5ea`. This is a preparation record,
not approval to publish, rename installed plugins or change their identity.

## Identity (F17)

- CMake project/host numeric version is `1.0.0`. A host displaying that number
  alone does not prove that the binary is an accepted stable release.
- Default `VDX7_RELEASE_BUILD=OFF` displays `1.0.0-dev`. Keep this setting for
  development and current exact-commit candidates until all acceptance gates
  pass. The existing candidate workflow explicitly sets it OFF.
- Candidate workflow artifact names include the full source SHA. Record that
  SHA, build options, platform/architecture, dependency revisions and binary
  checksum with each test. A future `rcN` display label is not implemented by
  the current boolean option; do not describe a dev binary as displaying rcN.
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
Keep the dated owner-reported REAPER evidence and its missing exact-SHA details
separate from future candidate acceptance.

## Package review and remaining blockers

The candidate workflow has read-only repository permissions and no
release/tag publication step. The follow-up adds a matching complete source ZIP,
embedded manifest and source ZIP checksum beside the VST3/documents/licenses.
See [source packaging](SOURCE_PACKAGING_1.0.md). Final binary checksums and
distribution packaging are still separate gates; artifacts remain development builds.

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
- Generate matching complete source with dependency notices, checksums and
  an exact-SHA manifest; inspect final archives for ROMs, credentials, local
  paths, caches and unrelated files. Repeat offline extraction/build checks
  on that actual source archive, not merely an equivalent working tree.
- Keep installation instructions and supported platform/host claims aligned
  with the actual artifacts and recorded tests. The no-publisher-signature /
  macOS ad-hoc policy above remains in effect unless the owner explicitly changes it.

The owner reports REAPER acceptance. Existing records identify macOS 26.7 and
Windows 10 x64 build #219 / `29ab5e3`, but not every tested SHA/binary hash,
REAPER application version or the full rate/block/instance matrix; preserve
that as OWNER-REPORTED. No final RC is frozen or accepted by this document. Final archive inspection,
exact-candidate host matrix and separate publication approval remain OPEN.
