# Release approval guard — validation 2026-10-01

Baseline: main `ff1df3619824cc8d1642869216b19280ad88e8d0` after PR #108.
Development branch: `codex/1.0.1-release-provenance`.
Scope: AUDIT-20260930-A4 only, with the reproduced Git-identity edge case.
No runtime/GUI, parameter identity, project-format or installed-plugin change.
Published 1.0.0 tag and assets are unchanged; no new release is approved.

## Reproduced gap

The baseline packager accepts a syntactic source/tooling identity and the
accepted flag without checking a committed reviewed release record. An
isolated synthetic source/dependency fixture creates an accepted archive
while its committed policy is null; the baseline verifier accepts its
integrity. This proves a missing provenance guard, not unauthorized publication
or a compromised GitHub account. No real product release or ROM was used.

A second fixture demonstrates local Git replacement objects changing policy
contents while the original commit SHA is still reported. Ordinary baseline
snapshot reads see the substituted approved policy; the guarded reader sees
the original null policy. Replacement ancestry can also forge a foreign
parent relationship. Guarded authorization rejects the substituted identity
and forged ancestry by reading real Git objects with replacement disabled.

## Implementation

- Strict schema, boolean types and full commit identities; duplicate JSON
  keys, missing/null policy and unknown/mismatched fields fail closed.
- Separate product A, frozen packager B and approval/workflow C identities;
  real ancestry and exact approval checkout are required. B cannot equal C.
- Exact reviewed source/tooling pair and canonical main workflow context
  required for accepted mode; preparation is not granted accepted status.
- Authorization runs before the platform matrix. Validated identities flow
  through checkouts, build metadata, source packaging and asset assembly.
- Accepted creation rejects before archive output. CLI output is written only
  after successful validation; failed guards do not change an existing output.
- New manifest approval fields bind the pair, context and policy digest.
  Offline verification checks internal consistency, not publisher identity.
- Legacy schema-1 integrity verification and the newer bundled verifier for
  older product snapshots remain compatible. No Git/network is needed by the
  extracted checker.

The default policy is null. No approval SHA pair has been entered and no
accepted packaging workflow has been dispatched.

## Executed local checks

| Check | Result and scope |
| --- | --- |
| Python package/approval/checksum/workflow suite | 34/34 PASS: 10 existing package tests, 16 approval regressions, 2 checksum tests, 6 workflow contract tests |
| Approval regressions | Real temporary Git source/tooling/policy commits; exact tuple, dirty/untracked policy, wrong refs/SHAs/types, off-branch identities, same-script/different-tool SHA, replacement commit/blob/ancestry and failed-output controls |
| Offline bundled checker | PASS on a generated old-product/new-tooling archive without Git on PATH or a `.git` directory; fixture only, not a release asset |
| Workflow regression negative control | Baseline: 5 contract failures and 1 retained-scope pass; updated workflow: 6/6 PASS |
| Git replacement negative controls | Baseline substitution/forged ancestry reproduced; guarded real-object checks reject them |
| Workflow syntax | Ruby Psych YAML parser PASS; upstream actionlint 1.7.12 PASS with external shellcheck/pyflakes disabled; downloaded tool archive checksum verified |
| Existing ROM-free CTest executables | 13/13 PASS; unchanged runtime/build from the preceding verified round, not a newly rebuilt product |
| Registration contract | PASS, including seven negative controls, against the unchanged generated test inventory |
| Patch whitespace | `git diff --check` PASS |

Tests ran locally on macOS with Python 3.9.6 and Git 2.54.0. The Python suite
uses only the standard library. Synthetic fixture revisions are not approved
product revisions. The guard is not a signature; protected-main review and
separate publication approval remain necessary.
Independent final read-only review found no concrete blocking defect in the
accepted/preparation boundary, workflow propagation or legacy offline checker.
This review is separate from test execution and remote CI.

## Not run / separate gates

The final-head Windows/macOS and ASan/UBSan PR checks remain required before
merge. Actual stable workflow dispatch, new installer builds or host/audio
acceptance were NOT RUN in this local round. No REAPER launch, installed
plugin replacement, private-ROM test rerun, tag, Release or asset mutation.
Prior runtime/ROM evidence remains dated in the
[deep-fix validation](VALIDATION_20260930_DEEP_AUDIT_FIXES.md), not relabeled as
a new A4 execution. A5/A6/A7 and exact-1.0.1 acceptance remain open in the
[single execution plan](../release/EXECUTION_PLAN_1.0.md).
