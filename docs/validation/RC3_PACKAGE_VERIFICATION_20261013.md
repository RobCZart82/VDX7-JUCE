# RC3 package verification / RC3 csomagellenőrzés

Date / Dátum: 2026-10-13. Preparation only; not publication acceptance.
Csak előkészítés; nem publikálási elfogadás.

Source: `581b1ea50355754f324c8ebf5381eecfb257dc82`.
Candidate: `1.2.0-rc3`.
[Workflow 38436119848](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/38436119848).
Windows x64, macOS Universal and assembly jobs: PASS.
Source-main Windows/macOS builds: PASS (38432732355 / 38432732231).

## Verified / Ellenőrizve

- Downloaded combined preparation artifact 11832564740; no installation.
- Local SHA-256 verification: all four packages and both BUILD-INFO files PASS.
- Both Manual ZIPs contain the same corresponding-source ZIP and valid checksum:
  `cee56ad9dd7e3500a153ce66c542d55d78e48e035dedbef5a1511d5f0acb0962`.
- Source archive verifier: 5727 files PASS. Platform-specific Manual VST3 markers PASS.
- macOS VST3: arm64 + x86_64 Universal; strict deep ad-hoc signature check PASS.
- BUILD-INFO records the exact source, workflow, toolchains and dependency pins.
- CI packaging guards, ROM-free tests and Windows installer smoke steps PASS.
  This is CI evidence, not a local Windows/REAPER acceptance test.

Package SHA-256:

| Package | SHA-256 |
| --- | --- |
| Windows Manual ZIP | `c5480747630bf8d53301c814c5b6f069ce6c85269397944e514f36a224e5ee63` |
| Windows Setup EXE | `063401365e1ebfb56dda15a2880d45d68717e5179fd0f1cfa6b04f4b7dd0e465` |
| macOS Manual ZIP | `810627f930e8720cbd10dc2f3150610f2d3d489426539c866091af32a83aff5f` |
| macOS PKG | `5eecf6dd1f78d2c9d8d066177b6c14b22aaaf90841d81b256b06666d0cb9dc6f` |

## Remaining / Hátralévő

RC3 macOS plugin binary SHA-256:
`02be446a62eb7ef50b9296a45d68d749a5c92a37da6edc20fdde7db17659ffce`.
Installed plugin SHA-256 remains
`c40cb0d9731b36cb691811977bdb73acd5b108fb18da188a257d90d66cce69d3`.
They differ. Earlier REAPER smoke and owner reports do not establish exact RC3
binary acceptance. Windows/macOS host tests on these packages remain NOT RUN
in this verification round. Record OS, host, architecture, binary hash and
case-by-case results (or explicit owner-approved deferrals) before acceptance.

Az RC3 és a telepített plugin eltér. A korábbi tesztek nem zárják le az RC3
elfogadását. Az exact RC3 Windows/macOS hosttesztek ebben a körben NOT RUN;
OS/host/architektúra/binárishash és tételes eredmény vagy külön elfogadott
halasztás szükséges. A felhasználói HU/EN dokumentáció végellenőrzése és a
külön publikálási döntés továbbra is nyitott.

No plugin or DAW project changed; no tag, release or public assets created.
Unsigned Windows installer; macOS PKG lacks Developer ID signing/notarization.
Only original Yamaha DX7 Mk I v1.8 is supported; no firmware is distributed.
Documentation-only follow-ups do not relabel or rebuild this fixed RC3 source.
Any new package build needs a new recorded candidate/source identity and checks.
