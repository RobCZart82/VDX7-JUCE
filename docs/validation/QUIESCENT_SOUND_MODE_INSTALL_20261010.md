# Quiescent Classic/Clean motor-install — 2026-10-10

Baseline: `2d4b49c7fee2d734c9390506fd3a1d4897ca4de8` (#177 után).
Ág: `feature/quiescent-sound-mode-install`. Engine-only D5 részlépés.

## Szűk szerződés

Az `installSoundModeWhileQuiescent` owner-only, inaudible cold/project API;
nem élő UI request. A meglévő audio-reset eldobja a korábbi native/SRC historyt,
de megtartja a core EGS scan fázisát. Pending install esetén a félkész régi
mintakör a szokásos instrukcióórákkal fejeződik be, egyetlen kimenete nem kerül
az új historyba. A következő teljes 96-órás kör elején a legutolsó kívánt mód
települ unity gainnel. Régi rámpa nem folytatódik az új cold hangon.

Nincs extra firmware-instrukció, óra, warmup, boot vagy második motor. A teljes
óraszám és instrukcióhatári overshoot megmarad. A félkör kimenetének kihagyása
kizárólag explicit cold history-discard része, nem változás a normál live úton.
Normál prepare/reset/live request nem indít automatikusan quiescent installt.
A hívó quiescence-ének biztosítása a következő processor-integráció feladata;
a nyilvános belső engine API önmagában nem bizonyít ilyen host-garanciát.

## Tesztek

- Mind a 96 lehetséges scan-fázis: a régi félkör nem kapcsol félúton, nem jelenik
  meg új hangként; első új teljes minta Clean unity, pontos mintaszám.
- Félbehagyott élő rámpa cold telepítése, újabb desired, többminta/overshoot:
  régi rámpa megszűnik, legfrissebb intent nyer, összes óra megmarad.
- Actual engine native tároló kiürül, valódi scan fázis megmarad.
- Privát eredeti 1.8, saját carrier-paraméterek: Classic/Clean × install boot
  előtt/után × 44,1/48/96 kHz. Az első mintától 32 768 mintás trace a direkt,
  stabil upstream EGS-mode referenciával azonos, véges és nem nulla.
  Ez 12 eset, nem teljes factory-bank vagy perceptuális elfogadás.

Az első 2048-mintás firmware-próba nem minden rate-en érte el a MIDI/envelope
hangindulását, ezért a nem nulla guard FAIL-t adott. A végső trace 32768 minta;
továbbra is az első mintától hasonlítunk, nem dobunk el warmupot és nem lazítjuk
a nem nulla feltételt. Ez teszt-fixture időablak, nem production hibajavítás.

Firmware SHA256: `6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Yamaha bájtok/factory patch-ek nem kerülnek Gitbe vagy CI-be.

Végső helyi ellenőrzés macOS ARM64 / JUCE 9.0.3:
Release ci_checks build, 22/22 ROM-free CTest és 94/94 Python PASS.
Az új privát engine-mátrix és a korábbi live/sustain/partition regresszió PASS.
Célzott RelWithDebInfo ASan/UBSan ugyanezzel a privát firmware-rel PASS;
`detect_leaks=0:halt_on_error=1`, `halt_on_error=1:print_stacktrace=1`.
Ez nem TSAN vagy LeakSanitizer eredmény. Whitespace diff PASS.

## Nyitott kapuk

Processor cold/project quiescence tranzakció, összes host-reset/release/reload/
mono út, Clean writer/admission, APVTS restore atomikussága és SETTINGS még nyitott.
Szűrőhistory/hangminőség, CPU/allokáció, teljes gyári bank, TSAN/leak, REAPER és
új packaging elfogadása nem e részlépés állítása. Final-head platform/sanitizer
CI és review, majd külön main-kör szükséges. Nincs release/tag/asset/GUI vagy
telepített plugin változás.
