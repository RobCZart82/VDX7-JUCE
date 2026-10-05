# SysEx export acknowledgement and bank library regression

This round fixes a reproduced false unexported-edit warning and makes the private
bank-folder processor integration test available through CTest. Baseline main
is `dccb745b6b73b6ccecdb39266028ed696550d050`. No release, tag, approval record,
installed plugin or original private ROM is changed by this development round.

## Reproduction and correction

An accepted synthetic VMEM bank with byte 13 of its first voice set to `0x40`
contains a reserved seven-bit field bit. After renaming the first voice, a
successful single VCED export left `hasUnexportedEdits()` true. Exporting the
whole bank cleared it; the identical single-voice control with byte 13 set to
zero also cleared it. The baseline reproduction was repeated before the fix:
FAIL for the reserved-bit single export, PASS for both controls.

VCED has no representation for those VMEM reserved bits. Comparing decoded
VCED against the original working RAM therefore cannot acknowledge the export
reliably. The immutable export snapshot now retains the original packed bytes
as well as the encoded message. Export validates their consistency before any
write, then compares current RAM with the original packed snapshot. It never
normalises the working sound to acknowledge an export. Actual later edits and
failed writes must remain dirty; export bytes and the project format are unchanged.

`vdx7_export_acknowledgement` checks canonical and reserved-bit voices, unchanged
RAM, later edits, full-bank acknowledgement and failed I/O. It is a separately
registered local-ROM test and also runs in the complete processor test.

## Private bank integration registration

With `VDX7_ENABLE_ROM_TESTS=ON`, the optional CMake path
`VDX7_TEST_FACTORY_BANK_FOLDER` registers `vdx7_factory_bank_library`. The existing
`--factory-bank-library` processor probe now runs as an ordinary CTest with a
timeout, local-ROM label and mandatory v1.8 ROM-profile fixture. It requires all
eight locally owned reference banks; incomplete/invalid fixtures fail, not skip.
Providing a folder with ROM tests disabled, or a nonexistent/non-directory path,
fails configuration rather than silently ignoring the requested test.

For local execution, supply the existing ROM option and the private folder:

```text
-DVDX7_ENABLE_ROM_TESTS=ON
-DVDX7_TEST_ROM_FILE=<private combined v1.8 ROM>
-DVDX7_TEST_FACTORY_BANK_FOLDER=<private eight-bank folder>
```

After building `vdx7_ci_checks`, run CTest normally or filter
`^vdx7_factory_bank_library$`. Validate the JSON inventory using
`scripts/check_test_registration.py <inventory.json> --factory-bank-library`.
The checker enforces 46 tests for the ordinary complete local suite, 47 when the
bank folder is enabled, and the unchanged 14 ROM-free tests. Its self-test covers
default/private-bank positive controls and missing/unrequested-bank negatives.

Windows/macOS public CI uses an empty directory and placeholder ROM only for
registration inspection, never firmware execution. A green registration check
does not claim private integration passed. ROM and bank contents are never
uploaded or packaged. In this local round the existing combined ROM's eight
payload hashes matched the reference identities; unchanged payloads were wrapped
as temporary private SysEx fixtures outside the repository.

## Validation status

- PASS: baseline reproduction and canonical/full-bank controls establish the defect.
- PASS: local optional-bank CTest inventory, labels, fixtures, timeouts and checker controls.
- PENDING: current corrected Windows build and complete local CTest regression.
- PASS: Python suite, 76 tests passed and one Windows symlink-capability SKIPPED.
- PENDING: final PR Windows/macOS/ASan-UBSan checks.
- NOT RUN: REAPER, installed-package acceptance, local macOS/sanitizer and new final packaging.

Recorded results will be updated before merging; pending work is not PASS.
Earlier final packages at `dbad14a` do not contain this product fix. Their hashes
and acceptance evidence remain historical; rebuilding/reapproving exact final
packages and matching source is a separate release gate.
