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
