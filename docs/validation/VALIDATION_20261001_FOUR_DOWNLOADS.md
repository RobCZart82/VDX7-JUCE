# Four-download publication layout — 2026-10-01

Baseline: main `98ad6797b178035539962c710516c119ed1213a6`, after #110.
Branch: `codex/four-release-downloads`.
Owner requires four future user downloads: Windows x64 EXE + Manual Install
ZIP; macOS Universal PKG + Manual Install ZIP. Existing v1.0.0 is unchanged.

## Implementation

After the existing complete inventory/hash verification, guarded approval
tooling stages only those four version-specific basenames into a new directory.
Each selected input and copied output must match SHA256SUMS; missing, tampered,
wrong-version or malformed/duplicate checksum data fails closed. Preflight
failure creates no staging output. Existing output is never overwritten, and
output inside the retained validation directory is rejected. Publication must
consume this allowlist, not the complete validation artifact or any wildcard.

The workflow uploads a separate four-file Actions artifact. Source ZIP,
BUILD-INFO and hash-file generation/retention are unchanged. README HU/EN and
the [public-download policy](../release/PUBLIC_DOWNLOADS.md) separate future
user downloads from internal evidence and GitHub's automatic source links.
Release notes require hashes and reviewed durable matching-source access.
No replacement source-delivery mechanism, accepted package or publication is
approved by this change. No ROM or secret is added.

## Executed checks

Local Windows, Python 3.13.3; synthetic asset bytes, not actual installers.

| Check | Result |
| --- | --- |
| Full Python suite | PASS: 50 executed, 51 discovered; one existing Windows symlink capability test SKIP/NOT RUN (WinError 1314). |
| Four-download regressions | PASS: 7/7; four exact copies; retained source/evidence untouched; missing/tampered assets; preserved old output; malformed/duplicate/unsafe/missing manifest entries; invalid/prerelease labels; wrong version; overlap with validation directory. |
| Workflow contracts | PASS: 8/8; new public allowlist contract FAILS on baseline as expected, previous seven PASS. Ordering verifies checksums before staging; no evidence file enters the four-file upload. |
| Existing package/approval/toolchain regressions | PASS except explicit capability skip; unchanged approval/read-only/publication boundaries retained. |
| Workflow syntax | PASS: actionlint 1.7.12 with shellcheck/pyflakes disabled; same upstream-checksum-verified local tool used in A6. |
| Python 3.9 grammar | PASS via AST grammar check, not a Python 3.9 runtime run. |
| Whitespace | PASS: git diff --check. |

## NOT RUN and unchanged gates

- Final-head platform/sanitizer PR checks and merge are pending at this record.
- Native stable packaging workflow and staging of actual candidate binaries.
- Runtime/REAPER/private-ROM tests in this publication-layout-only round;
  Source/, plugin identities, GUI and runtime CMake configuration are unchanged.
- Coherent 1.0.1 version/label migration, A5 installer migration, A7 final asset
  reconciliation/durable source-access review and exact-candidate acceptance.
- New release/tag/asset publication or modification. Null approval unchanged.

The current workflow still builds 1.0.0-labelled assets; do not dispatch it to
replace the published release. The generic staging selector's future-version
tests do not claim that the guarded packager already accepts 1.0.1.
The [single plan](../release/EXECUTION_PLAN_1.0.md) keeps those gates separate.
