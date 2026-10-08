# D5 állapot- és kérésrendezési szerződésmodell — validáció

Dátum: 2026-10-08. Baseline main:
`0a8b1bdaa5e99652b4aa2a1958dca04e9ba7f152` (#163 merge után).
Ág: `test/classic-clean-state-contract`.
[Műszaki szerződés és production belépési pontok](../development/CLASSIC_CLEAN_STATE_CONTRACT.md).

## Hatókör

Új `Tests/VDX7SoundModeStatePrototype.h`, a meglévő ROM-mentes
`vdx7_sound_mode_prototype` tesztbe kötve. Saját string/enum adat, sem Yamaha
firmware/bank, sem valós DAW-projekt nem kell. A modell egyetlen soros tulajdonost
feltételez; nem használ valódi mutexet/threadet/JUCE XML-t vagy binary codecet.
Nem változtat production processoron/engine-en, paramétereken, SETTINGS-en,
projektformátumon vagy release/tag/asseten. Minden új mező/API egyelőre jelölt.

## Vizsgált modellelvek

- Mindkét property hiánya → legacy Classic, korábbi Clean állapot után is.
- Pontos feature-version `1` és mode `0`/`1`; hiányos pár, üres/hibás szöveg,
  ismeretlen verzió elutasítása. Az elutasított restore nem változtatja a modell
  desired/revision/pending/engine-ready pillanatképét. Production teljes RAM/
  bank/paraméter transaction-megőrzése **nem** ebből a modellből bizonyított.
- ROM nélkül és pending projektben a kívánt Clean mentése, renderer-dispatch
  nélkül. Compatible completion után a legutolsó elfogadott kívánt érték kérhető.
  A kompatibilitás itt külső eredmény, nem ROM-azonosító vizsgálat.
- Korábbi projektből származó UI revision elutasítása recall után. Current
  revision kérései coalescelnek; invalid enum elutasítva. Revision overflow
  mutation nélküli elutasítás, nincs stale-token újrafelhasználás.
- Régi restore-completion token nem teheti readyvé az új pending projektet.
- Pending mode edit menthető, de nem dispatcholható az unrelated renderernek;
  engine elvesztése nem veszi el a kívánt értéket. Két modellpéldány izolált.
- Save-before-audio és save-during-ramp a **kívánt** Clean-t adja, miközben a
  teszt-only átmenet még Classic-ban halkít. Átmeneti gain/aktív mód nem payload.

## Reprodukció

```text
cmake --build <build> --config Release --target vdx7_ci_checks
ctest --test-dir <build> -C Release -L rom-free --output-on-failure --no-tests=error
<build>/Release/vdx7_sound_mode_prototype_tests
<build>/Release/vdx7_sound_mode_prototype_tests --stale-ui-negative-control
python -m unittest discover -s Tests -p "test_*.py" -q
python scripts/check_test_registration.py --self-test
```

A stale-UI negatív adapter szándékosan lecseréli a régi token eredeti revisionjét
a mostanira, mintha a UI automatikusan újrapróbálná a régi Clean-szándékot.
Elvárt **FAIL/exit 1**:
`a UI token from before project recall cannot overwrite the recalled mode`.
Ez mutatja, hogy a teszt valóban védi a revision-admissiont; nem production bug.
A korábbi unmuted-switch és ignore-clean mód ugyanúgy az elvárt határ/OPS
ellenőrzésnél ad FAIL/exit 1. A hibás kontrollok nem normál CTestek.

## Eredmény és még nem bizonyított rész

PASS helyben: Windows MSVC Release teljes CI-tesztcél build, 18/18 ROM-mentes
CTest; mindhárom negatív kontroll elvárt hibája; tesztleltár önellenőrzés és
tényleges ROM-mentes inventory; diff-formaellenőrzés. Python: 78 futott,
77 PASS / 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR.

NOT RUN: új final-head Windows/macOS/sanitizer CI és review (PR után
ellenőrizendő); typed JUCE property/binary codec; valódi processor-lock/BUSY/
atomic/race/reentráns save; teljes snapshot rejection; native/SRC mode/gain
ordering és lifecycle; privát eredeti v1.8 firmware, sűrű MIDI/sustain/tail/mono,
SETTINGS, callback-allokáció/CPU, REAPER és hallásos elfogadás.

Az `engineReady` ebben a modellben compatible-current-project readiness, nem
egy valódi `isRomLoaded()` getter. A methodnevek és proposed schema nincs még
szállítva. A következő kör a valódi JUCE codec/processor-integráció tesztje;
utána engine/native ordering és firmware-lifecycle, majd SETTINGS UI. A Classic
alapvonal megőrzése és a végleges v1.8-only firmware-döntés változatlan.
