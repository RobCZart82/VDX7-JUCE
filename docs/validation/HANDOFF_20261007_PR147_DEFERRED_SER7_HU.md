# A 147 számú pull request és a halasztott SER7 diagnosztika átadása

Dátum: 2026-10-07. Címzett: a VDX7-JUCE fejlesztését folytató ChatGPT Work és Codex.
Ellenőrzött main: `9ed484f893db6f14e3dfb1b5e8bca9a0b36f9fce`.

## 2026-10-08: végleges támogatási döntés, a további SER7-munka visszavonva

A felhasználó végleges döntése szerint hivatalosan kizárólag az eredeti Yamaha
DX7 Mk I v1.8 (IG11469) támogatott. Minden más firmware-verzió, köztük a SER-7
és a módosított képek nem támogatottak, és a jövőben sem lesznek támogatottak.
Az [egységes fejlesztési terv](../development/DEVELOPMENT_PLAN.md) vezeti ezt a döntést.

Az alábbi 2026-10-07-es átadás és teszteredmények történeti bizonyítékok.
A benne szereplő „halasztott munkacsomag”, „folytatásakor” és SER7-támogatási
teendők nem aktív utasítások: ez a döntés felülírja őket. A diagnosztika
megőrzendő, de SER7-támogatást nem fejlesztünk. Ez nem általános műszaki
lehetetlenségi állítás és nem betöltési tiltás.

### PR-rendezési eredmény — 2026-10-08

A felhasználó külön kérésére a [#147-et](https://github.com/RobCZart82/VDX7-JUCE/pull/147)
beolvasztás nélkül lezártuk. Az eredeti védelem #149-cel már main-ban van;
a megmaradt SER7-kutatás nem része a végleges fejlesztési koncepciónak.
A `test/firmware-reset-vector-admission` ág és az alább azonosított head
megmarad, nincs törlés vagy history-átírás. A lezárási magyarázat a PR
leírásának elején és külön hozzászólásban is megtalálható; az eredeti leírás
megőrzött történeti checkpoint. A [#159](https://github.com/RobCZart82/VDX7-JUCE/pull/159)
dokumentálja a támogatási döntést és ezt az eredményt. Az alábbi „nyitott
Draft”, „folytatás” és PR-bezárásra vonatkozó mondatok a régi állapotot írják le.

## Eredeti átadás — 2026-10-07, történeti állapot

**A [147 számú PR](https://github.com/RobCZart82/VDX7-JUCE/pull/147) teljes beolvasztása nem javasolt.**
A PR nyitott Draft, és a halasztott SER7-diagnosztika kódját, kontrolljait és
történeti eredményeit őrzi. Az általános firmware reset-cím védelem és regressziói
már külön, a [149 számú PR](https://github.com/RobCZart82/VDX7-JUCE/pull/149) révén
bekerültek a main-ba. A zöld CI nem teszi a SER7-et támogatott firmware-ré.

## A main ágba már beolvadt védelem

A 149 számú PR merge commitja: `c4bf603099eb6f6c4097715aca0017975e61c20e`.
A [firmware-validátor](../../Source/VDX7FirmwareValidation.h) a 16 KB-os kép
big-endian reset-vektorát ellenőrzi: a C000..FFFF ROM-tartomány alá mutató
cím elutasítandó a futó motor módosítása előtt. A közvetlen motor- és
processor-regressziók a bemeneti védelmet, hibaszöveget és állapotmegőrzést
ellenőrzik; a címhatárokhoz 65 536 szintetikus kontroll tartozik.

Ez strukturális védelem, nem teljes firmware-boot vagy hangzásvalidáció.
ROM-ba mutató reset-cím mellett sérült végrehajtható kód továbbra is lehetséges.
Az egész 147 számú ág beolvasztása nem szükséges ehhez a már meglévő védelemhez.

## A Draft PR szerepe

A 147 számú ág: `test/firmware-reset-vector-admission`.
Ellenőrzött head: `bac64d8596a498b4c3fa34c8437391018a6d2adb`.
Az ág a már leválasztott firmware-védelmen túl a kézi opt-in
`Tests/VDX7FirmwareAudioTests.cpp` diagnosztikát és SER7-kontrollokat tartalmazza.
Ezek nem azonosak a main által támogatott funkciókkal.

A [148 számú dokumentációs PR](https://github.com/RobCZart82/VDX7-JUCE/pull/148)
rögzítette az elsődleges irányt: **ajánlott és támogatott firmware az eredeti
DX7 Mk I v1.8 (IG11469); a SER7 jelenleg nem támogatott és nem ajánlott**.
Az original/v1.8 stabilitása elsődleges. A SER7 támogatása külön, halasztott
munkacsomag, nem a következő kiadás feltétele.

## Megőrzendő tesztbizonyítékok

Ezek korábbi helyi komponensfuttatások eredményei, nem új futtatások ebben az
átadási körben. A wrapper frissen fordult, de a változatlan korábbi Release
core könyvtárral linkelt; ez nem teljes új pluginbuild.

- PASS: original maskrom, v1.8 és SER7 betöltése a reset-cím admission-védelemmel,
  valamint RAM-megőrzés hibás új betöltés után. A betöltési PASS önmagában nem
  bizonyít működő hangkimenetet.
- PASS: original és v1.8 szintetikus single-carrier hangszínnel, 48 kHz-en
  mérhető note-on hang és note-off utáni lecsengés.
- FAIL: SER7 ugyanebben a kontrollban nem adott mérhető note-on hangot;
  a nyers MIDI mind a 16 csatornáján néma maradt. A v1.8/original nyers
  channel 0 pozitív kontrollja PASS.
- NOT RUN: teljes SER7-funkciómátrix, friss teljes plugin-integráció,
  valódi REAPER-hangzás és projekt-visszatöltési elfogadás.

A SER7 némaságának oka nincs igazolva. Ez a kontroll nem általános bizonyíték
arra, hogy a SER7 semmilyen körülmények között nem működhet az emulátorral.
A privát firmware-ek és bankok nem kerülhetnek commitba, PR-ba, CI-ba vagy csomagba.

## Utasítások a következő fejlesztési körhöz

1. Ne állítsd Ready for review állapotra és ne olvaszd be a 147 számú PR-t
   kizárólag a zöld pipák miatt. Ez az átadás nem beolvasztási engedély.
2. A már main-ba került firmware-védelmet ne implementáld újra. Új munkához
   friss main SHA-t ellenőrizz, és külön ágat, tesztet, PR-t használj.
3. A SER7 diagnosztikát őrizd meg. Folytatásakor különítsd el a bank/RAM
   telepítés, a firmware MIDI-fogadás és a firmware-specifikus címkezelés
   vizsgálatát; ne módosíts memóriacímeket feltételezés alapján.
4. SER7-támogatási állításhoz igazold a hangkimenetet, PERFORMANCE-beállításokat,
   hangszínváltást és projektmentés/visszatöltést, majd végezz DAW-ellenőrzést.
5. Az aktív mérföldköveket az [egységes fejlesztési tervben](../development/DEVELOPMENT_PLAN.md)
   vezesd. Ez az átadás a PR kezelését tisztázza, nem egy második fejlesztési terv.

A PR bezárása, ágának törlése vagy release-publikálás nem része ennek az átadásnak.
