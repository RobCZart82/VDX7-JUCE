# Repository and release preparation review 2026-10-02

Reviewed main: `5298373020374293302c3db806fbf44958046a02` after #116.
This is a source, regression and preparation-artifact review, not final
host acceptance or a claim that the whole repository is free of defects.
No runtime fix, installed plug-in change, tag or release publication occurs.

## Scope and findings

Reviewed all changes since `d4f589764284e72a2d532a6055ac8d918cb53d9c`:
32 files across processor/engine/voice validation, tests, packaging scripts,
Windows and stable workflows, guides and release records. Rechecked related
state-save/restore, audio try-lock/deferred-MIDI boundaries, approval/source
staging and toolchain capture. Earlier audited unchanged paths are not claimed
to have received a new exhaustive line-by-line audit.

No additional reproducible runtime defect was found in this review. The bank
validation, minimum source contents, checksum output-alias protection and Inno
compiler-engine probe changes are consistent with their regression controls.
GUI, plug-in codes, installer layout and CMake metadata are unchanged since the
previous reviewed version round. No new feature is required before publication.

Two active-document status errors were found: the plan still described #116 as
open, and plan/release notes still described independent download review as
blocked. Corrected current status without rewriting dated historical evidence.

The strict combined-ROM policy remains a material compatibility boundary:
the earlier original 48 KB fixture was rejected, and its historical 28/43
failures remain failures. A constructed valid control passing 43/43 does not
certify the original ROM or guarantee legacy-project recall. The owner-chosen
policy and state-preservation coverage are documented in
[bank validation](VALIDATION_20261002_BANK_SOURCE_INTEGRITY.md).

## Fresh checks

- PASS: 77/77 Python tests on macOS Python 3.9.6, no skips, 11.819 seconds.
- PASS: reconfigured/rebuilt native `vdx7_ci_checks` from this main with ROM
  tests OFF, then 13/13 CTests, 3.52 seconds, parallel 4.
- PASS: fresh state-transition executable `--bank-only` with private local
  firmware: rejection, preservation, warning, export and reopen. This focused
  test builds synthetic banks; it is not the full original-ROM suite.
- PASS: YAML parsing for all five workflows and whitespace checks.
- PASS: main Windows `37002235337` and macOS `37002235328` at reviewed SHA.
  Final #116 head `d7a17a8db5dffafd95cce3f9a93e1355b1f0f991` Windows
  `37001238670`, macOS `37001238709`, ASan/UBSan `37001238669`: PASS.
- No open PRs or issues at the read-only GitHub checkpoint. This is not proof
  that unreported bugs do not exist.

The previous temporary actionlint executable no longer exists; fresh actionlint
was NOT RUN. YAML parsing is not a replacement lint claim. The unchanged
workflows retain their earlier lint/actual native preparation evidence.

## Independent preparation artifact inspection

Downloaded the exact existing run `36998278475` through authenticated GitHub
API, not browser UI, into a separate local inspection directory. All three
outer SHA-256 values matched GitHub metadata:

| Artifact | Outer ZIP SHA-256 |
| --- | --- |
| Release Assets `11222504465` | `0eada5e0c73d06330ddd5433baf9f6ffc9cfb50d959fc0e726be26e4a0fc8b89` |
| Four Downloads `11222698809` | `540058178f9dc283c7dcc1b4a5c815b00dcee8c222e66483ad7174014a4aaafd` |
| Source Downloads `11222783226` | `ac32fc3d146072b04a2a67e0b14485f48a94fcde6e0fbcea70d4c68c0ca33563` |

All seven inner checksums PASS. Four Downloads contains exactly the four
binary files, byte-identical to complete validation. Source Downloads contains
exactly source ZIP + one-file checksum, and both source copies match.

Preparation binary hashes, NOT final accepted publication hashes:

| File | SHA-256 |
| --- | --- |
| Windows x64 Setup EXE | `b99e9a8ebe6b32b46fac2f168a6a325c41608e51b78582150015ce1bea917d02` |
| Windows x64 Manual ZIP | `74e1f7e48ea847e927b1532bf3a814467690c023ab802ba41b63d344be09e733` |
| macOS Universal PKG | `a51a913e12a01e5c9272f425b9afdce24d5f4d0cc17f32c90f074e5d5ef38d9f` |
| macOS Universal Manual ZIP | `e9e501eef264c32df48a4a9cd7ef59c21b9810bdaa92c6dbc7c947e5623bb4ca` |

Source SHA-256:
`37f62d48dac516ccc1e5e85fc5d460a0412309352ae3cf5d568268f5fc2e8fa9`.
Both the current checker and extracted bundled checker verified 5,117 files;
the bundled run used `PATH=/nonexistent`, without Git or network access.
This is verification, not an offline compilation.
All 276 product-tracked paths
present in the archive were independently compared to exact `6cc8cda…` Git
objects and matched. Mandatory product/dependency licences and notices exist.
This comparison does not independently authenticate every dependency file;
the manifest/pins and earlier dependency provenance remain distinct evidence.

Both BUILD-INFO records bind source/tooling/workflow to
`6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14`, canonical main workflow,
1.0.1, and preparation-only status. `release_accepted` is false. Windows records
MSVC 19.44.35229.0, SDK 10.0.26100.0, Inno 6.7.1 and hosted upgrade/uninstall
PASS. Mac records Xcode 15.4, AppleClang 15.0.0, SDK 14.5 and target 11.0.
These are observed build versions, not bit-reproducibility promises.

Windows Manual payload is an x86-64 PE DLL. Mac Manual payload is arm64+x86_64,
Info.plist version 1.0.1, identifier `org.vdx7.prototype`, minimum macOS 11.0;
strict code-signature verification PASS, ad-hoc only, no TeamIdentifier.
PKG is unsigned; expanded payload installs under
`/Library/Audio/Plug-Ins/VST3/VDX7.vst3`, and every Manual bundle file matches
the expanded PKG byte-for-byte. No installation was performed. Manual ZIPs
contain only VST3 bundles, not AU/Standalone or user firmware; licence/guide
delivery is through corresponding source and release documentation.

The SDK-generated moduleinfo files use trailing commas. A Python strict JSON
reader rejected them; inspection of the pinned SDK's moduleinfo parser confirms
support for its extended JSON format. This diagnostic parse mismatch was not
classified as a reproduced host defect or repaired by rewriting generated data.

## Remaining gates and limits

Source/runtime/tests/scripts/CMake/installer/workflow bytes are identical between
preparation source `6cc8cda…` and reviewed main `5298373…`; later changes are
documentation. Native source regression evidence can therefore be reused with
that explicit boundary. The old archive does not include newer guide/plan text.

Choose and freeze final A/B. If current documentation is included, create its
matching preparation archive and repeat changed inventory/hash checks. Run the
final corresponding-source offline build or obtain explicit deferral. The full
private-ROM suite, interactive save dialog, independent Windows EXE extraction,
local installer execution, REAPER/audio/multi-instance matrix and physical Intel
Mac were NOT RUN here. Earlier reported/control results are not relabelled.

Obtain exact-candidate host acceptance or explicit, itemized owner deferrals,
including strict-ROM legacy-project risk. Commit exact A/B approval in separate
C, produce accepted artifacts, recheck actual final hashes/contents, finalize
HU/EN notes and source access, then obtain publication authorization. Publish
separate matching `v1.0.1-source` non-latest and `v1.0.1` with four downloads
only after those gates. Approval JSON remains null and v1.0.0 is untouched.

The [updated single plan](../release/EXECUTION_PLAN_1.0.md) is authoritative;
this report records evidence rather than creating another roadmap.
