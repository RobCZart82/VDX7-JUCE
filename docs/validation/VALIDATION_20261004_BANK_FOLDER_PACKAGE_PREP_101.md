# 1.0.1 bank folder package preparation

The new candidate contains the legacy bank-acceptance correction, content-identified
factory bank folder, durable project catalog and bounded project-data decoder.
The four new packages and complete corresponding source have passed preparation
and independent inspection. They are not the earlier b7fce05 files, accepted-mode
final artifacts, real REAPER acceptance or a published release. The old approval
record and published v1.0.0 remain unchanged.

This preparation checkpoint is retained as history. The subsequent exact-pair
policy, final accepted-mode build and independent final-file verification are
recorded in the [new final report](VALIDATION_20261004_FINAL_BANK_FOLDER_PACKAGE_101.md);
its hashes, not the preparation hashes below, identify the owner test handoff.

## Exact source and automated evidence

Product source and packager: `dbad14a2ef8307e565675893b6b3b837cfab9b02`.
PR 124 final source head `9ee19fa71fb77066e2c73baf8252e6faad243981` passed
Windows `37221380085`, macOS `37221380183` and sanitizer `37221380004`.
Code and security reviews completed for that head without findings or unresolved
threads. The merge introduced no further product changes. Post-merge main
Windows `37222243363` and macOS `37222243364` also passed.

The rebuilt local full regression was 45/45 PASS, required ASan/UBSan components
9/9 PASS and Python suite 77/77 PASS. Private bank-folder processor checks passed
normally and under ASan/UBSan. Leak detection was disabled on the local macOS
configuration; these results do not claim a full private-ROM sanitizer run.
See the [feature evidence](VALIDATION_20261004_FACTORY_BANK_FOLDER.md).

Preparation workflow [37225128633](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37225128633)
ran from this exact main commit with `accepted_for_publication=false`.
Authorization, Windows, macOS and assembly all passed. Each native platform
passed 14/14 ROM-free tests and 77/77 Python tests. Windows additionally passed
the actual published-1.0.0 installer upgrade/uninstall and synthetic user-file
preservation checks. No local installed plug-in or REAPER session was touched.

## Exact corresponding source and fresh offline build

Source archive:
`VDX7-1.0.1-dbad14a2ef8307e565675893b6b3b837cfab9b02-corresponding-source.zip`.
SHA-256: `ff3020e41d760ee3f1c8e826f5fe04344b301022f976219384f564cd9a66c14e`.
The current and bundled checkers verified 5,128 source files. Every committed
wrapper file matched its exact Git object. JUCE is pinned to
`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator to
`d5473776a0449d60a997b91bdc888598a33265ac`.

The separately generated local ZIP and downloaded hosted ZIP are byte-identical.
A new extraction and new build directory used bundled dependencies with
`FETCHCONTENT_FULLY_DISCONNECTED=ON`, stable 1.0.1, Release, arm64 and deployment
target 11.0. Local CMake 4.4.3, AppleClang 21.0.0 and SDK 27.0 completed the
180-step VST3/CI build; 14/14 CTests passed in 7.00 seconds and 77/77 Python tests
passed in 17.560 seconds. The private eight-bank, latest-edit and absent-file
project-recall probe also passed from this new extracted-source build.

The bundled verifier passed with no Git on PATH. The initial verifier command
used a nonexistent guessed Python path; using the observed system Python path
passed. The offline dependency configuration and compilation succeeded without
fetching sources. Normal JUCE bundle generation replaced its intermediate
signature; explicit final ad-hoc signing and strict verification then passed.
This local arm64 build is not the hosted Universal or Windows binary, and none
of these checks is physical Intel Mac or real REAPER evidence.

## Independently downloaded artifacts

The GitHub API downloads matched the API's SHA-256 digests. Exact four/two-file
staging matched the retained complete validation artifact byte for byte.
All seven inner checksum entries, including both BUILD-INFO records, passed.

| Artifact | ID | Outer ZIP SHA-256 |
| --- | --- | --- |
| Complete validation | 11311986880 | `496157e6f4a8337fc5eadf5318a4804733cd32fa88d3eca664b207600859d3a9` |
| Four downloads | 11311623345 | `34a8c230bc0f2e0098a60c75137b69f56a7bf73256d9487ec0679dcbd7c9b361` |
| Separate source | 11311777365 | `dbc4ce65bf299743cc58be95c6f48775e00b874b448d27cee7d755e68d8ae9ef` |

| Preparation product file | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `d35b571bcf7b4cc67b9cc4ca940b69bde6d002e236acb754f6f3cbfc4d32ea33` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `f5491aa1b4b006c44b3af9dd6bc9cc80c923e6108536709b885842c20b895b94` |
| `VDX7-1.0.1-macOS-universal.pkg` | `c79467d79187bb2d0eec7391d7d0b05bfdfc0e5d88f1c8f06e2d5003e6dd5a1f` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `4099fed7e4a4080897b31548b2daf47696b652763fd80955e405c1fdf1be165c` |

Both BUILD-INFO files bind source, packager and preparation workflow to dbad14a
and explicitly say preparation, not accepted or approved for publication.
The source manifest likewise has `release_accepted=false` and no new approval
proof. These preparation hashes cannot be copied into an accepted-mode report
without downloading and checking the actual later files.

## Native payload boundaries

Both Manual ZIPs contain only the complete VST3 bundle and normal ZIP metadata;
no AU, Standalone, firmware or SysEx bank is included. Windows Manual contains
PE x86-64. Hosted upgrade evidence names the exact Setup.exe hash above, verifies
both installed candidate payload hashes and one stable AppId/registration, removes
the plugin/registration on uninstall and preserves synthetic USER/document bytes.
This is hosted Windows installer evidence, not a local Windows installation,
playable-bank or host/audio test.

The Mac Manual contains both arm64 and x86_64, stable version 1.0.1 and
`org.vdx7.prototype`. Strict ad-hoc signature verification passed. Every expanded
PKG plugin file matched the Manual bundle. PKG identifier is
`com.robczart82.vdx7.vst3`, version 1.0.1, with payload under
`/Library/Audio/Plug-Ins/VST3/VDX7.vst3`. The package is unsigned and not notarized;
no Developer ID claim is made. Hosted toolchains record MSVC 19.44.35229.0,
Windows SDK 10.0.26100.0 and Inno 6.7.1; macOS uses Xcode 15.4,
AppleClang 15.0.0.15000309 and SDK 14.5.

## Remaining gates

The new exact source/packager pair still requires owner approval for final test
packaging, a separate reviewed committed approval record, green PR and post-merge
main gates, then an accepted-mode build and inspection of its actual files.
The b7fce05 approval is historical evidence for that pair only; it does not approve
dbad14a. No policy is silently rewritten. The owner performs real Windows/macOS
REAPER acceptance on the final verified files. Separate publication permission
and durable matching-source publication follow, never precede, those gates.

The [single execution plan](../release/EXECUTION_PLAN_1.0.md) remains authoritative.
No previously invalid combined-ROM test is relabelled PASS, no omitted host test
is implicitly deferred, and no old package/host acceptance is reused for this
changed product.
