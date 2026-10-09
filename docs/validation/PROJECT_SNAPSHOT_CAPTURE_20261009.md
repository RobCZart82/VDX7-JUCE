# Projektmentés rögzített paraméterértékei

Dátum: 2026-10-09. Baseline: `31d39032c4d8aa53a88a0cee1003e47b0bdbeec2`.
Ág: `fix/coherent-project-snapshot`. Ez a Classic/Clean integráció előtt
szükséges mentési javítás, nem új release vagy kész Clean funkció.

## Reprodukált hiba

A valódi `getStateInformation()` leválasztotta a RAM-ot, bankot, routingot és
firmware-azonosságot az engine mutex alatt, majd a hostparaméter-fát később
olvasta. Betöltött firmware mellett a 145 hangszínértéket már a rögzített
motoradatokból javította ki, de a három hangerő/kerék értéket nem. Firmware
előtt a hangszín-paraméterek is a későbbi élő APVTS állapotából származtak.
Így egy capture utáni módosítás vagy project restore összekeverhette a mentés
korábbi payloadját a későbbi paraméterekkel.

A `VDX7ImportedProcessorTests.cpp` meglévő, kizárólag a teszt-executable-be
fordított save-boundary hookja determinisztikusan a capture és az encoding
között szerkeszt, illetve valódi `setStateInformation()` restore-t végez.
A változatlan production kódon az új ROM-free teszt eredménye:

```text
FAIL: save must not read parameter values from a later edit or project restore
```

## Javítás és regressziók

Mind a 148 paraméter a capture során rögzített értékkel kerül a leválasztott
APVTS másolatba. Loaded állapotban a hangszín továbbra is a motor/RAM
pillanatképéből származik, no-ROM állapotban a stabil paraméter-pointerek
atomikus értékeiből. A hangerő, pitch és mod wheel mindkét esetben rögzül.
A normal-path APVTS másolás és XML/base64 encoding továbbra is a mutexen
kívül történik; a pending projekt megőrzési útja nem változik.

A teszt mind a 148 mentett értéket összeveti a capture előtti kontrollal,
valódi bináris recallt végez egy másik processor-példányban, és ellenőrzi,
hogy az eredeti példány későbbi routing/hangerő változása élőben megmarad.
Firmware nélkül külön kör ellenőrzi a már pending projektet is.
A ROM-free kontroll a meglévő `vdx7_imported_bank_state` teszt része, így
Windows/macOS CI és az ASan–UBSan workflow is futtatja; nincs új inventory
vagy workflow-szűrő. Az opcionális `--snapshot-only <private ROM>` csak a
célzott loaded kontrollt futtatja, nem igényel kombinált factory-bank ROM-ot.

## Ellenőrzések

Helyi macOS ARM64, AppleClang `21.0.0.21000334`. JUCE:
`be29c81492b6151c8ea8d14c840e1311963b3a83` (9.0.3); dx7Lib upstream:
`d5473776a0449d60a997b91bdc888598a33265ac`.

- Javítás előtti új ROM-free reprodukció: FAIL, CTest exit 8.
- Javítás utáni teljes ROM-free CTest: **21/21 PASS**.
- Python regressziók: **94/94 PASS**, nincs SKIP.
- `vdx7_ci_checks`, macOS ARM64 VST3/AU/Standalone dev-build: PASS.
- CTest tényleges ROM-free leltár és checker self-test: PASS, kilenc negatív kontroll.
- Célzott no-ROM/pending processor-snapshot ASan–UBSan: PASS.
- Célzott loaded snapshot és valódi bináris recall: Release és ASan–UBSan PASS.
  A privát 16 384 bájtos v1.8 fixture SHA256-ja
  `6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
  Sem firmware-bájt, sem factory bank nem kerül Gitbe vagy csomagba.

Az ASan kör `detect_leaks=0`, `halt_on_error=1`; UBSan
`halt_on_error=1:print_stacktrace=1`. Ez nem LeakSanitizer vagy ThreadSanitizer
eredmény. A célzott helyi sanitizer nem a teljes komponensmátrix futtatása.
A végleges PR-fej Windows/macOS/komplett sanitizer CI-je és review-ja külön
szükséges a beolvasztás előtt; a baseline zöld eredménye nem e javításé.

## Megmaradó korlátok

A javítás a capture **utáni** értékkeveredést zárja ki. Az egyes atomikus
paraméterolvasások nem teszik a capture közben érkező önálló automatizációs
írásokat vagy a teljes project restore-t egyetlen atomikus tranzakcióvá.
A valódi desired-mode/payload/pending/ROM-epoch tulajdonlás még a
[fejlesztési terv](../development/DEVELOPMENT_PLAN.md) nyitott D5 feladata.

A Clean reader fail-closed marad; nincs új mód-writer, SETTINGS kapcsoló vagy
hostparaméter. Nincs új DSP/lifecycle vagy audio-callback módosítás.
Classic hangzási/null-difference mérés, valódi REAPER/hallásos és új release
csomagelfogadás e körben NOT RUN. A dev-buildek nincsenek telepítve;
publikált 1.0.1 tag, source és asset változatlan.
