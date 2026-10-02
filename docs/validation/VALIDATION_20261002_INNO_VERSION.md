# Inno compiler version guard correction — 2026-10-02

Baseline main: `9d53e9ee578ae614f76025d36ef7a258659de3f6`.
Failed non-publishing run: [36993835698](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36993835698).

## Evidence and correction

Windows built/tested the plugin, then packaging failed at `Inno Setup version
mismatch: 0.0.0`. Chocolatey reported InnoSetup 6.7.1 already installed.
The former guard treated numeric executable ProductVersion metadata as engine
identity. macOS packaging PASS; assembly and Windows install/upgrade NOT RUN.

The [pinned upstream 6.7.1 source](https://github.com/jrsoftware/issrc/blob/is-6_7_1/Projects/ISCC.dpr)
does not support `--version`; help exits before loading the engine. A no-output
stdin compile loads the engine and prints its version. The new Python verifier
uses `/O- -`, a minimal `Output=no` script, a 60-second timeout, no shell, and
requires exit zero plus exactly one complete `Inno Setup 6.7.1` engine record.
Wrong, missing, ambiguous or suffixed versions and unsuccessful probes fail
closed. The workflow checks the verifier exit code before producing its EXE.
Existing provenance still records the verified version and executable SHA-256.

## Validation

- PASS: before implementation, workflow regression fails against the old guard;
  three new probe tests error because the helper is absent (test-first checkpoint,
  not a native reproducer). The Action log is the native failure evidence.
- PASS: after implementation, 4 probe tests and 12 workflow-contract tests.
- PASS: full Python suite before the final CLI-output control: 76 discovered,
  75 executed, one Windows symlink-capability SKIP; the final 4-test probe suite
  was then rerun successfully. Skipped coverage is not counted as PASS.
- NOT RUN: local native ISCC compile (Inno is not installed locally).
- NOT RUN: fresh corrected remote installer preparation and upgrade/uninstall.

No product C++ changes, ROM, release approval, tag or published asset changes.
No REAPER, user plugin replacement, or local installer installation.

## Native follow-up

Correction merged in #115 as `6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14`.
Fresh non-publishing run `36998278475` PASS on both platforms and assembly.
Actual Inno 6.7.1 verification, Windows clean install/uninstall and 1.0.0 ->
1.0.1 upgrade/uninstall/user-file preservation PASS. [Candidate record](VALIDATION_20261002_PACKAGE_PREPARATION.md).
This supersedes the initial remote NOT RUN checkpoint, not the local native
NOT RUN or final publication-acceptance boundary.
