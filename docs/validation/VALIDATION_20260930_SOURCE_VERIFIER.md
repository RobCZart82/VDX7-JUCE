# Corresponding-source bundled checker — 2026-09-30

AUDIT-20260930-A3. The published 1.0.0 archive retains the old product checker,
which rejects the newer accepted manifest. Do not replace that published asset.

New package generation preserves every selected product Git blob, including
the original checker, and adds `.vdx7-source-tools/package_source.py` from the
specified packager commit plus a generated `SOURCE_PACKAGE_README.txt`.
Both additions are covered by the ordinary manifest inventory/hash checks.
The executing checker must match that committed tooling blob; false/stale
tooling identity and generated-path collisions fail before creating output.

## Results

- FAIL before fix (expected): new cross-version regression could not find the
  separately bundled checker in the generated ZIP.
- PASS after fix: 10 Python source-packaging unit tests. Cross-version tests
  generate both prep and accepted archives, preserve the synthetic old product
  checker, and execute the checker extracted from each ZIP in a subprocess.
- PASS: existing hash/mode/identity/tampering/extra-path/safety controls plus
  reserved-tooling-path and false-packager-identity controls.
- PASS: two real accepted-mode LOCAL TEST archives generated from product
  `d79ed5214d82caf70e3941e5a620bab137d3f9ca` and local tooling commit
  `f07d37511f263a720b78bc012ba568f06ed14d6c`; both contain 5084 files and hash
  `e18d4dc9d1d522c84dfd239183d7c0d3a3eb2c65d0796baa1b4d09f5d36555c2`.
  Accepted-mode testing is not publication approval. No test ZIP was uploaded.
- PASS: generation-time manifest verification of both full archives.
- PASS: full real archive extracted and its bundled checker independently
  executed against the original ZIP; all 5084 files verified.
- NOT RUN: new offline rebuild of this extracted archive (the product sources
  are unchanged); do not infer it from checker success.
- NOT RUN: final 1.0.1 candidate packaging, installed-host acceptance.

Reproduce with committed tooling, pinned dependency Git checkouts and fresh
output directories: `python scripts/package_source.py create --repo <repo>
--juce <juce> --core <core> --commit <old-product-sha> --package-label 1.0.0
--release-accepted --packager-commit <tooling-sha> --output <new-dir>`.
Then use the extracted `.vdx7-source-tools/package_source.py verify <zip>`.
The original product scripts remain historical source, not the recommended
checker for metadata produced by a newer packager.

The local checksum suite's symlink-creation case still needs Windows privilege
or CI; do not count that separate environment limitation as a packaging PASS.
No proprietary ROM or installed binary is used by these source-package tests.
