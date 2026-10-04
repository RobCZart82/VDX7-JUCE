# Final bank folder package verification for 1.0.1

The new bank-folder candidate's exact-pair approval, protected merge, post-merge
main tests, hosted final packaging and independent downloaded-file inspection
all passed. The exact files below are ready for the owner's final Windows and
macOS REAPER tests. No private-machine installation, real host acceptance, tag,
Release or public asset change has been performed.

## Exact source tooling and approval

- Product A and frozen packager B:
  `dbad14a2ef8307e565675893b6b3b837cfab9b02`.
- Reviewed policy PR [126](https://github.com/RobCZart82/VDX7-JUCE/pull/126)
  head: `cc8a2bad67561997390d0813e6f563ebb06c0704`.
- Protected main approval/workflow C:
  `3766035aa42de22e581c1b2014edeeecbbff01f9`.
- Exact committed policy SHA-256:
  `29549ff421f8f16fe754bcc97c11fdc61c70a5ea5b18c267579df60ea4fd6cd6`.
- Canonical workflow:
  `RobCZart82/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@refs/heads/main`.

The owner authorized continuing after the handoff identified this exact A/B
final-test-build gate. The policy change does not claim real host acceptance or
permission to publish. The older b7fce05 approval and packages are historical,
not the new candidate's evidence.

PR-head Windows `37231799259`, macOS `37231799262` and ASan/UBSan
`37231799268` all passed. Both reviews completed without findings for that
head and there were no unresolved threads. Post-merge main Windows
`37232572296` and macOS `37232572281` passed before the canonical final
workflow was dispatched. No product, packager, installer or workflow was
changed by PR 126.

Final test packaging:
[37233357714](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37233357714).
Its exact head is C above. Authorization, both native platform jobs and assembly
all passed. macOS passed 14/14 ROM-free tests in 2.89 seconds and 77/77 Python
tests in 7.446 seconds; Windows passed 14/14 in 2.72 seconds and 77/77 Python
tests in 25.201 seconds. These are executed tests, not registration-only smoke.

## Fresh corresponding source build

The local accepted-mode source ZIP contains 5,128 files and verifies against
the current and bundled checkers, including the exact A/B/C policy binding.
Archive: `VDX7-1.0.1-dbad14a2ef8307e565675893b6b3b837cfab9b02-corresponding-source.zip`.
SHA-256: `1f842657c9c88e786239c2dcd0ef7fcc74b71f762ecb49f6f4bdf4769cfec33d`.
The independently downloaded hosted source ZIP is byte-identical to this
freshly extracted and tested archive. Every committed wrapper file was also
compared to its exact A Git blob. The manifest's C identity and policy digest
match the committed policy, not an inferred preparation approval.

A new extraction and new build directory used bundled pinned JUCE/Retromulator,
`FETCHCONTENT_FULLY_DISCONNECTED=ON`, Release, stable 1.0.1, arm64, deployment
target 11.0 and ROM test registration OFF. CMake 4.4.3, Ninja, AppleClang
21.0.0.21000334 and SDK 27.0 completed the VST3/CI build. The initial nested
helper build failed because Ninja was not on PATH; supplying the observed Ninja
directory corrected the environment only. No source file or archive was altered.
Normal JUCE intermediate bundle-signature replacement was followed by explicit
final ad-hoc signing and strict verification PASS.

The new extraction passed 14/14 ROM-free CTests in 7.43 seconds and 77/77 Python
tests in 13.400 seconds. The bundled source checker passed with no Git on PATH.
The focused private eight-bank test passed content identities, partial folder,
latest factory edits, absent/changed local catalog recall and CUSTOM recall.
Private input files stayed local and unchanged; temporary fixtures were isolated.
This is a local arm64 build and processor probe, not the hosted Universal/Windows
binary, physical Intel Mac coverage, audible fidelity or real REAPER acceptance.

## Independently downloaded final files

The GitHub API downloads matched all three outer artifact digests. Complete
validation retained seven checksummed files plus the checksum manifest; the
four-download and two-file source staging contained exactly their allowlists
and matched those complete files byte for byte. All seven inner checksum entries
passed, including both BUILD-INFO records.

| Artifact | ID | Outer ZIP SHA-256 |
| --- | --- | --- |
| Complete validation | 11314657977 | `60c927c8cdcad165095e266fabee57e62799ba5db3c6dc86de28ff3996490a6a` |
| Four downloads | 11314563227 | `3866a5bf165cd779e0cd7e0f5897c8726d847185613fe4a5055157867f6345e7` |
| Separate source | 11314583189 | `2e1ebe48d2456fe3fee8a1d3b53e04700ac49496bb38fca86dc86389f2b011be` |

| Final product file | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `9e49b095becc63a7e80f480aafac569b9367c2b8d6bfab6ef91ec5916127732d` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `a352afd62d58a0efabad278a98566f71a0ce11b6946f2a29f0f6ad11607488fd` |
| `VDX7-1.0.1-macOS-universal.pkg` | `7bd7069681e9689aee29683b78b8551c16962d259d7c9492c27e00f1f609c1b9` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `5226479a4a21449f7b82a56d5f9cfdc55ba96505cf6cb1f004ffe831d12eefb9` |

BUILD-INFO Windows SHA-256:
`6ed5bb7c8cae0f983afd0814b74164be330a1b912baa8e60f3ebe261a0bfb4b7`.
BUILD-INFO macOS SHA-256:
`2e2b1f8fd2f5063ac758a25d2a9c1a0fa99e1d7a7dc2462043e837039410b8a4`.
Both records bind A/B/C, the canonical workflow and the actual run above.
Preparation `37225128633` and the older b7fce05 files remain historical only;
none of their product hashes was substituted for these actual final downloads.

## Native installer and payload inspection

Both Manual ZIPs contain only the VST3 bundle and normal ZIP metadata, without
AU, Standalone, firmware or SysEx bank files. Windows Manual contains PE x86-64.
Hosted native upgrade/uninstall evidence binds the exact Setup.exe hash above,
the published 1.0.0 baseline hash
`670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3`,
two installed candidate payload hashes, stable AppId and one registration.
The upgraded uninstaller removed the plugin/registration and preserved synthetic
USER/document bytes. This is not local Windows installation, playable USER-bank
coverage or host/audio acceptance.

The Mac Manual contains arm64 and x86_64, stable version 1.0.1 and identifier
`org.vdx7.prototype`; strict ad-hoc signature verification passed. Every expanded
PKG plugin file matches the Manual, and the entire PKG payload contains exactly
that bundle under `/Library/Audio/Plug-Ins/VST3/VDX7.vst3`, with no extra payload.
PKG identifier is `com.robczart82.vdx7.vst3`, version 1.0.1, install root `/`.
The PKG is unsigned and not notarized; no Developer ID claim is made.

Observed hosted toolchains: Windows MSVC 19.44.35229.0, SDK 10.0.26100.0,
CMake 3.31.6, Inno 6.7.1; Mac Xcode 15.4, AppleClang 15.0.0.15000309,
SDK 14.5, CMake 4.4.3 and deployment target 11.0. Dependency pins remain JUCE
`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator
`d5473776a0449d60a997b91bdc888598a33265ac`. These observations do not claim
bit-reproducible installers or physical Intel Mac coverage.

## Owner test handoff and remaining publication gates

Download the [four final test files](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37233357714/artifacts/11314563227)
and [separate matching source](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37233357714/artifacts/11314583189).
These are GitHub Actions artifact ZIPs and may require GitHub sign-in; unpack
the outer ZIP first. They are retained test artifacts, not durable public release
downloads. The source artifact has its ZIP and one-entry SHA256SUMS.txt.

The owner installs and performs real Windows/macOS REAPER acceptance with the
[HU/EN checklist](../release/RELEASE_CHECKLIST_1.0_RC.md).
Bank-folder identities, latest factory/CUSTOM edits, absent/changed-folder
project recall, ROM/controller/audio behavior and multi-instance results must
be recorded for these exact files. Earlier 1.0.0 or b7fce05 host results are not
carried forward. Unrun checks require explicit disposition, not assumed PASS.
Publication and durable matching-source delivery still require a separate
owner decision under the [single execution plan](../release/EXECUTION_PLAN_1.0.md).
