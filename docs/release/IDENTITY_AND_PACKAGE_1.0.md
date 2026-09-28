# 1.0 build identity and package boundaries

Source review: 2026-09-28, baseline `811f3a3`. This is a preparation record,
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

The existing candidate workflow has read-only repository permissions and no
release/tag publication step. It uploads the VST3 and selected documents and
licenses. It is **not yet a complete corresponding-source/checksum release
package**. Build workflows' VST3 ZIPs likewise are development artifacts.

- Windows code signing is not configured in these workflows.
- macOS performs ad-hoc signing; this is not Developer ID/notarisation.
- Pinned source dependencies remain JUCE
  `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator
  `d5473776a0449d60a997b91bdc888598a33265ac`. Offline layout and prior build
  evidence are in [SOURCE_DEPENDENCIES](../guides/SOURCE_DEPENDENCIES.md).
- `NOTICE.md` still describes the historical v0.6.6 published package;
  `release/README_HU_EN.txt` is explicitly historical. Neither proves that a
  matching 1.0 source archive has been generated. Before publication, prepare
  candidate-specific notices/readmes while preserving upstream licenses.
- Generate matching complete source with dependency notices, checksums and
  an exact-SHA manifest; inspect final archives for ROMs, credentials, local
  paths, caches and unrelated files. Repeat offline extraction/build checks
  on that actual source archive, not merely an equivalent working tree.
- Keep installation instructions and supported platform/host claims aligned
  with the actual artifacts and recorded tests. Final signing/notarisation
  policy requires an explicit owner decision if distribution policy changes.

No final RC is frozen or accepted by this document. Final archive inspection,
exact-candidate host matrix and publication approval remain OPEN.
