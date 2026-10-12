# D5 interrupted-reset adapter matrix

Dátum: 2026-10-12. Baseline main:
`58cebee45c6e22452131d74ecbea568c6b950549` (#182 merge).
Ág: `test/sound-mode-release-lifecycle`.

## Scope

Tesztbővítés, production kód módosítása nélkül. A korábbi reset-adapter teszt
csak állandó gain mellett kért új intentet. Az új ROM-free ScanProbe mátrix:
96 scan-fázis × 2 irány × 2 fade-pozíció (128/384 scan) × 2 cancellation = 768.
Precondition igazolja a valódi megszakított fade-out/fade-in állapotot.
EGS reconstruction után latest intent, unity gain, phase 0, három teljes új
scan és 17 overshoot clock ellenőrzött. Nincs régi rampából származó gain.

Ez a production adapter API kontrollált tesztje, nem firmware-es host-reset,
processor release/prepare integráció, hallásos vagy REAPER-bizonyíték.
Nem nyitja meg a Clean reader/writer/SETTINGS kaput. Új hibát nem bizonyított.

## Eredmények

- PASS: Windows Release native-sound-mode teszt build és 768 új eset.
- PASS: ROM-free CTest 22/22 a munkaterületi TEMP/TMP könyvtárral.
  Az első, alapértelmezett TEMP futás 16/22 PASS, 6 fájlírási FAIL volt;
  ugyanazon binárisok az írható tesztkönyvtárral mind átmentek. Nem kódjavítás.
- PASS: dokumentáció/dependency guard 3/3 és diff whitespace ellenőrzés.
- PASS: a `--reset-negative-control` kihagyott reconstruction restore mellett
  pontosan az új unity/intent assertionnél FAIL exit 1-et ad; ez elvárt kontroll.
- PASS: korábbi `--mid-scan-negative-control` továbbra is elvárt FAIL exit 1.
- NOT RUN: új ág macOS/sanitizer CI, privát firmware és telepített REAPER teszt.

#182 final-head Windows/macOS/sanitizer mind PASS és beolvadt; merge utáni
main Windows/macOS CI PASS. Az új tesztág CI eredménye nem helyettesíthető
ezzel a történeti PASS-szal.
Irányadó sorrend: [egységes terv](../development/DEVELOPMENT_PLAN.md).
