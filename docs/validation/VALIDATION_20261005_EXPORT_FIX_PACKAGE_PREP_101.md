# Export corrected 1.0.1 package preparation

The export correction merged in PR 128 has passed new preparation packaging and
independent inspection of the actual downloaded files. This checkpoint does not
approve the new product/tooling pair for final packaging, claim REAPER acceptance
or authorize publication. The dbad14a approval record and published v1.0.0 remain
unchanged. The [single execution plan](../release/EXECUTION_PLAN_1.0.md) controls
the remaining gates.

## Exact candidate and executed platform tests

Product source A and proposed frozen packager B are both
`96267f7304e657821ce35c54536689981f41ef27`. Remote main was verified at that SHA,
with no open PRs, before this preparation. PR 128 final-head Windows, macOS,
ASan/UBSan and local 47-test evidence is recorded in the
[regression report](VALIDATION_20261005_SYSEX_EXPORT_ACKNOWLEDGEMENT.md).
Post-merge main Windows `37267855991` and macOS `37267855982` passed before
packaging; the merge tree matches the tested PR head.

Preparation run [37269260691](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37269260691)
used canonical main and `accepted_for_publication=false`. Authorization,
Windows packaging, macOS packaging and assembly all PASS. Both platform jobs
passed 77/77 Python tests. Windows passed 14/14 ROM-free CTests in 2.26 seconds;
macOS passed 14/14 in 2.00 seconds. Neither public platform executed private
firmware or bank-folder tests.

Windows upgrade/uninstall testing PASS on the hosted runner. Its evidence binds
the actual candidate Setup hash below and published 1.0.0 baseline hash
`670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3`.
Both installed candidate payload hashes matched; the stable AppId and single
registration were retained. Uninstall removed the bundle/registration and
preserved synthetic USER/document bytes. This is not an installation on the
owner's machine, a playable USER-bank test or host/audio acceptance.

## Independent downloaded file inspection

All three outer ZIPs match the GitHub API digests. The complete artifact has
exactly seven checksummed files plus SHA256SUMS.txt. Four-product and separate
two-file source staging match the complete artifact byte for byte. The seven
inner checksums PASS. The independent checker also rejects changed bytes,
extra unlisted files and duplicate checksum entries in three negative controls.

| Artifact | ID | Outer ZIP SHA-256 |
| --- | --- | --- |
| Complete validation | 11327896442 | `55948a51e798f5d07e50ab75337c276148974292825291001b07e4ceed56896b` |
| Four downloads | 11327832563 | `702b87c85997afe5c2109c439898b463e3c9930ee8540a18c17d6a2cdbba0bdf` |
| Separate source | 11327887389 | `5a43a63f733729f675afc0741bcaeb499d94cd2a91fd87e4bcb8090d210a6c27` |

| Preparation product file | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `c0631bf258637ad65de84c1bab04743a8f4b576e635dd5f92dd37bc4adbb20b7` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `e9248897c3946514843501767eaa99611639ba905b46575787fe26925bc9b890` |
| `VDX7-1.0.1-macOS-universal.pkg` | `4cf632f88e1e7f979c1807a84a203efa85eb9e44bec6e1f7e6f4f27da44c4c41` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `11864152a622e2c8d701f7f6d48aa08ed082757964cd445f3c9461ffd3980945` |

Both BUILD-INFO records bind source, packager and preparation workflow to the
exact SHA above, run 37269260691, attempt 1, and explicitly say preparation,
not accepted or approved for publication. The source manifest is likewise
`release_accepted=false`, with no new approval proof. These hashes cannot be
substituted for the actual later accepted-mode files.

## Binary and installer payload boundaries

Both Manual ZIPs contain only VST3 bundles and normal Mac ZIP metadata, no AU,
Standalone, ROM or SysEx bank. Windows plugin PE architecture is x86-64. The
actual extracted hosted Windows Manual plugin passed the separately built
Steinberg VST3 SDK 3.8.1 validator: 47 tests passed, zero failed, plugin version
1.0.1, instrument/synth category. This run used no firmware or REAPER. Validator
acceptance does not claim audible compatibility or supported 64-bit processing;
unsupported double precision is handled as such by the validator.

