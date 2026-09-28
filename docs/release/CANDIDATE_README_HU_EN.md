# 1.0 development candidate / fejlesztői jelölt

## English

Not an accepted stable release. The host may report numeric version 1.0.0 while
the interface correctly displays 1.0.0-dev. Identify each artifact by its full
source SHA, not that version label alone. Verify source ZIP checksums and keep
its SOURCE_MANIFEST.json with the test record. Binary checksums/signature status
must be recorded separately for the exact artifact actually tested.

Primary distribution target: Windows x64 and macOS Universal VST3. AU and
Standalone compile coverage is not a claim of completed runtime acceptance.
Current macOS CI signing is ad-hoc, not Developer ID/notarisation. Windows
publisher signing is not configured. Do not disable system-wide protection.

Follow [the English guide](../guides/GUIDE_EN.md) for installation and ROM layout.
Back up projects, the USER library and the existing plugin before choosing to
install a candidate. Build/package validation itself does not install anything.
No Yamaha firmware or factory bank is supplied. The local complete test suite
requires a private combined 48 KB v1.8 fixture; product ROM support is broader.
There is one persistent 32-slot USER bank, not a named-bank library manager.

Known acceptance work: exact-SHA host matrix, interactive Settings/About/HiDPI,
offline render and tail/dense-MIDI characterization, final package/signature
review. See [the checklist](RELEASE_CHECKLIST_1.0_RC.md). Do not treat compilation,
pluginval or sanitizer PASS as complete host/audio acceptance.

## Magyar

Ez fejlesztői jelölt, nem jóváhagyott stabil kiadás. A host 1.0.0 száma mellett
a felület 1.0.0-dev jelölése szándékos. A tesztelt fájlt mindig a teljes
forrás-SHA-val, ellenőrzőösszeggel és platformmal azonosítsd.

A fő célformátum Windows x64 és macOS Universal VST3. Az AU/Standalone sikeres
fordítása önmagában nem futásidejű elfogadás. A macOS-csomag ad-hoc aláírt, nem
notarizált; Windows kiadói aláírás jelenleg nincs beállítva.

Telepítéshez kövesd a [magyar útmutatót](../guides/GUIDE_HU.md), és előtte mentsd
a projekteket, USER bankot és a meglévő plugint. A csomagkészítés nem telepít.
ROM nem része a csomagnak. Egyetlen, 32 helyes USER bank támogatott.
A végleges host-, felület- és csomagellenőrzések még külön feladatok; a sikeres
automatikus tesztek nem jelentik a stabil kiadás jóváhagyását.
