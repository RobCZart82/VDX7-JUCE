# Installer upgrade and separate source preparation — 2026-10-01

Baseline: main `d4f589764284e72a2d532a6055ac8d918cb53d9c`, after PR #112.
The owner requests 1.0.1 publication and selected a separate non-latest
`v1.0.1-source` release. The main release retains four user downloads.
This round prepares those checks/payloads; it does not approve final assets.

## Windows upgrade assessment and smoke

Keep the existing AppId, VST3 install path and uninstall-file directory.
The [Inno Setup documentation](https://jrsoftware.org/ishelp/topic_setup_uninstallfilesdir.htm)
warns against moving uninstall files between versions because old logs can
no longer be appended. No host scan failure from the current layout was
reproduced; changing it is not justified without a migration design.

The new smoke refuses local/self-hosted machines and existing installation
or data fixtures. It only runs on disposable GitHub-hosted Windows runners,
after the existing fresh candidate install/uninstall smoke. It downloads the
published 1.0.0 installer, verifies its exact SHA-256 before execution, installs
it, upgrades to 1.0.1 and then uninstalls the upgraded application.

Baseline installer SHA-256:
`670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3`.

The smoke checks one stable AppId registration, both installed versions,
every candidate payload file hash, complete bundle/registration removal and
preservation of synthetic files at the USER-bank and Documents locations.
Synthetic markers are not playable USER banks or firmware. This is not a
REAPER/audio test. Its result and installer hashes enter the hashed Windows
BUILD-INFO asset only after the full smoke passes.

PR Windows CI parses the helper with the real PowerShell parser without
running installers. The local macOS environment lacks PowerShell; actual
parser execution is a remote gate, not a locally claimed pass.
[GitHub runner variables](https://docs.github.com/en/actions/reference/workflows-and-actions/variables)
define the hosted/self-hosted boundary used by the guard.

## Separate source payload

After combined asset checksums pass, source staging verifies its checksum and
embedded source manifest, exact product/packager identity and acceptance status.
Accepted mode additionally requires the exact committed approval proof.
It creates exactly the source ZIP plus a one-file SHA256SUMS manifest in a new
directory; old output/validation assets are never overwritten. The workflow
uploads that separate allowlist while retaining the four-file binary allowlist
and complete internal validation artifact. No workflow gains publication rights.

## Executed local checks

- PASS: 68/68 Python tests, no skips, macOS Python 3.9.6. Eight new source
  staging regressions include integrity, tuple/status/approval mismatches,
  no output on rejection, invalid versions/SHAs, duplicate checksum entries,
  preserved old output and separation from retained binary assets.
- PASS: existing four-download regressions and new workflow/upgrade contracts.
- PASS: actual 5,108-file preparation source archive from
  `6e638f1b47d895d030d2ee506c336aab6776468c` staged as exactly two files
  and its generated SHA256SUMS checked successfully. This is the earlier local
  test fixture, not the final candidate. Its hash is recorded in
  [version preparation](VALIDATION_20261001_VERSION_101.md).
- PASS: actionlint 1.7.12 with optional shellcheck/pyflakes disabled, YAML
  parsing and whitespace checks for both changed workflows.

## NOT RUN and remaining gates

Actual Windows upgrade/uninstall, native 1.0.1 installer packaging/toolchain
capture and exact final source/four-file staging remain NOT RUN here.
Final-head Windows/macOS/sanitizer and post-merge main checks are required.
Runtime sources, GUI, identities, installer layout and approval JSON are
unchanged in this round; no installed plugin, ROM or REAPER state is modified.
No tag/release/assets are published. Freeze the final source/tools only after
green gates, run non-accepted native preparation, inspect payloads/hashes,
then obtain the exact approval and reconcile final publication metadata.
The [single plan](../release/EXECUTION_PLAN_1.0.md) remains authoritative.
