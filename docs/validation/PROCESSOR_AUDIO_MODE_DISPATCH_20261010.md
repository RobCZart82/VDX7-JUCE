# Processor audio-owner és natív módkérés — 2026-10-10

Baseline: `12fdc07b807f0196ac80b671c8b96d0a9c9f94b5`, #176 után.
Ág: `feature/processor-audio-mode-dispatch`, [PR #177](https://github.com/RobCZart82/VDX7-JUCE/pull/177).
Helyileg tesztelt kód: `5ba7c9cc2cc9c6607f66bf5975507c30d2b41e04`.
A következő PR-linkes dokumentációs commit nem változtat futtatott kódot.
Szűk D5 részlépés, nem release.

## Változás

Az owner külső, már birtokolt engine-lock alatt is fogad audio-visit műveletet.
Csak a saját mutex ténylegesen birtokolt tokenje enged átadást. A pending vagy
nem ready állapot nem dispatchol a régi motorra. A self-locking teszt/API út
ugyanezt a kaput használja, nem második állapotmodellt.

A valódi processBlock a meglévő try-lock és state/host-reset megfigyelések után
a friss desired módot kéri a #176 natív adapterétől. Nincs új audio-lock,
allokáció, fájl/XML/hash, firmware warmup vagy host callback ebben a lépésben.
Nem pre-lock snapshotot publikál vissza. A mode request önmagában nem reset.

## Ellenőrzési scope

Owner-teszt: rossz és nem birtokolt lock, unloaded/pending tiltás, matching
completion, későbbi Classic recall és readiness visszavonás.
Valódi processBlock-teszt: pending Clean nem jut a motorra; ready Clean test-only
owner injekció átadódik; egy valódi másik szál engine-lockja mellett nincs
dispatch; a következő megszerzett audio-blokk a legújabb Classic értéket küldi.
A Clean injekció **nem** szállított UI vagy pozitív Clean project-admission.

A privát firmware fixture ideiglenes régi példánya hiányzott; a teszt explicit
fájllétezési kapuja FAIL-t adott, majd a helyi ZIP-ből újra kinyert, hash-ellenőrzött
eredeti 1.8-cal újrafuttattuk. SHA256:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Firmware-bájtok vagy factory bankok nem kerülnek Gitbe/CI-be.

## Nyitott kapuk

Végső helyi kódellenőrzés macOS ARM64 / JUCE 9.0.3:

- Release `vdx7_ci_checks` build és 22/22 ROM-free CTest: PASS.
- Python 94/94: PASS.
- Actual processor `--snapshot-only`, ROM nélkül és privát 1.8-cal: PASS;
  ide tartozik a fenti audio-dispatch injekció, valamint a korábbi detached
  save/admission/project-owner/stale-ROM completion regresszió.
- RelWithDebInfo célzott ASan/UBSan owner és privát actual processor
  snapshot-only: PASS. `detect_leaks=0:halt_on_error=1`,
  `halt_on_error=1:print_stacktrace=1`; nem LeakSanitizer vagy TSAN eredmény.
- Whitespace diff: PASS. Nem készült új installer vagy telepített plugin.

Cold install első új mintától unity-gain, reset/reload/mono lifecycle, Clean
writer/admission, APVTS/host restore teljes tranzakciója és SETTINGS még nyitott.
Teljes Classic/Clean hangminőség-, CPU/allokációs, gyári bank-, TSAN/leak- és
REAPER-elfogadás, új installer/source csomag nem e részlépés eredménye.
Final-head Windows/macOS/sanitizer CI és review, majd külön main-futások szükségesek.
Az 1.0.1 release/tagek/assetek, GUI és telepített plugin nem változik.
