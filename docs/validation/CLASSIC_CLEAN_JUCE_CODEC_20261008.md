# D5 JUCE állapotkódoló — validáció és ChatGPT Work / Codex átadás

Dátum: 2026-10-08. Repository: RobCZart82/VDX7-JUCE.
Baseline main: `328d93992a764dbd1ac89a50feb3ad4e1e2692b3`.
Ág: `test/classic-clean-juce-state-codec`.
Irányadó [fejlesztési terv](../development/DEVELOPMENT_PLAN.md),
[állapot-/ownership szerződés](../development/CLASSIC_CLEAN_STATE_CONTRACT.md).

## Előző mérföldkő lezárása

#164 final head `5386aafc049f2e43a1375ac4e5134651e956fec7`:

- PASS: [Windows 37766081260](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37766081260).
- PASS: [macOS 37766081404](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37766081404).
- PASS: [ASan/UBSan 37766081269](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37766081269).
- PASS: merge előtt pontos head, mergeability, üres review thread/review lista ellenőrzése.
- PASS: [#164 merge](https://github.com/RobCZart82/VDX7-JUCE/pull/164), main a fenti baseline.
- Folyamatban a merge után indult main Windows `37768437463` és macOS
  `37768437555` az ellenőrzéskor; ez nem e codec CI-eredménye, nem PASS még.

## Változás és bizonyított határ

`Source/VDX7SoundModeState.h` önálló, nem aktivált komponens. Pontos root és
`soundModeVersion=1` / `soundMode=0/1` ellenőrzés int/int64/string típuson;
mindkettő hiánya legacy Classic. Bool/double/void/array/object/binary,
prefixes/string whitespace/unknown version és fél pár elutasítva, caller mode
és tree változatlan. Írás csak captured kívánt enumot tesz deep detached
copy-ba. Nem kerül gain, active mode, revision, UI token vagy átmenet a state-be.

`Tests/VDX7SoundModeStateTests.cpp` valódi JUCE ValueTree, `createXml/fromXml`,
`AudioProcessor::copyXmlToBinary/getXmlFromBinary` használatával tesztel.
Mindkét mód, legacy Clean után, részleges/hibás binary metaadat és rossz root
lefedett. Az opaque, szintetikus fixture teljes root/child payloadja és minden
6144 RAM-byte megőrzött; nem valódi Yamaha tartalom és nem teljes valid projekt.
Writer output child módosítása sem érinti az eredetit; pending copy régi
kívánt módjának felülírása sem módosítja az input snapshotot.

XML scalarra alakít: egy eredetileg bool `true` az XML körút után string `1`
lehet. Ekkor a dekódolt string séma szerint érvényes, míg in-memory bool
elutasított. Az eredeti típusprovenance rekonstrukciója nem biztosított.

PluginProcessor/VDX7Engine/SETTINGS nincs módosítva vagy helperhez kapcsolva.
Nincs working Clean metaadat elfogadási/mentési ígéret a mai pluginban.
Nincs új ismert production bug állítás; a negatív kontrollok szándékosan
hibás test-only adapterek, nem az aktuális plugin hibareprodukciói.

## Helyi eredmények

Környezet: Windows x64, MSVC 17.14.60, Release konfiguráció; a repository
rögzített JUCE `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` függősége.
Buildkönyvtár: `C:/Users/gyuriczar/Documents/Codex/build-imported-ui-20261008`.

| Ellenőrzés | Státusz | Megjegyzés |
|---|---|---|
| Első új teszt fordítása | FAIL, javítva | Test-adapter `read` ADL névütközés; átnevezve `readUnderTest`-re. Nem production hiba. |
| Új codec cél és teljes `vdx7_ci_checks` build | PASS | MSVC újrafordítás után |
| ROM-mentes CTest | PASS | 19/19, 0 FAIL |
| Normál codec futtatás | PASS | exit 0 |
| Permisszív scalar/pair adapter | PASS kontroll | Elvárt FAIL / exit 1: `invalid pair/root must be rejected, not coerced or defaulted` |
| Shallow ValueTree-copy adapter | PASS kontroll | Elvárt FAIL / exit 1: `writer must not mutate captured/shared tree` |
| Leltár checker self-test + tényleges ROM-free inventory | PASS | Új teszt, timeout és ROM-free címke szerepel |
| Python regresszió | PASS, részleges lefedettség | 78 futott: 77 PASS, 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR; a SKIP nem tesztelt siker |
| Új rész-PR final-head Windows/macOS/ASan/UBSan | NOT RUN | Push után ellenőrizendő, lokális sanitizer nincs |
| Production processor/DSP/UI integráció és valódi thread race | NOT RUN | E kör nem végzi |
| Privát v1.8 és REAPER/hallásos teszt | NOT RUN | Nem indítottunk REAPER-t és firmware-t sem használtunk |

## Reprodukció

1. Konfigurálás ROM-free módban (`VDX7_ENABLE_ROM_TESTS=OFF`) a rögzített
   függőségekkel, build `vdx7_ci_checks`, Release.
2. `ctest --test-dir <build> -C Release --output-on-failure --no-tests=error -L rom-free`.
3. `<build>/Release/vdx7_sound_mode_state_tests` → exit 0.
4. Ugyanez `--permissive-read-negative-control`, majd
   `--shallow-copy-negative-control` → mindkettő elvárt exit 1. Nem normál CTest
   sikertelen futások: opt-in regresszióérzékenységi kontrollok.
5. `python -m unittest discover -s Tests -p 'test_*.py' -q`, majd
   `scripts/check_test_registration.py --self-test` és a tényleges CTest JSON
   leltár `--rom-free-only` ellenőrzése.

Az új ROM-free cél a normál Windows/macOS CI aggregate és a sanitizer build/
CTest regex része. A Python workflow-teszt ellenőrzi mindkét admissiont;
ez önmagában nem ASan/UBSan runtime PASS.

## Következő fejlesztőnek

1. E rész-PR végleges head CI/review; csak zöld, rendezett PR mergelhető.
2. Processor owner tranzakciókat és determinisztikus UI/restore/save/audio
   race-regressziókat készíts. Valódi pending/current-revision admissiont, teljes
   projektelutasítás mutation nélkül, reentráns save és helyes try-lock határt
   bizonyíts. A codec nem mutex/mailbox és nem teljes restore-validátor.
3. Native mode/gain ordering, instruction overshoot, SRC history és lifecycle
   integráció, majd privát eredeti v1.8 Classic null-difference és terhelésmérés.
4. Csak ezután SETTINGS UI és végleges production mező-aktiválás; a részkomponens
   elkészülte nem engedély DSP nélküli working Clean jelzésre.
5. Valódi hostmátrix és loudness-matched hallásos elfogadás továbbra is nyitott.

Csak eredeti DX7 Mk I v1.8 támogatott; SER7 és minden más firmware tartósan
nem támogatott. Nincs Yamaha ROM/bank/audio a commitban, nincs paramétersorrend/
pluginazonosító vagy release/tag/asset változtatás, nincs publikálási engedély.
