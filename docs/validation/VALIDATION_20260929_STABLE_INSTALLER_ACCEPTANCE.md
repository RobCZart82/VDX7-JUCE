# Stable 1.0.0 installer acceptance — 2026-09-29

## Exact build under test

- Product source: `d79ed5214d82caf70e3941e5a620bab137d3f9ca` (main, merge of PR #101).
- Non-publishing package workflow: [run 36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919), successful on the exact source SHA above.
- Windows x64 and macOS Universal builds and ROM-free CTests: PASS.
- Windows installer compile and CI install/uninstall smoke test: PASS.
- macOS Universal architecture/signature/payload checks: PASS.
- Combined asset inventory and SHA-256 manifest generation/verification: PASS; every listed file returned `OK` from `sha256sum -c`.
- VST3-only distribution; no AU asset is included.

## Combined Actions artifact

- Name: `VDX7-1.0.0-Release-Assets-d79ed5214d82caf70e3941e5a620bab137d3f9ca`
- Artifact ID: `11059321610`
- Outer Actions ZIP SHA-256: `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`
- Expires 2026-12-28. The outer ZIP digest does not substitute for inner package hashes.

Expected contents:
- `VDX7-1.0.0-macOS-universal.pkg`
- `VDX7-1.0.0-macOS-universal-Manual.zip`
- `VDX7-1.0.0-Windows-x64-Setup.exe`
- `VDX7-1.0.0-Windows-x64-Manual.zip`
- `VDX7-1.0.0-d79ed5214d82caf70e3941e5a620bab137d3f9ca-corresponding-source.zip`
- `BUILD-INFO-macOS-universal.txt`, `BUILD-INFO-Windows-x64.txt`, `SHA256SUMS.txt`

## Owner-reported real-machine acceptance

- macOS: screenshots show the Universal `.pkg` was initially blocked by Gatekeeper because Apple could not verify the unsigned installer. The owner used Privacy & Security → Open Anyway, authorized installation, and reached the Installer success screen. The installed VST3 then opened in REAPER; UI showed version 1.0.0. The owner reports it works and sounds good. Screenshots identify Mac mini M1 (2020), 16 GB, macOS Tahoe 26.7.
- Windows: the owner reports that the installer works and the installed plugin opens and works in REAPER.
- These are OWNER-REPORTED tests, not assistant-run tests. Exact installed package checksums, REAPER versions for these installer trials, and a detailed rate/buffer/transport matrix were not supplied. Do not infer deferred matrix cells passed.

## Distribution caveats and remaining gates

- Windows `Setup.exe` is unsigned and may trigger an unknown-publisher prompt.
- The macOS `.pkg` is unsigned; its VST3 bundle has only an ad-hoc signature and is not Developer ID signed or notarized. Gatekeeper warning was observed. Do not advise users to disable system-wide protections.
- This is a validation artifact, not yet a public release. Its BUILD-INFO files explicitly say “PREPARATION ARTIFACT; not accepted or approved for publication.” Do not attach these unchanged to a stable Release. Final BUILD-INFO and regenerated/reverified manifest are required.
- The workflow is read-only for repository contents and cannot create tags or Releases. This run created no stable `v1.0.0` tag or Release.
- Independently inspect the extracted artifact: verify the embedded manifest against every file; inspect VST3 and corresponding-source ZIP; confirm source/build identity and dependency revisions; scan for firmware, credentials, local paths, caches, and unrelated files.
- Finalize bilingual release notes and deferred-test disclosures, then review exact source SHA, final asset list, signatures/warnings, and remaining acceptance status before publication.
