# Export corrected 1.0.1 final test packages

This report records the owner's 2026-10-05 approval and final test packaging
for the export-corrected 1.0.1 candidate. It does not grant host acceptance
or publication permission. Published v1.0.0, tags and release assets are unchanged.
The [single execution plan](../release/EXECUTION_PLAN_1.0.md) controls remaining
work; [preparation evidence](VALIDATION_20261005_EXPORT_FIX_PACKAGE_PREP_101.md)
is retained separately and its hashes are not the final download hashes.

## Exact source and approval

Product A and frozen packager B both equal
`96267f7304e657821ce35c54536689981f41ef27`. After reviewing preparation run
37269260691, the owner explicitly approved this pair for final test packaging
("jóváhagyom"). The policy update started from main
`3a1e73ef96d63f48232f7643d559890ccdfbe871`, whose Windows `37272134073`
and macOS `37272134026` checks PASS.

Protected [PR 130](https://github.com/RobCZart82/VDX7-JUCE/pull/130) final head
`2d03e23ac371f9fb1957d3a74c78223c2e89111c` passed Windows `37274112795`,
macOS `37274112791` and ASan/UBSan `37274112810`. There were no submitted
reviews or review threads; the merge state was clean and the merge was guarded
by the exact head SHA. C is `edcb7471bf2ee08d3f8430e317cb9d17274f7576`.
Distinct post-merge main Windows `37275184151` and macOS `37275184128` PASS
before workflow dispatch. The merge tree equals the tested policy head.
Source, tests, packaging scripts, workflows and installer files are unchanged
from A; only the separately reviewed policy and documentation changed.

The exact policy SHA-256 is
`341185da3ca9d4c6ad92de0da48ac0f91053f0cedb220057cfe6221422729715`.
PASS: the old immutable policy rejects the new pair despite a dirty working-tree
JSON edit. PASS: the committed new policy and merged C validate exact A/B,
ancestry and the canonical main workflow context. These local context strings
are not authenticated GitHub identity; the actual canonical workflow is the
separate hosted authority.

## Final packaging status

Canonical main run [37276345684](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37276345684)
was dispatched from C with source A and `accepted_for_publication=true`.
The workflow retains read-only contents permissions and cannot publish.
PASS: authorization `111654102756`, Windows `111654142408`, macOS
`111654142410` and assembly `111657327619`; total duration 11 minutes 22 seconds.
The hosted authorization output binds exact A/B/C, policy hash and main workflow.
Both platform suites PASS 77/77 Python tests. Windows PASS 14/14 ROM-free
CTests in 3.42 seconds; macOS PASS 14/14 in 2.25 seconds. Public jobs contain
no private firmware/bank execution. Accepted metadata is not final human
publication authorization.

## Actual final downloads and independent inspection

PASS: all three outer ZIPs match their GitHub API SHA-256 digests. The complete
artifact contains exactly seven checksummed files plus SHA256SUMS.txt. The four
product files and separate two-file source staging match those same bytes.
PASS: all seven inner checksums and an independent positive control plus three
negative controls (changed bytes, extra file and duplicate checksum entry).
Both BUILD-INFO files record source A, packager B, approval C, canonical workflow,
main ref, run 37276345684, attempt 1 and accepted-but-not-published status.
The source manifest records `release_accepted=true` and exact policy proof.

| Actions artifact | ID | Outer ZIP SHA-256 |
| --- | --- | --- |
| Complete validation | 11330254467 | `4a32f8346358caf9aa80f83dee91ee43f660569980875763c52771745938ef09` |
| Four product downloads | 11331225153 | `59dabedbc5a49f41aee76a0306e2041f9b286104e8ed76a90d4e1a09d5254af3` |
| Separate source | 11330463965 | `eca1c42d0897228f93b6dae09e9d5e481228b5b3c89dcdd91bd1dbb65de04b7e` |

| Final file | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `d9472981ee8d651ca1b609e3e94683470e19be2a7d841c21e1b62edc394ac84e` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `ebe161d3826cb885cfa2c62aa85458cdad28414d80e1f1b59fcfe056a87bda88` |
| `VDX7-1.0.1-macOS-universal.pkg` | `08be0e9d93db219f0516a42c2848ba5e4b10f72a9e57cfca1d2902f711817b69` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `18e518e1855c53d2f0bf9d6260a287ea05ac5c2624dfe91fb942fca25e506fbc` |

Download [all four test products](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37276345684/artifacts/11331225153)
and [matching source with checksum](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37276345684/artifacts/11330463965).
These are expiring Actions artifacts, not published Releases. Extract the outer
four-product artifact first; it contains the actual EXE, PKG and two Manual ZIPs.
The final source archive is not the dependency-free automatic GitHub source ZIP.

PASS: Windows plugin PE is x86-64; Mac Mach-O contains arm64 and x86_64,
version 1.0.1, bundle ID `org.vdx7.prototype`. Both Manual packages contain VST3
only, no AU/Standalone/ROM/SysEx banks. Independent XAR/CPIO inspection finds
exactly five Mac bundle files, byte-identical to the Manual plugin, installed
under `/Library/Audio/Plug-Ins/VST3/VDX7.vst3`. PKG ID
`com.robczart82.vdx7.vst3`, version 1.0.1, install root `/`, no package signature.
Hosted macOS strict ad-hoc codesign verification PASS; local Windows macOS
installation/codesign execution NOT RUN. Windows Setup Authenticode NotSigned.

PASS: actual hosted Windows upgrade/uninstall smoke binds candidate installer
hash `d9472981ee8d651ca1b609e3e94683470e19be2a7d841c21e1b62edc394ac84e`
and published 1.0.0 baseline
`670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3`.
Stable AppId, existing uninstall directory and exactly one registration are
preserved; both candidate payload hashes match installed files. Uninstall removes
bundle/registration and preserves synthetic USER/document bytes. This is not
installation on the owner's machine or a playable-bank/firmware/host test.

PASS: the actual extracted final Windows Manual plugin passed the independently
built official Steinberg SDK 3.8.1 validator, 47 tests passed, zero failed,
version 1.0.1, Instrument/Synth. The module itself reports SDK 3.8.0. No firmware
or REAPER was used. Unsupported 64-bit processing is handled as unsupported,
not claimed as implemented; this is not audible or host/REAPER acceptance.

Observed hosted Windows toolchain: CMake 3.31.6, MSVC 19.44.35229.0,
SDK 10.0.26100.0, Inno 6.7.1. Hosted Mac: CMake 4.4.3, Xcode 15.4,
AppleClang 15.0.0.15000309, SDK 14.5, deployment target 11.0.
No bit-reproducible installer or publisher-signature claim is made. Node 20
action deprecation, future ubuntu-latest migration and Mac runner-capacity
notices are maintenance warnings, not failed tests or product defects.

## Fresh corresponding source verification

The independently generated accepted source archive is
`VDX7-1.0.1-96267f7304e657821ce35c54536689981f41ef27-corresponding-source.zip`.
Its local SHA-256 is
`d916d3d0cba018736a0a8202270b373f5f2d9afbbf32b9b58c9bb190a3bba4ba`.
PASS: current and bundled standalone checker verify all 5,131 manifest files;
the bundled checker also runs with Git absent from PATH. Approval proof binds
the exact A/B/C, canonical workflow/ref and policy hash. Integrity and internally
consistent metadata are not publisher authentication or a signature.
PASS: actual hosted source ZIP is byte-identical to this locally generated and
freshly offline-tested archive. All 290 included wrapper files match immutable
Git blobs at A. The current verifier also PASS on the downloaded source, and
both product/source staging scripts PASS on the downloaded complete inventory.
The source includes pinned dependency sources, required notices and checker;
proprietary data is excluded.

Pinned JUCE is `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`;
Retromulator is `d5473776a0449d60a997b91bdc888598a33265ac`.
A new extraction and build directory use bundled dependencies only,
`FETCHCONTENT_FULLY_DISCONNECTED=ON`, stable 1.0.1, Windows x64 Release and
ROM tests OFF. PASS: fresh offline configuration (MSVC 19.44.35229.0,
SDK 10.0.26100.0, CMake 3.31.6), VST3 and CI-target compilation.
PASS: 14/14 ROM-free CTests, 2.32 seconds. PASS: actual JSON test inventory,
labels, fixtures and timeouts, plus the checker's nine negative controls.
These source tests do not validate audible firmware behavior or the hosted
binary. Native execution used an isolated temporary directory; no plugin was
installed. Local policy Python suite: 76 PASS and one symlink-capability
SKIPPED (WinError 1314), 77 total in 68.542 seconds. SKIPPED is not PASS.

The first download attempt failed under restricted network access; permission
was granted and all actual downloads/digests subsequently PASS. The local
inspection helper initially omitted SHA256SUMS.txt from extracted assets, so
the optional staging rerun correctly failed closed for a missing manifest.
The helper was corrected to extract the already verified manifest unchanged;
both staging reruns then PASS. Neither was a product/package failure, neither
required a repository source change, and neither initial result is hidden.

## Owner tests and publication still open

NOT RUN: installation on the owner's computer, exact final Windows/macOS
REAPER/ROM/audio acceptance, sample-rate and buffer matrix, automation,
multiple instances, HiDPI, native save dialogs, offline versus real-time render,
and physical Intel Mac testing. The owner performs these on the exact final
downloads, or explicitly disposes of each unrun check where the plan permits.
Earlier 1.0.0 or different-candidate host tests are not final 1.0.1 acceptance.
The previous 47/47 local source tests, including private firmware coverage,
are bounded by the exact PR 128 source
and [regression report](VALIDATION_20261005_SYSEX_EXPORT_ACKNOWLEDGEMENT.md);
they were not rerun merely for a policy/documentation change.

Windows EXE remains unsigned; macOS PKG is unsigned and its plugin is only
technically ad-hoc signed, not Developer ID signed or notarized. Do not weaken
system security to suppress warnings. Supported firmware is original DX7 Mk I
v1.8 (IG11469), not SER-7. Invalid combined factory data is rejected in full
with old state preserved; test old project recovery on copies and keep backups.

NOT RUN and not authorized: tags, source/product GitHub releases and asset
uploads. After owner acceptance and separate publication approval, publish
durable matching source no later than binaries as `v1.0.1-source` (not latest),
then `v1.0.1` as latest with exactly four product downloads. Actions artifacts
expire and are not durable corresponding-source publication. No Yamaha firmware
or factory-bank data is committed, uploaded or included in any package.

Magyarul: a pontos 96267f7 forrás–csomagoló páros jóváhagyva, a védett policy
és main ellenőrzések PASS. A végső csomagolás, tényleges fájlvizsgálat,
friss offline forrásbuild és Windows VST3-validátor PASS. A tulajdonosi végső
REAPER-teszt és külön publikálási engedély
továbbra is nyitott; telepítés vagy publikálás nem történt.
