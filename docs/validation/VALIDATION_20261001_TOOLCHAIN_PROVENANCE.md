# A6 toolchain evidence — 2026-10-01

Baseline main: `e3f87eae65d76c718e307180f5eb74abbe7f2662` after PR #109.
Branch: `codex/1.0.1-toolchain-provenance`.
Pull request: [#110](https://github.com/RobCZart82/VDX7-JUCE/pull/110).
Implementation commit: `a9998a20b1aae76c23503521cd842ab321980b3c`;
the subsequent PR-link ledger update changes documentation only.
Scope: packaging-only Inno pin and observed compiler/SDK/CMake/runner/dependency
evidence, plus completion of the already merged A4 ledger. Runtime sources,
parameter identities, GUI, project format, product versions and package labels
are unchanged. No ROM, installed-plugin replacement, REAPER, tag, Release or
asset mutation. Committed release approval remains null.

## Baseline negative control and implementation

The new workflow contract fails on the baseline: Inno selection is floating,
the actual ISCC version is unchecked, and platform BUILD-INFO lacks observed
compiler/SDK/runner metadata. Six existing workflow contracts still pass.
This is an A6 provenance gap, not a reproduced synthesis or host failure.

The modified workflow selects/validates Inno 6.7.1, checks installation status,
and appends collector output to both existing hashed BUILD-INFO files. The
seven-asset inventory, exact source/tooling/approval checkout boundaries and
read-only no-publication permissions are retained. The collector verifies
dependency pins/clean tracked sources and rejects absent/ambiguous metadata.
It does not copy tool binaries, paths, firmware or the entire environment.
See the [mechanism and limitations](../release/BUILD_TOOLCHAIN_PROVENANCE.md).

## Executed checks

Local Windows, Python 3.13.3; synthetic toolchain/SDK/compiler fixtures only.

| Check | Result / scope |
| --- | --- |
| Full Python suite | PASS: 42 executed tests; 43 discovered, one capability-limited checksum symlink test skipped (WinError 1314). Not 43 executed PASS. |
| New collector tests | PASS: 8/8; Windows SDK/compiler/hash, configured macOS SDK and Universal targets, missing runner/compiler data, ambiguous compiler/SDK, wrong Inno/architecture, dependency identity/edits/missing pins, preservation of previous evidence on failed capture. macOS metadata is simulated, not a native macOS execution. |
| Workflow contracts | PASS: 7/7 after change; baseline negative control FAIL as expected on the new A6 contract. |
| Existing source/approval/checksum regressions | PASS except the explicit Windows symlink skip; real temporary Git fixtures and offline verifier exercises retained. No product installer created. |
| Workflow syntax | PASS: upstream actionlint 1.7.12, tool archive checked against upstream SHA-256 manifest; external shellcheck/pyflakes disabled. |
| Python 3.9 grammar | PASS via AST feature-version check; not a Python 3.9 runtime execution. |
| Patch whitespace | PASS: git diff --check. |

The first sandboxed collector test attempt could not create/read its temporary
fixture directories (WinError 5). Re-running with permitted temporary-file
access passed; this was an environment restriction, not a collector failure.

## NOT RUN / remaining gates

- Final-head Windows/macOS/ASan-UBSan PR gates and merge: pending at this local
  record. Do not infer these from baseline main's green checks.
- Actual native platform collector capture, pinned Inno installation/compiler
  version-resource verification, installer build and full stable workflow.
- A new C++/plugin build or host test in this packaging-only round; runtime
  files are unchanged. The earlier 2026-10-01 review's 19/19 targeted Windows
  CTests belong to the baseline review, not a fresh A6 execution.
- Final 1.0.1 label/version changes, A5 migration, A7 payload/hash/license
  reconciliation, exact-candidate binary acceptance and publication approval.

A6 implementation is locally tested, not yet accepted on real candidate
assets. The workflow remains 1.0.0-labelled and must not be used to overwrite
published assets. No new accepted packaging was dispatched. Observed versions
do not freeze all tools/transitive components or guarantee bit-identical output.
The [single execution plan](../release/EXECUTION_PLAN_1.0.md) keeps these gates
open. A separate publication request is still required.

## Final PR and merge closure

PR [#110](https://github.com/RobCZart82/VDX7-JUCE/pull/110) is merged at
`98ad6797b178035539962c710516c119ed1213a6`. On final head
`4e3db20e8d4f40a14417cf19eb5fc3649366d324`,
[Windows](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36844542776),
[macOS](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36844542886) and
[ASan/UBSan](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36844542494)
PASS. Post-merge
[Windows](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36846727543) and
[macOS](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36846727561) PASS.
These close the pending PR gates, not actual installer workflow/collector
capture or final-candidate acceptance. No published assets were changed.
