# VDX7 1.0.0-dev — development build / fejlesztői változat

## Current status / Aktuális állapot (2026-09-21)

Implemented since the initial snapshot below: persistent 32-slot USER bank and
Save As, firmware-backed PERFORMANCE controls, SETTINGS tuning/channel filter,
windowed-sinc resampling, warm GUI and vector-branded About. See README.md and
README_HU.md for current usage. Normal CI and exact-commit candidates share
`vdx7_ci_checks`: five ROM-free tests plus a compile-only stress runner.
Candidates target macOS Universal and Windows x64; compilation is not host acceptance.

Az alábbi induló állapot óta elkészült a 32 helyes USER-bank és Save As,
a firmware-alapú PERFORMANCE, a SETTINGS hangolás/csatornaszűrés, a sinc resampling,
a meleg tónusú GUI és a vektoros About. Aktuális használat: README_HU.md.
A CI/kiadásjelölt közös tesztcélt használ; Universal és Windows x64 fordítás
nem helyettesíti a hosttesztet. Nyitott: párhuzamossági/MIDI vizsgálatok,
GUI-finomságok, teljes platformelfogadás és végső csomagolás. Nincs végleges release.

## Historical initial stabilization snapshot / Történeti induló állapot

The sections below describe the starting milestone only; their pending-feature
list and commit IDs are historical, not the current main or acceptance evidence.
Az alábbi hátralévő funkciók és commitok az induló mérföldkőhöz tartoznak,
nem a jelenlegi main állapotát vagy annak elfogadottságát jelölik.

## Magyar

Ez nem végleges 1.0.0 kiadás. A kiadott v0.6.6 változat és csomagjai változatlanok.
A fejlesztés alapja a main 483daf7 és a korábbi, helyi 80ebf54 MIDI-tartomány-bővítés.
A végleges kiadás feltételei a ROADMAP_1.0.md fájlban szerepelnek.

Az első stabilizáló változtatások:

- Hiányzó ROM esetén a projekt hangszíne megmarad; a kompatibilis ROM kézi
  megadása után helyreáll. ROM nélküli újramentés/újranyitás sem dobja el.
- Rossz méretű ROM betöltési kísérlete nem állítja le a működő hangszert.
- CC64/65: 0–63 kikapcsolás, 64–127 bekapcsolás; CC11 relatív hangerő.
- Átmenetileg foglalt engine mellett a MIDI-eseményeket korlátos puffer őrzi;
  túlcsorduláskor biztonsági hangelengedés történik. Az érintett audióblokk
  továbbra is néma lehet: a teljes valós idejű architektúra-ellenőrzés még hátravan.
- Élő, érvényes 4104 bájtos bank-SysEx frissíti a kijelzett és host-állapotot.
  Más élő SysEx üzenet jelenleg elutasított; egyhangszínes fájlimport megmarad.
- MIDI bankváltás ugyanazt a belső bankot módosítja, amelyet a GUI szerkeszt.
- A korábbi 512 mintás előrerenderelés helyett a következő EGS-mintáig fut az engine.
- Teljes 0–127 MIDI-bemenet, továbbra is egy hangszer, omni csatornakezeléssel.

Nem kész még: PERFORMANCE/SETTINGS, jobb minőségű resampling, teljes host-
és platformelfogadás, végleges kiadási csomagolás. A végleges 1.0.0 nincs publikálva.

Teszteléshez először készíts biztonsági másolatot projektjeidről és saját bankjaidról.
Csak a teljes VDX7.vst3 csomagot másold a felhasználói VST3 mappába, bezárt
gazdaprogram mellett. macOS: ~/Library/Audio/Plug-Ins/VST3/; Windows:
C:\Program Files\Common Files\VST3\. Ne legyen két azonos VDX7 példány a
gazdaprogram keresési útvonalain. A régi plugin megőrzése ajánlott a keresési
útvonalon kívül. Az alapértelmezett helyi build nem telepít automatikusan.

macOS-en az aláírás ad-hoc, nem Developer-ID/notarizált. Csak megbízható csomagnál,
ha a karantén akadályozza a betöltést, kizárólag a telepített pluginra:

    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/VDX7.vst3"

Saját kompatibilis DX7 firmware szükséges. Yamaha firmware és gyári ROM-adat
nem kerülhet a GitHubra vagy a kiadási csomagokba. Licencek: LICENSE.txt,
NOTICE.md, THIRD_PARTY.md, LICENSES/. Forrásfüggőségek: SOURCE_DEPENDENCIES.md.

## English

This is a development build targeting stable 1.0.0, not the final release.
Existing v0.6.6 releases remain unchanged. The branch combines reviewed main
483daf7 with local full-range MIDI development 80ebf54. See ROADMAP_1.0.md.

Initial stabilization covers deferred project recall while firmware is missing,
non-destructive invalid-ROM rejection, correct CC64/65 thresholds, relative CC11
expression, bounded MIDI retention during engine transactions, validated live
bank SysEx with coherent editor/host state, consistent CC32 internal bank selection,
and demand-driven native sample generation instead of 512-sample lookahead.
Full-range 0–127 MIDI input remains single-part omni, not multitimbral/MPE.

On queue overflow, a safety note release replaces the discarded event batch.
An engine transaction can still silence an audio block; full realtime ownership
and notification auditing remains outstanding. Only complete 4104-byte bank dumps
are accepted live. Single-voice import remains available through files.

PERFORMANCE/SETTINGS, higher-quality resampling, complete host/platform acceptance,
and final release packaging remain unfinished. No final 1.0.0 is published.

Back up projects and edited banks before testing. With the host closed, install
the entire VDX7.vst3 bundle into ~/Library/Audio/Plug-Ins/VST3/ on macOS or
C:\Program Files\Common Files\VST3\ on Windows. Keep old versions outside the
host scan paths; do not leave duplicate plugin IDs in scanned locations. The
default local build does not install automatically.

macOS builds are ad-hoc signed, not Developer-ID signed or notarized. For a trusted
download only, if quarantine blocks loading, remove quarantine specifically from
the installed bundle using the command above. Windows requires the current x64
Microsoft Visual C++ v14 runtime, as documented by the v0.6.6 release.

Supply your own compatible DX7 firmware. Never upload firmware/factory ROM data.
License and component notices are in LICENSE.txt, NOTICE.md, THIRD_PARTY.md and
LICENSES/. Dependency revisions are recorded in SOURCE_DEPENDENCIES.md.
