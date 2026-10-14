# Candidate guidance / Jelölt-útmutató

Publication checkpoint / Publikálási checkpoint: [v1.2.0](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.2.0)
is published stable Latest, 2026-10-14. Final owner acceptance and authorization
supersede the earlier preparation-only and pending-host statements below.
A v1.2.0 stabil Latest megjelent; a végleges tulajdonosi elfogadás felülírja
az alábbi korábbi előkészítési és nyitott host-státuszokat.

## Current v1.2.0 scope / Aktuális v1.2.0 kör

Stable Latest `1.2.0` is published from run 38548177165, source
`7e2271352d0f05fdad39bae26fdec764064fa1e1`. Final package hashes/source were
verified; the owner separately tested and accepted the final installers and
binaries on macOS/Windows and approved publication. RC3 is historical.
Only original Yamaha DX7 Mk I v1.8 firmware is officially supported; SER7/Special
Edition and other firmware versions are not officially supported. Firmware is
not supplied. Windows x64 Setup EXE + Manual ZIP and macOS Universal PKG +
Manual ZIP contain VST3 only, not AU/Standalone. Both Manual ZIPs include the
matching source archive and its checksum under `Source/`. Windows installer is
unsigned; macOS PKG is not Developer ID signed/notarized (plugin ad-hoc only).
See [final owner acceptance](../validation/FINAL_120_OWNER_ACCEPTANCE_20261014.md),
[download policy](PUBLIC_DOWNLOADS.md) and the [active plan](../development/DEVELOPMENT_PLAN.md).
Unreported exhaustive measurements are not claimed as PASS.

Megjelent a stabil Latest `1.2.0` a 38548177165 futás változatlan csomagjaival,
forrás: `7e2271352d0f05fdad39bae26fdec764064fa1e1`. A final checksum/source
ellenőrzés megtörtént; a tulajdonos külön tesztelte és elfogadta a végleges
telepítőket/binárisokat macOS/Windows alatt, és engedélyezte a publikálást.
Az RC3 történeti jelölt.
Hivatalosan kizárólag az eredeti Yamaha DX7 Mk I v1.8 firmware támogatott;
SER7/Special Edition és más firmware-verzió nem támogatott hivatalosan.
Firmware nincs mellékelve. A Windows x64 EXE + Manual ZIP és macOS Universal
PKG + Manual ZIP csak VST3-at tartalmaz, AU/Standalone-t nem. Mindkét Manual ZIP
`Source/` mappájában szerepel az azonos forrásarchívum és ellenőrzőösszege.
A Windows telepítő aláíratlan; a macOS PKG nem Developer ID-aláírt/notarizált
(a plugin csak ad-hoc aláírt). A fenti bizonyíték, letöltési szabály és aktív
terv az irányadó. Nem közölt teljes körű műszeres mérést nem állítunk PASS-nak.

## Historical 1.0.1 preparation / Történeti 1.0.1 előkészítés

The text below is historical, not current firmware policy or release tasks.
Az alábbi szöveg történeti, nem aktuális firmware-szabály vagy feladatlista.

> **2026-10-05 publication checkpoint:** [1.0.1 is published](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1), with [matching source](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source).
> Az 1.0.1 megjelent. [Current acceptance, owner-authorized deferrals and publication record](../validation/VALIDATION_20261005_PUBLICATION_101.md) supersede earlier open publication gates below. Earlier candidate/test statements are retained as dated preparation history, not current release status.

## English

Not an accepted stable release. The host may report numeric version 1.0.1 while
the interface correctly displays 1.0.1-dev. Identify each artifact by its full
source SHA, not that version label alone. Verify source ZIP checksums and keep
its SOURCE_MANIFEST.json with the test record. Binary checksums/signature status
must be recorded separately for the exact artifact actually tested.

For an exact 1.0.1-rcN build, use the non-publishing release-candidate workflow
and its explicit `candidate_label` input; it builds with the matching RC display
version. Ordinary development builds remain `1.0.1-dev`.

Primary distribution target: Windows x64 and macOS Universal VST3. AU and
Standalone compile coverage is not a claim of completed runtime acceptance.
Windows builds have no publisher signature. macOS builds carry only a technical
ad-hoc signature, not Developer ID/notarisation. The OS may show a warning or
make loading more difficult. Do not disable system-wide protection; see the guide.

Follow [the English guide](../guides/GUIDE_EN.md) for installation and ROM layout.
Back up projects, the USER library and the existing plugin before choosing to
install a candidate. Build/package validation itself does not install anything.
No Yamaha firmware or factory bank is supplied. The local complete test suite
requires a private combined 48 KB v1.8 fixture; current official support is v1.8 only.
There is one persistent 32-slot USER bank, not a named-bank library manager.

Earlier 1.0.0 owner reports do not accept this 1.0.1 candidate.
Known acceptance work: exact-SHA host matrix, interactive Settings/About/HiDPI,
offline render and tail/dense-MIDI characterization, final package/signature
review. See [the checklist](RELEASE_CHECKLIST_1.0_RC.md). Do not treat compilation,
pluginval or sanitizer PASS as complete host/audio acceptance.

## Magyar

Ez fejlesztői jelölt, nem jóváhagyott stabil kiadás. A host 1.0.1 száma mellett
a felület 1.0.1-dev jelölése szándékos. A tesztelt fájlt mindig a teljes
forrás-SHA-val, ellenőrzőösszeggel és platformmal azonosítsd.

A pontos 1.0.1-rcN buildhez a kiadást nem publikáló jelölt-workflow-t és a
`candidate_label` mezőt használd; a felületen is a megfelelő RC-verzió jelenik
meg. A szokásos fejlesztői buildek továbbra is `1.0.1-dev` jelzésűek.

A fő célformátum Windows x64 és macOS Universal VST3. Az AU/Standalone sikeres
fordítása önmagában nem futásidejű elfogadás. A Windows-csomagon nincs kiadói
aláírás; a macOS-csomag technikai ad-hoc aláírást kap, de nem Developer ID-
aláírt és nem notarizált. Az operációs rendszer figyelmeztethet vagy
megnehezítheti a betöltést; a rendszer egészére kiterjedő védelmet ne kapcsold ki.

Telepítéshez kövesd a [magyar útmutatót](../guides/GUIDE_HU.md), és előtte mentsd
a projekteket, USER bankot és a meglévő plugint. A csomagkészítés nem telepít.
ROM nem része a csomagnak. Egyetlen, 32 helyes USER bank támogatott.
A végleges host-, felület- és csomagellenőrzések még külön feladatok; a sikeres
automatikus tesztek nem jelentik a stabil kiadás jóváhagyását.
A korábbi 1.0.0 felhasználói tesztek nem igazolják ezt az 1.0.1 jelöltet.
