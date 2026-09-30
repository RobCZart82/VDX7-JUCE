# Reproducible development source package

This prepares local/CI artifacts; it never uploads, tags or publishes a release.
Python 3.10+ and Git are required. Use a full commit SHA that contains the new
packager; the dirty working tree and untracked files are intentionally ignored.

## Bundled verifier and product/tooling separation (1.0.1 development)

New packages include `.vdx7-source-tools/package_source.py` and
`SOURCE_PACKAGE_README.txt`. From the extracted folder, run
`python .vdx7-source-tools/package_source.py verify <original-source.zip>`.
These generated additions are hashed in the manifest; the selected product
snapshot, including its original `scripts/package_source.py`, is unchanged.
This fixes AUDIT-20260930-A3 for future packages, not the already published
1.0.0 archive. Its older product checker does not understand accepted metadata.

When packaging an older product commit, pass `--packager-commit <full-tooling-SHA>`.
That commit must be available in `--repo`; the tool verifies that its committed
script matches the executing script (CRLF checkout normalization allowed).
Without the option, the product commit is also the packager commit. Commit
tooling first; uncommitted checker changes are not valid provenance.

```text
python -m unittest discover -s Tests -p test_package_source.py -v
python scripts/package_source.py create --repo . --juce <JUCE-checkout>
  --core <Retromulator-checkout> --commit <40-character-SHA>
  --package-label 1.0.0-dev --output <new-folder>
python scripts/package_source.py verify <new-folder>/<source-package>.zip
```

The dependency checkouts must contain the pinned commits, but their current
branches and local edits are irrelevant: only committed Git blobs are read.
The packager validates pins against the selected wrapper's CMakeLists.txt and
fails if they drift. It does not fetch dependencies or require ROMs. A destination
that already exists is refused rather than overwritten.

Contents: the wrapper snapshot, full pinned JUCE source with all bundled notices,
the pinned dx7Lib subset under `third_party/dx7Lib`, and Retromulator LICENSE/README
under `third_party/retromulator-notices`. `SOURCE_MANIFEST.json` records exact
revisions, file sizes, modes and SHA-256 hashes. `SHA256SUMS.txt` hashes the ZIP.
Manifest integrity is not a digital signature or independent proof of authorship.

Ordering, ZIP timestamps, file modes and stored (uncompressed) entries are fixed.
The same Git inputs should therefore produce byte-identical ZIPs across hosts;
this does not claim reproducible compiler output or signed plugin binaries.
Uncompressed ZIPs are larger deliberately, avoiding compression-library drift.

Safety checks reject unsafe paths, links/submodules, caches/VCS metadata,
firmware/bank/compiled/credential-like filenames, unknown ROM-directory payloads,
nested wrapper archives and common private-key/token/personal-path patterns.
Pinned JUCE reference assets (including icons ZIPs and teapot OBJ meshes) are
retained. These checks are not a universal secret or firmware detector: final
human archive review and matching-binary provenance are still required.

After verifying the ZIP, extract to a new directory and build with:

```text
cmake -S <extracted-source> -B <new-build> -DFETCHCONTENT_FULLY_DISCONNECTED=ON
  -DVDX7_ENABLE_ROM_TESTS=OFF -DVDX7_RELEASE_BUILD=OFF
cmake --build <new-build> --config Release --target VDX7_VST3 VDX7_Standalone vdx7_ci_checks
ctest --test-dir <new-build> -C Release --output-on-failure --no-tests=error
```

Add the platform generator/architecture options from the HU/EN build guide.
No installed plugin should be replaced by this check. The exact-candidate workflow
prepares matching test-candidate source and packages. The final stable build still
needs its exact-SHA binary/source pairing, checksum and archive review. Owner-
reported focused RC1 host/listening checks are recorded separately; the broader
host/audio/GUI matrix is explicitly deferred, not passed. See the
[release checklist](RELEASE_CHECKLIST_1.0_RC.md).

Magyarul: a csomag kizárólag a megadott commitok fájljait tartalmazza, helyi
ROM-ot, munkapéldány-módosítást vagy buildmappát nem másol át. Az ellenőrzés és
az offline próbafordítás nem helyettesíti a végleges kiadás jóváhagyását.
