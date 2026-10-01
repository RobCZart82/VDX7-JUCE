# Coordinated 1.0.1 candidate preparation — 2026-10-01

Baseline: main `01c54c2e4709e97a69a67fffbbc257647a03688e`, after PR #111.
The owner requests corrective development toward 1.0.1 publication and merging
verified green PRs. This round prepares version identity, not final acceptance.

## Changes and preserved behavior

Project version, Windows installer version/name, macOS package version and
stable/candidate workflow labels move together to 1.0.1. Normal builds retain
the development suffix; stable preparation remains explicitly not accepted.
The Windows output basename derives from AppVersion. AppId, install directory,
plugin identity, parameter IDs/order, state format, DSP and GUI are unchanged.
Published v1.0.0 and its assets are untouched.

New 1.0.1 authorization checks project/installer versions in immutable source
Git objects before platform work. Source archive creation repeats the check
before output. Old source cannot simply be relabelled 1.0.1. The supported
labels are explicitly limited to 1.0.0/1.0.1 and their dev/rcN forms; accepted
mode still requires stable labels and the exact committed approval tuple.
The default source-package label is 1.0.1-dev. Historical 1.0.0 archives remain
integrity-verifiable; verification is not publisher authentication.

## Executed checks

Local macOS, Python 3.9.6:

- PASS: 57/57 Python tests, no skips. This includes the existing approval,
  checksum, dependency/toolchain and four-download regressions.
- PASS: 1.0.1 preparation rejects old project or installer versions; creation
  rejects mismatches without creating output.
- PASS: separate exact 1.0.1 approval tuple and synthetic accepted archive
  verify with their bundled checker without Git/network access. These are
  disposable test fixtures, not an actual product approval.
- PASS: 1.0.0 backwards compatibility, 1.0.1 dev/rc/stable identities and
  unsupported version/rc0/rc01/newline/path negative controls.
- PASS: CMake display identity for 1.0.0 and 1.0.1, including development,
  candidate and stable forms; invalid combinations fail.
- PASS: stable/candidate workflow actionlint 1.7.12 checks, with optional
  shellcheck/pyflakes integrations disabled; whitespace check.
- PASS: fresh local Ninja Release build of `vdx7_ci_checks` with ROM tests
  disabled, AppleClang 21.0.0.21000334; 13/13 executable CTests passed.
  Generated project version is 1.0.1 and display version is 1.0.1-dev.
- PASS: actual complete preparation source archive from
  `6e638f1b47d895d030d2ee506c336aab6776468c`, with pinned JUCE/core Git
  objects: 5,108 files; SHA-256
  `6feeced2a439fc0cc5dcefd68dd624eaf2cff0065ebc9bf3fcdbf5e48fc03df5`.
  Its extracted standalone checker verified it with no Git executable on PATH.
  This is local source-preparation evidence, not a final release asset.

## Remaining release gates

Final-head Windows/macOS/ASan-UBSan checks and post-merge main Actions are
pending. The native stable installer workflow has not run for this candidate;
actual installer/toolchain capture, source archive and four-download staging
must be checked after a green merge. A5 upgrade/uninstall evidence, durable
matching-source access, exact-candidate host/private-ROM acceptance or explicit
deferral, separate frozen-tool approval and final hashes remain open.

No installed plugin was replaced, no REAPER session was modified and no
release/tag/assets were published. The committed approval record stays null.
The [execution plan](../release/EXECUTION_PLAN_1.0.md) remains the sole ledger.

## Review follow-up

PR #112 review identified stale active candidate/source instructions in
NOTICE, the HU/EN candidate guide and the source-packaging example. They now
use 1.0.1 forms, with a regression checking those active instructions.
After this follow-up, the complete local Python suite passed 58/58 with no
skips; workflow syntax and whitespace checks passed again.
Historical 1.0.0 validation records and actual 1.0.0-dev screenshot captions
remain unchanged. The identity guide separates current 1.0.1 instructions
from historical 1.0.0 evidence.

The owner selected a separate v1.0.1-source GitHub release for durable source
access, not source inside the Manual ZIPs. That source release will not be
latest; the main v1.0.1 remains latest with four user downloads. This decision
does not approve the final candidate or create a release in this round.

## Merge and private-ROM regression closure

PR #112 final head `15f11830e432018809630d2d9cce6085c7103ef6` passed Windows
`36907192768`, macOS `36907192756` and ASan/UBSan `36907192754`; the
review conversation was resolved after the documented correction.
It merged as `d4f589764284e72a2d532a6055ac8d918cb53d9c`.
Post-merge Windows `36908721471` and macOS `36908721498` both completed
successfully at that exact merge commit, before the next round was submitted.

The fresh 1.0.1 development harness also passed 43/43 executable CTests using
the private local ROM in 152.10 seconds, parallel 4. Of 44 registered tests,
interactive `vdx7_processor` save-dialog coverage was explicitly NOT RUN.
No ROM data, installed plugin or REAPER state was changed/uploaded. This
closes source-level regression evidence, not final stable binary/host acceptance.
