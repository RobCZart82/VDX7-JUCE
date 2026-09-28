# Source packaging preparation — 2026-09-28

Baseline main `d4ef9516b23e7198e6963d1a08b74195a9802159` includes merged #88.
Its PR source passed Windows 36405070149, macOS 36405070251 and ASan/UBSan
36405070221. No product/audio/GUI source is changed in this packaging round.
No REAPER, installed-plugin replacement, ROM upload, tag or release publication.

## Implemented

- Exact-commit source packager reads Git blobs, not working files. Pinned JUCE
  and dx7Lib sources/notices are included. Fixed ZIP ordering/timestamps/modes;
  uncompressed entries avoid compression-library drift. Manifest hashes all
  payload files; separate checksum hashes the complete ZIP.
- Refuse unsafe paths, links/submodules, common cache/firmware/credential payloads
  and existing output destinations. Pattern checks are not a universal security
  guarantee; final review remains required.
- Six ROM-free Python tests exercise determinism, tamper detection, forbidden
  content, unsafe paths, modes/identity and dirty/untracked working-file exclusion.
- Ordinary Windows/macOS CI runs the packaging tests; exact-candidate workflow
  prepares a matching source ZIP without granting release-write permissions.
- Current HU/EN candidate instructions and NOTICE wording no longer imply that
  the historical v0.6.6 archive is the source for a 1.0 development binary.

## Verification record

- Initial sandbox test attempt: environment ERROR, temporary-directory access
  denied. Normal-permission retry found a genuine packaging-test FAIL caused
  by git archive applying Windows CRLF conversion. Switched to direct Git blobs.
- Six packaging unit tests after correction: PASS (Python 3.13, Windows).
- Baseline source package probe: PASS, 5068 files; all manifest hashes verified.
  ZIP SHA-256 `03d10b87092493dcc99667711fb5e453c58629e96eccb60db56f0d05ba7637e2`.
- Exact local source commit `dc68b6fb951a7e135c5c562cd427d63e3c78835a`:
  two independently generated ZIPs PASS, byte-identical SHA-256
  `f296eafcec099e07fbbb6615c8722ea2c350c9c5786ceb7785a6858567fb2354`.
  Each contains 5073 payload files plus the manifest; manifest verification PASS.
  This identifies the tested local commit, not a claim that it is remote main.
- Actual ZIP extracted into a new directory: configure PASS with VS 2022 x64,
  `FETCHCONTENT_FULLY_DISCONNECTED=ON`, `VDX7_ENABLE_ROM_TESTS=OFF` and
  `VDX7_RELEASE_BUILD=OFF`. JUCE/dx7Lib were resolved inside the extracted source.
  This checks dependency-disconnected building, not a network-isolated sandbox.
- Extracted-source Release builds `VDX7_VST3`, `VDX7_Standalone` and
  `vdx7_ci_checks`: PASS. CTest: 10/10 PASS, 5.87 seconds. The extracted copy's
  six Python packaging tests: PASS, 1.123 seconds. ROM-dependent executables
  compiled but their opt-in runtime tests were NOT RUN in this ROM-free build.
- Local logs retained outside Git: `PACK_offline_configure.log`,
  `PACK_offline_build.log`, `PACK_offline_ctest.log` in the task output directory.
- New remote CI/exact-candidate workflow: NOT RUN at this local checkpoint.

## Post-merge main verification — 2026-09-28

Exact source: `fbea5ea167598f9b625eab9145f53b8700d9eb15` (merge commit #89).
This supplements, and does not rewrite, the earlier `dc68b6f` branch-source
evidence above.

- Fresh `origin/main` fetch confirmed the source SHA. PR #89 Windows, macOS and
  ASan/UBSan runs passed (36410161563, 36410161589, 36410161474); post-merge
  main Windows/macOS runs passed (36411273600, 36411273595).
- Local Release build target `vdx7_ci_checks`: PASS. ROM-free CTest: 10/10 PASS.
- Source-packaging Python unit tests: 6/6 PASS. CTest registration checker:
  full 36-test local-ROM-on inventory and seven negative controls PASS.
- Generated the corresponding-source archive directly from this exact commit
  using Python 3.12.14. 5,073 payload files; embedded manifest verification
  PASS; ZIP SHA-256 `19c5ab95e1af36c15b19194a37603557b1975bcd5247d72f94b284ff8db113cd`.
  Archive path-portability scan found no case-fold collisions or Windows
  reserved/trailing-dot/space names.
- Checked 192 relative Markdown links across the repository: none unresolved.
- Owner-reported REAPER acceptance and accepted signing policy are documented
  in the release plan/checklist. The plan's prior entries specify macOS 26.7
  and Windows 10 x64 build #219 / `29ab5e3`; not all binary hashes, REAPER app
  versions or matrix cells are available. This is not runtime validation.
- NOT RUN on this exact merge commit: extracted-source offline rebuild,
  full local-ROM integration execution, exact-candidate workflow, interactive
  Settings/HiDPI and exact-RC host/audio acceptance. This was non-host work.

## Independent VST3 validator

PASS: official Steinberg SDK `v3.8.1_build_84`, commit
`3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, locally built Release validator.
Its own 51 self-tests passed; separately, VDX7 passed 47 tests, 0 failed,
exit code 0. Invocation: `validator.exe <extracted-build>/VDX7.vst3`.
VDX7 was the Windows x64 development binary built from the ZIP above.

- Validator SHA-256: `e3c6d635fc7fc1e3b4b38beaec551c9cd09adab3c74af55f216395a6d5756986`.
- Plugin module SHA-256: `ebea162a064899a856bda4cb84028c9ad65ba562d285da4714e240a142430cb8`.
- Log: `PACK_vdx7_validator.log` (local). Unsupported 64-bit audio processing
  is reported as informational, not a failure. No ROM fixture was explicitly
  supplied to this validator; this is API/bundle validation, not audible ROM or
  real-host acceptance. Repeat on the final exact candidate.
- [Official validator documentation](https://steinbergmedia.github.io/vst3_dev_portal/pages/What%2Bis%2Bthe%2BVST%2B3%2BSDK/Validator.html).

## Interactive Standalone check (partial)

The newly built local Standalone launched and displayed the approved EDIT view,
`1.0.0-dev`, and a loaded-firmware status using an existing private local ROM.
No ROM was copied into this package or uploaded. About opened successfully;
VDX7/GYR/signature assets, development version, notices and OK control were
visible without obvious clipping at the observed size: PASS for this narrow
visual/opening check only. The Windows desktop then locked. UI automation was
stopped; About dismissal, Settings interaction, preset changes and HiDPI remain
NOT RUN. The test application was left open, not forcibly terminated. These
observations do not close F12 or establish settings persistence/audio acceptance.

Remaining separate gates: binary package/checksum/signature policy, final archive
review, exact-RC host/audio acceptance, interactive Settings/About/HiDPI access.
No automatic acceptance or publication is implied by a successful source ZIP.
