# 1.0.1 corresponding source build verification

Preparation source/tooling: `b7fce0503059c8c2e3c61d6ccf5af33612739e32`
after merged #118. This is a candidate identity, not an approved publication
tuple. Main Windows `37046632292` and macOS `37046632237` both PASS;
#118 final-head Windows/macOS/ASan-UBSan checks also PASS.

## Exact source archive

Created the stable-labelled, non-accepted source archive directly from this
Git snapshot and pinned dependency objects, not the working tree. It contains
5,120 files. Source and packager identities both match the SHA above.

Archive: `VDX7-1.0.1-b7fce0503059c8c2e3c61d6ccf5af33612739e32-corresponding-source.zip`.
SHA-256: `4bc1ac989d4d5372b580fa8f8718eaeb6ca7d10c2a78f75cb37f0f2f302afc98`.
Current and extracted bundled checker PASS; the bundled check used
`PATH=/nonexistent`, without Git or network access.

## Fresh archive build

Extracted into a new directory without Git metadata. Configured a fresh build
with CMake 4.4.3, Ninja, AppleClang 21.0.0.21000334, Release,
`VDX7_RELEASE_BUILD=ON`, `VDX7_ENABLE_ROM_TESTS=OFF`, and
`FETCHCONTENT_FULLY_DISCONNECTED=ON`. Configuration resolves JUCE and dx7Lib
from the archive's `third_party` directories; no external dependency checkout
or network fetch is used. Compiler, SDK, CMake, Ninja and Python are still
required local build tools; they are not supplied by corresponding source.

- PASS: fresh `vdx7_ci_checks` build from extracted archive.
- PASS: 13/13 ROM-free CTests, 5.15 seconds, parallel 4.
- PASS: 77/77 Python tests from extracted archive, no skips, 10.146 seconds.
- PASS: focused extracted-source `--bank-only` processor regression with
  private local firmware and synthetic factory banks, covering warning,
  rejection, preservation, export and reopen. No firmware belongs to the ZIP.
- PASS: `VDX7_VST3` build and manifest helper from the archive, stable 1.0.1.
  This local native build is not the hosted Universal/Windows release binary.
- PASS: checkout Python suite 77/77, no skips, 10.418 seconds.

The first CTest invocation used an incorrect relative build path and did not
run tests; the corrected invocation produced the recorded 13/13 result.
The first VST3 target invocation could not locate Ninja in its nested JUCE
helper environment. Retrying with the existing Ninja directory on PATH passed;
no source modification or network installation was needed. Compiler warnings
were not promoted into claims of a clean warning-free build.

The initial local VST3 signature check failed after moduleinfo generation,
because generated resources changed after JUCE's automatic signing step.
Applying the same explicit final ad-hoc signing/strict verification used by
the native preparation workflow passed. Only the generated local build bundle
was signed, not the installed plugin; this is not Developer ID/notarization.

`COPY_PLUGIN_AFTER_BUILD` is FALSE. No system plugin was replaced or host
launched. The build does not establish final host/audio acceptance, Windows
source-build verification, physical Intel Mac support or bit-reproducibility.
The full original/constructed-control private-ROM suites and interactive
save-dialog test were not repeated here; previous boundaries remain explicit.

## Parallel native preparation

Dispatched canonical main workflow
[37051589142](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37051589142)
for the same full source SHA, `accepted_for_publication=false`. Authorization
passed and native Windows/macOS packaging is in progress at this checkpoint.
Do not count this run or its eventual assets as PASS until completion and
independent download/hash inspection. Prior run `36998278475` is different
source/archive identity, although runtime/tooling bytes are unchanged.

## Owner disposition and remaining boundary

Owner explicitly deferred the detailed ROM diagnostic refinement to the next
corrective release on 2026-10-02. Larger optimization remains measurement-led.
Current strict combined-ROM rejection and state preservation remain unchanged;
this deferral is not acceptance of final binaries or legacy-project risks.

The source archive build gate is now supported by fresh exact-candidate
evidence. Remaining work is native preparation completion and final payload
review, exact host acceptance or explicit deferrals, source/tooling approval
in separate C, accepted artifact generation/reinspection, final HU/EN notes,
durable corresponding-source delivery and publication authorization.
`RELEASE_APPROVAL.json` remains null. No tag, release or public asset changed.
See the [single plan](../release/EXECUTION_PLAN_1.0.md).
