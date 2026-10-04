# 1.0.1 final test package verification

The owner approved the exact product/packager pair on 2026-10-03 and will
install and manually test the final packages in Windows/macOS REAPER.
This report records automated verification and package provenance, not
host acceptance or permission to publish. Existing v1.0.0 remains untouched.

## Approved identities and automated gates

- Product A and packager B: `b7fce0503059c8c2e3c61d6ccf5af33612739e32`.
- Approval C: `828320d78e5d2e193356af0485a15c12f0279824`, merged #120.
- #120 final head: `fa28fcc05107ca059c8119f364b97ed13d0c2f24`.
  Windows `37112554104`, macOS `37112554121`, ASan/UBSan `37112554105`: PASS.
- Post-merge main Windows `37113259165`, macOS `37113259214`: PASS.
- Accepted-mode canonical main workflow
  [37195536400](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37195536400)
  dispatched 2026-10-04: authorization, Windows, macOS and assembly all PASS.
  Accepted metadata is not publication authority.
- Current checkout Python suite: 77/77 PASS, no skips, 12.506 seconds.

## Exact source archive

Locally regenerated from Git objects A/B and committed policy C with pinned
JUCE `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator
`d5473776a0449d60a997b91bdc888598a33265ac`. Current checker: 5,120 files PASS,
approval provenance internally consistent. SHA-256:
`8d4da112f4565c49ab9128cb84bf5e4527470aa3486492397cfcb11b93ff8784`.
Archive name:
`VDX7-1.0.1-b7fce0503059c8c2e3c61d6ccf5af33612739e32-corresponding-source.zip`.

This is a new accepted-metadata archive, not a relabelled prep ZIP. Downloaded
hosted source ZIP is byte-identical to this locally built/tested archive.
Fresh extracted build configured with bundled dependencies and
`FETCHCONTENT_FULLY_DISCONNECTED=ON`, CMake 4.4.3, AppleClang 21.0.0,
Ninja, Release, `VDX7_RELEASE_BUILD=ON`, ROM tests OFF. Configuration PASS;
CI-target compilation PASS (156-step graph); 13/13 CTests PASS in 5.04 seconds,
77/77 extracted-source Python tests PASS in 11.109 seconds. Bundled checker
also PASS with `PATH=/nonexistent`, no Git/network. All 5,120 payload files
are byte-identical to the previously tested prep archive; only
`SOURCE_MANIFEST.json` differs with approval metadata. Focused private-firmware
`--bank-only` validation/preservation/warning/export/reopen PASS.
VST3 target and manifest helper compilation PASS; workflow-equivalent explicit
final local ad-hoc signing and strict verification PASS. This is local ARM64
compile evidence, not hosted Universal/Windows or REAPER evidence.
The first bundled-checker invocation used a nonexistent guessed filename;
retrying the observed `.vdx7-source-tools/package_source.py` passed. The first
restricted-PATH VST3 command could not locate CMake; retrying with its observed
absolute executable and existing Ninja directory passed. No installed bundle
or product source was changed. An explicitly supplied
`FETCHCONTENT_SOURCE_DIR_RETROMULATOR` was unused; CMake selected bundled
`third_party/dx7Lib` directly. This warning is not a fetched dependency.

## Independent preparation evidence and boundaries

The preceding exact-source preparation `37051589142` completed all four jobs.
Three outer artifact digests and all seven inner hashes PASS. macOS payload
is x86_64+arm64, stable 1.0.1, `org.vdx7.prototype`, target 11.0;
strict ad-hoc signature verification PASS. PKG expanded plugin and Manual ZIP
plugin are byte-identical. PKG destination is `/Library/Audio/Plug-Ins/VST3`.
`pkgutil --check-signature` reports no publisher signature, as documented.
Windows Manual contains PE x86-64. Hosted Windows actual 1.0.0-to-1.0.1
upgrade/uninstall and synthetic user-file preservation PASS. No local Windows
installer execution or user-machine replacement occurred.

## Final hosted packages independently verified

Downloaded native Mac, complete validation, four-download and source-download
artifacts through the GitHub API. All outer hashes matched GitHub digests:

| Artifact | ID | Outer ZIP SHA-256 |
| --- | --- | --- |
| Mac platform | 11300442859 | `803152b4a037f7d665f9c7c9fc3b63ccf4d0789d48b146ff8e643349c48472ce` |
| Complete validation | 11301047102 | `abecdfaf4a1f2e5d7a73c41b9cc16e0862496f14d5231479cfb3e2343b0b457c` |
| Four downloads | 11300982217 | `70d2f2372a78eb2b05979100acfe4be67f11e9b4b83ee3ef8dc305b5c9e1e325` |
| Source downloads | 11301126827 | `d89e5b0bd288dc86729788a688b38eb98722e9df629c24653c6e51fc8a3e6eb0` |

Seven inner SHA256SUMS entries PASS. Exact four/two-file inventories and all
staged copies byte-identical to complete validation PASS. Both manual ZIPs
contain only the complete VST3 bundle; no AU/Standalone/firmware is included.

| Final product file | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `2001a7f494580e45db706b50c1a9342991dcc3d90868ad1ad8ec9b179de948a0` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `31471ec4e7fd61e76fef1da5c1c948e37f4f1d25eb2fe70eab9aee8b8626efac` |
| `VDX7-1.0.1-macOS-universal.pkg` | `97ea4a9ebcba4166d2677f69e668d163e0b3b9f5a12c497c1853bf6613da2791` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `a2f17e467b6e8ff5901b3a311be9faadf4dc394ee0e982b097a8d76ddb4c6e39` |

Final source SHA-256 is the `8d4da112…` full value above; source staging
one-file checksum PASS, current/bundled 5,120-file checkers PASS, exact A/B/C
proof PASS. Both BUILD-INFO records match A/B/C and run 37195536400.
Windows observed MSVC 19.44.35229.0, SDK 10.0.26100.0, Inno 6.7.1;
macOS Xcode 15.4, AppleClang 15.0.0.15000309, SDK 14.5, target 11.0.

Final Windows native hosted 1.0.0 upgrade/uninstall PASS for installer hash
`2001a7f4…`; all two installed candidate payload hashes matched, one stable
AppId/registration, uninstall removed plugin/registration and preserved
synthetic USER/document bytes. This is not playable-bank or REAPER evidence.
Final independent Windows Manual PE x86-64 PASS. Final macOS Universal
x86_64+arm64, plist 1.0.1/identity/target, strict ad-hoc signature and every
expanded PKG versus Manual bundle file PASS. PKG remains unsigned and not
notarized; no Developer ID claim. No installed plugin was replaced.

No full private-ROM acceptance or host/audio/HiDPI/physical Intel Mac result
is inferred. Original invalid combined-ROM FAIL history remains valid.
Detailed ROM diagnostics are owner-deferred, but strict rejection/state
preservation remain unchanged. No ROM or raw personal-path log is committed.

## Manual handoff and publication boundary

The owner can now use the verified Four-Downloads artifact from run
37195536400; the outer Actions ZIP contains the four actual packages above,
not the plugin bundle itself. The source artifact remains separate.
Installation and final REAPER testing are NOT RUN here and
are not deferred by the packaging approval. Use the manual handoff section
of the [checklist](../release/RELEASE_CHECKLIST_1.0_RC.md); report unsupported
cells and any omitted tests explicitly rather than describing them as PASS.

After owner results, reconcile HU/EN notes, four product hashes and the separate
durable source-release payload. Obtain final publication permission before
creating tags/releases/uploads. The [execution plan](../release/EXECUTION_PLAN_1.0.md)
remains the only active roadmap.