The Mac Manual binary contains arm64 and x86_64, version 1.0.1 and bundle ID
`org.vdx7.prototype`. Independent XAR/CPIO inspection finds exactly the five
Manual bundle files under `Library/Audio/Plug-Ins/VST3/VDX7.vst3` in the PKG,
with identical bytes and no extra payload. PKG ID is `com.robczart82.vdx7.vst3`,
version 1.0.1 and install root `/`. The PKG has no package signature. Native
strict ad-hoc plugin signature and Universal architecture checks PASS in the
hosted macOS job; local Windows cannot repeat codesign or macOS installation.
Windows Setup Authenticode status is NotSigned. Neither package is Developer ID
signed or notarized; security warnings remain a documented limitation.

Hosted Windows records MSVC 19.44.35229.0, SDK 10.0.26100.0, CMake 3.31.6,
Inno 6.7.1. Hosted macOS records Xcode 15.4, AppleClang 15.0.0.15000309,
SDK 14.5, CMake 4.4.3, deployment target 11.0. No bit-reproducible binary claim
is made. Node 20 action deprecation and the future ubuntu-latest migration
appear as maintenance notices, not failed tests; they require a separately
validated tooling update if changed.

## Corresponding source and local execution

Source archive:
`VDX7-1.0.1-96267f7304e657821ce35c54536689981f41ef27-corresponding-source.zip`.
SHA-256: `68c5f39b05ab2829c52aa38788cdd955ab7e166bc582e49cddbfa51820eae5df`.
The hosted ZIP is byte-identical to the independently created local ZIP.
Current and bundled checkers PASS for all 5,131 manifest files. The bundled
checker also PASS with Git absent from PATH. All 290 included repository
wrapper files match their immutable A Git blobs. License notices and pinned
offline dependency sources are present; the payload checker rejects proprietary
firmware/bank inclusions.

Pinned JUCE: `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`.
Pinned Retromulator: `d5473776a0449d60a997b91bdc888598a33265ac`.
A new extraction and build directory use the bundled sources, stable 1.0.1,
Windows x64 Release, `FETCHCONTENT_FULLY_DISCONNECTED=ON` and ROM tests OFF.
Offline configure PASS with MSVC 19.44.35229 and SDK 10.0.26100.
Offline VST3 and CI-target compilation PASS. The fresh extracted-source build
passed 14/14 ROM-free CTests in 4.67 seconds. Its actual 14-test JSON inventory,
labels, fixtures and timeouts passed the registration checker and its nine
negative controls. This local stable source build is not the hosted binary,
private-ROM coverage or a host/audio acceptance result.

Local Python suite: 76 PASS and one symlink-capability SKIPPED in 71.625 seconds.
The first sandbox run failed because the default temporary directory was
inaccessible. A scoped temporary directory corrected that issue, but the
hardlink probe was still denied by sandbox permissions. The unchanged suite
passed on rerun outside that sandbox; the symlink limitation remains explicitly
SKIPPED. The initial sandboxed compilation stalled and was interrupted; only
this task's process tree was stopped, then compilation was restarted with the
scoped temporary directory outside that sandbox. No source repair was applied
to hide either environment issue.

## Remaining owner and publication gates

NOT RUN: installed-package acceptance on the owner's computer, REAPER,
audible ROM behavior, final host/buffer/sample-rate/automation/multi-instance
matrix, physical Intel Mac coverage and local macOS signature/installation.
Earlier dbad14a or 1.0.0 host results do not become PASS for this candidate.

Next request: explicit owner approval for final test packaging of exact
A=B `96267f7304e657821ce35c54536689981f41ef27`. Only then update the approval
record in a separate protected PR, wait for exact-head/main gates, run accepted
mode and inspect its actual final downloads again. The owner tests those final
files or explicitly disposes of each unrun host check. Separate authorization
is still required for source/product tags, Releases and uploads.

Preparation downloads:
[four products](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37269260691/artifacts/11327832563)
and [matching source](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37269260691/artifacts/11327887389).
These are temporary Actions artifacts requiring possible GitHub sign-in, not
durable public releases or final accepted packages. No local installed plugin,
private original ROM, tag, Release or public asset was changed.
