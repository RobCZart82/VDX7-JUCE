# 1.0.1 publication checkpoint — 2026-10-05

## Authority and frozen identity

The owner explicitly authorized publication and current repository/documentation updates, with GUI screenshots unchanged. The owner separately accepted the named manual-test deferrals below; they are NOT RUN / DEFERRED, never PASS.

Product and packager A=B: `96267f7304e657821ce35c54536689981f41ef27`.
Approval C: `edcb7471bf2ee08d3f8430e317cb9d17274f7576`.
Both public tags point directly to A. No rebuild or changes to the accepted files were made for publication. Later documentation commits do not change this identity.

## Evidence and limits

[Exact automated and package validation](VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md): Windows/macOS tests, debugger/sanitizer evidence, installer/payload checks, checksums, official Windows VST3 validation and complete matching offline source verification passed as recorded there. No new runtime-test execution on the publication documentation commit is claimed.

Owner-reported exact-bundle general acceptance: Windows 11 x64 / REAPER 7.82 x64; Mac mini M1 2020, 16 GB, macOS 26.7.1 / REAPER 7.82 Universal, PKG installation/use. Local owner checksum and native arm64 versus Rosetta mode were not independently supplied. Do not infer physical Intel Mac testing.

Additional owner-reported PASS in the current macOS test context:

- On a fresh REAPER project/instrument after restart, firmware under `~/Library/Application Support/VDX7-JUCE/ROM/DX7-V1-8.OBJ` and eight SysEx banks under `Factory Banks/` were automatically available in their correct slots.
- Selected voice survived project save, complete REAPER exit/restart and reopen.
- Edited unsaved (*) voice, its parameter settings and dirty marking survived the same project save/restart/reopen cycle.

The last recall message did not independently restate its platform; these detailed results are not extended to Windows. Screenshots demonstrate observed UI, not Yamaha provenance certification. Earlier 1.0.0 tests are not counted as new 1.0.1 tests.

## Explicitly accepted deferrals

NOT RUN / DEFERRED with owner approval in this chat on 2026-10-05:

- Full sample-rate/buffer and offline-versus-real-time render comparison matrix.
- Detailed automation, multi-instance, HiDPI and native save-dialog tests.
- Physical Intel Mac host test.
- Detailed old-project recovery with missing/different ROM.

Automated platform/debug tests, installers, content, checksums, matching source and known release blockers were not deferred. Deferred coverage remains future maintenance work; publication is not complete matrix certification.

## Public delivery

- [Product release](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1): stable/latest, exactly four uploaded VST3 user assets (Windows EXE + Manual ZIP, macOS PKG + Manual ZIP).
- [Source companion](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source): not latest; complete source ZIP plus a source-only SHA256SUMS file.
- [Bilingual final notes and exact hashes](../release/RELEASE_NOTES_1.0.1_HU_EN.md).

Public assets were downloaded again and compared by SHA-256 with the staged accepted files. Both tags target the exact product commit. Source staging verified 5131 manifest files before upload. Integrity is not publisher authentication. Unsigned Windows installer and unsigned/not-notarized macOS installer are explicitly disclosed; macOS ad-hoc signing is not Developer ID signing. No AU, Standalone, Yamaha firmware or factory bank payload is added. Existing v1.0.0 assets/tag and repository screenshots were not modified.
