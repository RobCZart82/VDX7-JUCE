# Stable 1.0.0 installer acceptance — 2026-09-29

## Exact build under test

- Product source: `d79ed5214d82caf70e3941e5a620bab137d3f9ca` (main, merge of PR #101).
- Non-publishing package workflow: [run 36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919), successful on the exact source SHA above.
- Windows x64 and macOS Universal builds and ROM-free CTests: PASS.
- Windows installer compile and CI install/uninstall smoke test: PASS.
- macOS Universal architecture, plugin signature and package payload checks: PASS.
- Combined asset inventory and SHA-256 manifest generation/verification: PASS.
- Owner reports real-machine installation and REAPER use on Windows and macOS; see limits below.
- Distribution is VST3-only; no AU asset is included.

## Artifact and independently repeated checks

- Artifact: `VDX7-1.0.0-Release-Assets-d79ed5214d82caf70e3941e5a620bab137d3f9ca`
- Artifact ID: `11059321610`
- GitHub outer Actions ZIP SHA-256: `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`; independently downloaded outer ZIP produced the same SHA-256.
- Expires: 2026-12-28.

The downloaded artifact contains the expected eight files: both BUILD-INFO files, `SHA256SUMS.txt`, the Windows installer and manual ZIP, the macOS installer and manual ZIP, and the corresponding-source ZIP. I independently ran `shasum -a 256 -c SHA256SUMS.txt`; all seven listed files returned `OK`. The inner hashes were:
- `BUILD-INFO-Windows-x64.txt`: `0fe8c4903c9e6ae9a58d7ba484f356937f1e83ef4616c9b1b9fd7a44ccf69b6d`
- `BUILD-INFO-macOS-universal.txt`: `2d37f33a8111d2e9feb9df53129a6a36dbca3a2885cb5b5eec7d6f960a7efc30`
- `VDX7-1.0.0-Windows-x64-Manual.zip`: `dae2f6a23971f7eb6e48eb9a5f120fd828f69bef8883da31b46265cbe8ba1dc1`
- `VDX7-1.0.0-Windows-x64-Setup.exe`: `670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3`
- `VDX7-1.0.0-d79ed5214d82caf70e3941e5a620bab137d3f9ca-corresponding-source.zip`: `7d0f616644d318803ee977b773620ed873edbeba935cfc86044c022d0772305f`
- `VDX7-1.0.0-macOS-universal-Manual.zip`: `03f43764b1652a556ab22c30b7cfe278acee30e0b594e24e3135787c9de6cb10`
- `VDX7-1.0.0-macOS-universal.pkg`: `9d1d6cabe08c425f2f6ef30172c43e25e00d0e08f9bf8add536505fe5fd12de8`

## Source archive and plug-in payload review

- Ran the bundled `scripts/package_source.py verify` against the corresponding-source ZIP: PASS, 5,082 files checked against its embedded manifest.
- Manifest source commit: `d79ed5214d82caf70e3941e5a620bab137d3f9ca`; dependencies match pinned JUCE `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and Retromulator `d5473776a0449d60a997b91bdc888598a33265ac`.
- No ROM, SysEx/MIDI data, credential-like files, local user/runner paths, private-key markers, or unexpected ROM-directory content were found. The archive contains vendored JUCE source and its upstream example assets.
- macOS manual VST3 binary reports both `arm64` and `x86_64`; `codesign --verify --deep --strict` reports it valid. The pkg payload points to `Library/Audio/Plug-Ins/VST3/VDX7.vst3`.
- Windows manual bundle contains the x86_64 VST3. Setup is a Windows PE installer; CI install/uninstall smoke passed.
- macOS pkg has no signature; the Windows Setup.exe has no publisher signature. These are confirmed expected limitations, not validation failures.

## Owner-reported real-machine acceptance

- macOS: screenshots show Gatekeeper initially blocked the Universal `.pkg`; the owner used Privacy & Security → Open Anyway, authorized installation, and reached the Installer success screen. The VST3 opened in REAPER and displayed version 1.0.0. The owner reports it works and sounds good. Screenshots identify a Mac mini M1 (2020), 16 GB, macOS Tahoe 26.7.
- Windows: the owner reports the installer works and the installed plugin opens and works in REAPER.
- These are OWNER-REPORTED, not assistant-run tests. Exact installed package checksums, REAPER versions for these installer trials, and a detailed rate/buffer/transport matrix were not supplied. Do not infer deferred matrix cells passed.

## Release blockers

This is still a validation artifact, not a public Release. Both BUILD-INFO files explicitly say “PREPARATION ARTIFACT; not accepted or approved for publication,” while the corresponding-source manifest has `release_accepted: false`. Do not attach these unchanged to a public stable release. The final stable asset set must carry accurate release BUILD-INFO/source acceptance metadata and have `SHA256SUMS.txt` regenerated and reverified. Preserve the existing tested installer/VST3 payloads if the final metadata-only repackaging can do so; otherwise clearly tie the newly generated binaries to their own acceptance evidence.

The Windows installer is unsigned. The macOS installer is unsigned; its VST3 has only an ad-hoc signature, not Developer ID signing or notarization. Gatekeeper warning was observed. Do not recommend disabling system-wide protections. The workflow is read-only for repository contents and cannot create tags or Releases; it created no stable `v1.0.0` tag or Release.

Final steps: produce a publication-ready asset copy with truthful accepted-status metadata, verify its new combined checksum manifest, finalize bilingual release notes and deferred-test disclosures, then review the exact release version/SHA/assets before publishing.
