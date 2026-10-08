# VDX7 egységes fejlesztési terv

Frissítve: 2026-10-08. Ez az egyetlen irányadó lista az 1.0.1 utáni munkákhoz.
Az 1.0.1 már megjelent; a régi kiadási kapuk nem új nyitott feladatok.
A következő kiadás verzióját és pontos funkciókörét később rögzítjük.
Ez a dokumentum nem új kiadás publikálási engedélye.

## Végleges firmware-támogatási döntés — 2026-10-08

Felhasználói döntés: **hivatalosan kizárólag az eredeti Yamaha DX7 Mk I v1.8
(IG11469) támogatott. Minden más firmware-verzió, köztük a Special Edition /
SER-7 és a módosított képek nem támogatottak, és a jövőben sem lesznek támogatottak.**
Az eredeti koncepció szerinti fejlesztést v1.8-ra hangolva folytatjuk;
minden más firmware működése és stabilitása nem biztosított. Ez nem az eredeti
v1.8 hibamentességének vagy minden host/OS kombináció elfogadásának állítása.
Ez végleges projektirány, nem ideiglenes vagy halasztott kompatibilitási cél.
A SER7 és a kettős firmware-támogatás kikerül a jövőbeli fejlesztési körből;
a korábbi diagnosztika és PASS/FAIL/NOT RUN bizonyítékok történeti anyagként
megőrzendők, nem aktív mérföldkövek vagy kiadási kapuk.

Az új motorjavítások, a firmware-függő RAM/címprofilok, projekt-visszatöltés és
kiadási elfogadás támogatott alapja az eredeti v1.8. Más firmware betölthetősége
nem jelent támogatást vagy későbbi támogatási ígéretet. A döntés nem ír elő új
firmware-engedélyezési listát, nem módosítja a betöltőt, a régi projektállapotokat
vagy a már publikált csomagokat; nem állítja, hogy a v1.8 hibamentes.

Dokumentációs kör alapja: `d8e0da69e8822a67cbd032facc813600559a6e7b` main.
A HU/EN README, kézikönyvek és ROM-elhelyezési útmutató ezt azonosan rögzítik.
A #147 teljes beolvasztása továbbra sem indokolt: az eredeti védelem #149-cel
elkészült; a SER7-kutatás történeti anyag. A felhasználó külön PR-rendezési
kérésére a [#147-et](https://github.com/RobCZart82/VDX7-JUCE/pull/147) 2026-10-08-án
beolvasztás nélkül lezártuk. A `test/firmware-reset-vector-admission` ág és a
`bac64d8596a498b4c3fa34c8437391018a6d2adb` head megmarad; az ág nincs törölve
vagy átírva. A lezárás indoka a PR leírásában és hozzászólásában is szerepel.
Az új koncepcióval összhangban a [#159](https://github.com/RobCZart82/VDX7-JUCE/pull/159)
viszi a kizárólag dokumentációs támogatási döntést és ezt a PR-rendezési
checkpointot; beolvasztási feltétele a végleges head zöld CI-je és a review-k
ellenőrzése. A checkpoint rögzítésekor a CI még folyamatban van.
Az alábbi korábbi checkpointokban
szereplő halasztott SER7-fejlesztést ez a döntés felülírja.

E kör ellenőrzése: PASS a támogatási szövegek egyezése hét dokumentációs
felületen, a történeti teszteredmények megőrzése, a nyolcfájlos kizárólag
dokumentációs diff és `git diff --check`, valamint az érintett Markdown-fájlok
190 helyi fájlhivatkozása. A meglévő Python-regressziók végső helyi futása
78 tesztből 77 PASS, 1 SKIP (Windows symlink-jogosultság), 0 FAIL/ERROR.
Parancs: `python -m unittest discover -s Tests -p "test_*.py" -v`.
A korlátozott környezetben a kezdeti futások ideiglenes mappa/hardlink
jogosultsági hibákkal FAIL státuszúak voltak; az írható munkamappát használó,
korlátozott környezeten kívüli újrafuttatás PASS. Ezek nem firmware-próbák.
NOT RUN: új C++/pluginbuild, privát firmware-, REAPER- és macOS-elfogadás
(nincs működésváltozás). A friss PR platform/sanitizer CI-je külön ellenőrzendő;
a helyi eredmények nem helyettesítik azt.

## Kiindulási állapot és bizonyíték

- [Publikált 1.0.1 és elfogadott teszthalasztások](../validation/VALIDATION_20261005_PUBLICATION_101.md).
- [Végleges csomagtesztek](../validation/VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md), [HU/EN kiadási jegyzet](../release/RELEASE_NOTES_1.0.1_HU_EN.md).
- A gyári bankmappa, tartalom szerinti felismerés, legacy SysEx-elfogadás és a munkahangszín projektmentése már megvalósított funkciók. Nem azonosak az alább tervezett általános bankkönyvtárral.
- A régi auditok lezárt javításait nem nyitjuk újra pusztán régi üres jelölőnégyzet miatt. Új hiba esetén reprodukció és új bizonyíték kell.

## Jóváhagyott későbbi fejlesztések

### D1 Részletes ROM hibajelzés

#### Aktuális állapot

FEJLESZTÉS ALATT, 2026-10-07. #139–#146 és #149 beolvadt: a részletes
hangszín- és motorhibák, a nulla/FF firmware és hibás cold-boot reset-cím
elutasítása, valamint a ROM-mentes processor-állapot sanitizer-lefedettsége
megvalósult. A `9ed484f` main Windows/macOS futása 15/15 CTest és 78 Python-teszt
PASS; a #151 sanitizer 11/11 PASS. Ezek nem privát ROM-os vagy REAPER-elfogadások.
#151 dokumentációs átadása is beolvadt; az új main `ba55afb` Windows/macOS
platformfuttatásai is PASS (37630504017 és 37630504016). Az Init fejlesztési ág
friss CI-kapui ettől különállóak.

Hátravan: nem homogén sérült végrehajtható firmware és valódi boot-egészség
követelményeinek specifikációja, kompatibilitási kontrollok és az érintett
privát-ROM/hostpróbák tételes elfogadása. Optimalizálás csak mérés után.
A már beolvadt PR/CI kapukat nem nyitjuk újra. SER7-támogatás nincs és nem lesz;
#147 beolvasztás nélkül lezárva, diagnosztikai ága megőrizve. A következő kiadásba kerülő
D1-kör külön döntést igényel; a publikált 1.0.1 csomagok nem változnak.

#### Korábbi részfeladatok és átadás

Az alábbi dátumozott checkpointok történeti bizonyítékok. A bennük szereplő
„hátravan”, „következő kapu” és NOT RUN az adott körre vonatkozik, nem írja
felül a fenti aktuális összefoglalót.

PR-kezelési átadás ChatGPT Work/Codex számára:
[A 147 számú Draft PR és a történeti SER7 diagnosztika](../validation/HANDOFF_20261007_PR147_DEFERRED_SER7_HU.md).
Az általános reset-cím védelem már #149-cel beolvadt; a #147 teljes
beolvasztása nem javasolt, SER7-támogatást a zöld CI nem igazol.

2026-10-07 különválasztás: új `fix/cold-boot-vector-guard` ág alapja
`4508caff3e799ee4c5729a6b1176e12e0019f17b`. Csak az általános reset-vektor
admission-védelem, a null/mérethatár és 65536 cím kontrollja, a közvetlen motor
állapotmegőrzése és a ROM-mentes processor hibaszöveg-regresszió került át.
A #147 privát hangdiagnosztikai programja és SER7 nyers MIDI kontrollja
nem része ennek az ágnak; azok külön Draft/halasztott munkák maradnak.
Nincs SER7 támogatási ígéret vagy teljes boot-egészség igazolás.
PASS: friss helyi MSVC wrapper/motor teszt, ROM-mentes kontroll és privát
original/v1.8 RAM-megőrzés, változatlan korábbi core könyvtárral linkelve.
Az új PR Windows/macOS/sanitizer kapuja szükséges; processor helyben NOT RUN;
REAPER és új teljes helyi pluginbuild NOT RUN. A firmware-ek privátak maradnak.

2026-10-07 felhasználói prioritás: az original/v1.8 stabil működése elsődleges.
Ajánlott/támogatott firmware: eredeti DX7 Mk I v1.8 (IG11469).
SER-7 jelenleg nem támogatott és nem ajánlott; a sikeres betöltés nem
funkcionális kompatibilitás. SER7 fejlesztés külön, halasztott kompatibilitási
feladat, nem a következő kiadás kapuja. A privát kontrollok és a SER7 hanghiány
diagnózisa megőrzendő; nincs általános firmware-inkompatibilitási állítás.
A #147 Draft reset-vektor munkája külön értékelendő, ez a dokumentációs
változás nem olvasztja be és nem publikál új binárist/release-t.

2026-10-07 új checkpoint: #145 beolvadt, Windows/macOS/sanitizer PASS,
review-megjegyzés nincs. Következő részfeladat alapja:
`6868007efe3f565bffd66fa1a5df53771858213a`.
A sanitizer workflow most megépíti a teljes `vdx7_processor_tests` célt,
de kizárólag annak ROM-mentes `vdx7_pre_rom_state` módját futtatja az eddigi
komponenstesztek mellett. Nincs privát ROM-fájl és nincs új DAW-teszt.
Statikus workflow-regresszió védi a build/CTest bekötést és a privát ROM-os
módok kizárását. Helyi instrumentált processor-futtatás NOT RUN;
az instrumentált CI eredménye még szükséges, a lefedettségi rést addig
nem tekintjük igazoltan lezártnak. Az általános boot-egészség továbbra is nyitott.

2026-10-07 aktuális checkpoint: #144 beolvadt; a `d8b83a52243fd0102392bc240c40b20317ddd0f0`
main platformellenőrzései és a #144 Windows/macOS/sanitizer ellenőrzése PASS.
A korábbi „friss PR/merge utáni ellenőrzés” kapu már teljesült.
Új audit reprodukálta: a teljesen nulla 16 KB-os firmware sikeresnek látszott,
miközben az emulátor illegal-opcode üzeneteket írt. Első, szűk javítás:
teljesen nulla/FF firmware elutasítása állapotváltoztatás előtt; nincs ROM-whitelist.
PASS: helyi MSVC közvetlen regresszió és privát firmware-prefix kontroll,
betöltött RAM/katalógus/kiválasztás megőrzésével; változatlan korábbi core könyvtárral
linkelve, nem új teljes pluginbuild. Az új javítás CI/review kapuja még nyitott.
NOT RUN: REAPER és új teljes helyi pluginbuild.
Hátravan: nem homogén hibás firmware felismerése és valódi boot-egészség feltételének
specifikációja kompatibilitási kontrollokkal. A `bootFailed` továbbra sem bizonyít
firmware-egészséget; e részfeladatot nem jelöljük megoldottnak.
A sanitizer közvetlen engine-tesztet futtat, a teljes processor/projektállapot
teszt instrumentált lefedettsége külön nyitott feladat.

Történeti állapot, 2026-10-06: FEJLESZTÉS ALATT. #139–#143 beolvadt;
az `08aae7cae05d4a185d972a26d51b2023df6e0ab8` main Windows/macOS
ellenőrzése 15/15 ROM-mentes CTesttel sikeres.

- Elkészült: packed-voice részletes validáció és paritás, combined/companion
  meződiagnózis, közvetlen engine kategóriák, méret/olvasási/limit hibák,
  ROM-mentes függőprojekt-regresszió.
- Aktuális javítás: a processor átveszi a motor strukturált hibáját;
  firmwareRejected/bootFailed külön üzenet, nem félrevezető mérethiba.
  A közvetlen `vdx7_rom_diagnostics` bekerül az ASan/UBSan build- és futtatási listába.
  Helyi instrumentált motor- és voice-data/SysEx teszt PASS;
  [pontos kör és korlátok](../validation/VALIDATION_20261006_ROM_DIAGNOSTIC_WIRING.md).
- Hátravan: a javítás friss PR Windows/macOS/sanitizer ellenőrzése, review és
  merge utáni main ellenőrzése; szükséges privát ROM-/hostpróbák tételes lezárása.
  Firmware/boot hibaüzenet-tesztek szintetikus kategóriákat formáznak,
  nem igazolnak valódi firmware boot-hiba reprodukciót.
- A nagyobb optimalizálás továbbra is méréshez kötött, nincs ígért gyorsulás.
  A D1 nincs teljesen késznek vagy új kiadásban szállítottnak jelölve.

#### Korábbi részfeladatok története

Az alábbi checkpointok az adott kör eredményeit őrzik; a „következő kapu”
és NOT RUN kijelentések nem írják felül a fenti aktuális státuszt.

Ötödik részfeladat, 2026-10-06: a #142 beolvadt, friss Windows/macOS/sanitizer
CI PASS, review-megjegyzés nincs. A függő projektállapot regressziója így CI-ben
is ellenőrzött. Következő tesztkör main alapja:
`feed924a2400494e143c0031f6ba742309b5ff24`.
Új ROM-mentes processor-ellenőrzések: hiányzó fájl, olvasható hibás méret és
49152 bájt feletti fájl külön diagnózist ad; korábbi hibaszöveget felülír,
személyes útvonalat nem közöl és nem tölti be a motort.
Ezen új tesztek teljes helyi futtatása NOT RUN; a `vdx7_pre_rom_state`
CI-futtatásának eredménye a beolvasztási kapu. REAPER továbbra is NOT RUN.

Negyedik részfeladat, 2026-10-06: állapotmegőrzési regressziók.
Kiindulási main: `77878b4c61a6b5655a2ebed95f4b3ac122097241`.
PASS: helyi MSVC-fordítás, ROM-mentes közvetlen engine-teszt és külön opt-in
futtatás helyi firmware-rel: hibás combined/companion adat elutasítása után
a RAM változatlan; combined esetén a betöltött állapot, bankkatalógus és
kiválasztás is változatlan. Az opt-in teszt csak az első 16 KB firmware-t
olvassa, bankadatot nem másol. A változatlan korábbi core Release könyvtárával
linkelt ellenőrzés nem új teljes pluginbuild és nem teljes ROM-os CTest-suite.
Új processor-regresszió ellenőrzi a függő projekt feedback/OP6 szerkesztéseinek
megőrzését hibás combined fájl után; helyi futtatása NOT RUN, CI-ben a
`vdx7_pre_rom_state` része. REAPER/GUI manuális ellenőrzés NOT RUN.
Következő kapu: friss Windows/macOS/sanitizer CI és review; ezután D1
elfogadási feltételeinek végső áttekintése. A privát ROM nem kerül feltöltésre.

#### Első részfeladatok checkpointjai

Harmadik részfeladat 2026-10-06: opcionális, allokációmentes strukturált
`RomLoadDiagnostic` a közvetlen engine-belépésen; invalidInput,
invalidFactoryData, firmwareRejected és bootFailed kategóriák, friss hibánál
régi részletek törlése. Pontosabb operator/LFO/pitch EG mezőnevek; változatlan
elfogadás. Új `vdx7_rom_diagnostics` ROM-mentes CTest és CI build/inventory
bekötés. PASS helyi MSVC engine-fordítás és közvetlen motor-regresszió a
0/31/32/255 hangszínhatárokra, null/partial inputra és hibás companionre;
a meglévő változatlan core Release könyvtárával linkelve, nem új teljes build.
Teljes CI és review még szükséges; betöltött/függő projekt integráció, REAPER
NOT RUN. D1 még nincs készre jelölve. #140 beolvadt, három PR-ellenőrzés PASS.

Második részfeladat 2026-10-06: a combined ROM szemantikai hiba előzetes,
állapotváltoztatás előtti felhasználói diagnózisa (bank/hangszín/bájt/mező/érték),
méret/olvasási hiba külön szöveggel, és kihagyott companion adat részletezése.
ROM-mentes processor-regresszió a 8. bank 32. hangszínére; személyes útvonal
nincs a hibaüzenetben. Helyi formatter/voice-data teszt PASS; teljes processor
és platform/sanitizer CI még ellenőrzendő, REAPER NOT RUN. D1 nem teljes:
közvetlen engine-hívás strukturált ROM-diagnózisa, pontosabb mezőnevek és
betöltött/függő állapot melletti integrációs bizonyíték még hátravan.

2026-10-06: FEJLESZTÉS ALATT, első részfeladat: allokációmentes packed-voice
diagnosztikai eredmény (kategória, hangszín/bájt, mező, érték/tartomány), szintetikus
tesztek és meglévő validátorral egybájtos teljes tartomány-paritás. Ez még nem
GUI/ROM-fájlhibajelzés, nem teljes D1 és nem hostteszt. A processor üzeneteinek,
companion/combined kategóriáinak és engine belépési diagnosztikájának bekötése,
tranzakcionális regressziója következő részfeladat; CI/review előtt nincs kész státusz.
Helyi első kör: PASS MSVC C++20 voice-data/SysEx teszt és 32 768 egybájtos
paritáskontroll; PASS diff-formaellenőrzés. NOT RUN új teljes pluginbuild,
privát ROM-os integráció és REAPER ebben a részfeladatban; a CI külön bizonyíték.

Eredeti specifikáció, jóváhagyva 2026-10-02: hibakategória, bank/hangszín, mező/bájtpozíció, érték és megengedett tartomány. Különüljön el olvasási/mérethiba, hibás kombinált ROM és kihagyott opcionális bank. Érvénytelen méret/pointer mellett nincs mezőolvasás; teljes ROM és személyes útvonal nem kerül naplóba.

Optimalizálás csak mérés után: érvényes és korai/késői hibás adat, companion/kombinált kép, ismételt validáció és zárolási idő. Közvetlen motorbetöltés sem maradhat ellenőrzés nélkül. Elfogadás: változatlan elfogadási szabályok és tranzakcionális állapotvédelem; bankhatár-, hétbites-, szemantikai-, diagnosztikai és függőprojekt-regressziók. Nincs néma javítás vagy általános 99-re vágás.

### D2 UTILITY Init Preset

#### Aktuális fejlesztés

2026-10-07: #150 adatgenerátora és #152 megerősített Init művelete main-ban van.
#152 Windows/macOS/sanitizer ellenőrzése PASS, a vegyes hangszín/kiválasztás
befogási versenyhelyzete javítva, review rendezve. Merge main: `269e230`.
A merge utáni main Windows/macOS ellenőrzése PASS (37641092380, 37641092371).
#153 is beolvadt `1675c83` main-nal: az új head Windows/macOS/sanitizer és
kód/biztonsági review sikeres, a merge utáni main Windows/macOS PASS
(37651481436, 37651481370). E kapuk teljesültek; a valódi host-elfogadás külön marad.
Az UTILITY → Init Preset megerősítést kér a munkahangszín cseréjéhez. A megerősítés
csak a befogott hangszínre érvényes: közben változó tartalom/kiválasztás/revízió
új megerősítést igényel. Mégse nem indít cserét. A gyári katalógus és a lemezen
lévő USER.vub változatlan; a RAM-ban csak az aktuális szerkeszthető másolat cserélődik.
Más munkahangszínek és PERFORMANCE/SETTINGS megmaradnak. E művelet nem automatikus
USER-mentés: a régi munkahangszín szerkesztéseit SAVE AS-szel kell megőrizni.

A kijelzett név Init Preset, dirty állapotban Init Preset*; tárolva Init Prese.
Külön példányonkénti/slotonkénti projektmező őrzi a kijelzés eredetét, nem név
alapján soroljuk át a betöltött SysEx-eket. Régi projektnél a mező alapértéke nulla.
A pontos teszteredmények a [fejlesztési validációban](../validation/VALIDATION_20261007_INIT_PRESET.md)
szerepelnek: helyi 15/15 ROM-mentes CTest ASan/UBSan mellett, 78 Python-teszt,
privát v1.8 Init processor- és párbeszédablak-teszt PASS. Friss
platform/sanitizer/review és valódi host-elfogadás
előtt D2 nincs késznek vagy kiadottnak jelölve. Következő részfeladat a külön
opt-in audio-mátrix: 44,1/48/96/192 kHz × 32/512/2048 mintás blokk,
megszólaló/sustainnel kitartott hangok alatti Init, felengedés, új hangindítás
és módosított Init projekt-visszaállítása. Ezek processor-harness tesztek,
nem valódi REAPER- vagy hangminőségi elfogadások. Kizárólag az azonosított
1.8-as firmware-rel értékelendők; az 1.6-os maskrom nem azonos tesztalap.
Az #153 review alapján a mátrix eltérő OP1 coarse/fine/output értékekből indul,
nem csupán átnevezett Init-ből. Helyi 12/12 PASS; a csak nevet/eredetjelzést
cserélő hibás kontrollt elutasítja. A #153 CI/review kapuja teljesült.

#### Eredeti specifikáció és adatmodell checkpoint

2026-10-07 első adatmodell-részfeladat, main alap:
`c4bf603099eb6f6c4097715aca0017975e61c20e` (#149 beolvadt, három CI PASS).
Saját VDX7 kezdőhangszín: algorithm 32, csak OP1 output 99, többiek 0;
ratio coarse 1/fine 0/detune 0, operator R1–R3 99/R4 80, L1–L3 99/L4 0;
pitch EG rate 99/level 50, transpose 0, feedback/modulation/scaling 0.
Tárolt 10 karakteres név `Init Prese`, tervezett GUI-felirat `Init Preset*`.
Nem Yamaha INIT dump. Pure adatgenerátor, nincs bank/RAM/SETTINGS-módosítás.
GUI, cancel/confirm és projekt-recall még nincs implementálva; nem kész D2.
Regresszió: packed validáció, algorithm/operator sorrend, név és példányizoláció.
PASS: helyi MSVC voice-data/SysEx komponensfuttatás. Új teljes pluginbuild és
REAPER NOT RUN; friss PR CI/review szükséges. E checkpoint után a lentebbi
„nincs implementáció” mondat a teljes felhasználói műveletre vonatkozik.

Eredetileg halasztott, jóváhagyva 2026-10-05; fejlesztése 2026-10-07 elindult. UTILITY → Init Preset után megerősítő ablak figyelmeztet a munkahangszín nem mentett szerkesztéseinek elvesztésére. Mégse teljes állapotmegőrzés; megerősítés csak a munkahangszínt inicializálja. Kijelző: `Init Preset*`; a csillag nem része az exportált DX7-névnek. A 10 karakteres tárolt név és a kijelzett felirat külön kezelendő.

A gyári/USER bankhely nem írható felül. ROM, bankkatalógus, MIDI/performance és GUI settings változatlan: ez nem teljes settings reset. Init szerkeszthető, exportálható, külön menthető, DAW-projekttel visszaállítható. A kapott magazinos patch csak privát referencia, nem igazolt Yamaha INIT adat; nem kerül csomagba/Gitbe.

Elfogadás: cancel/confirm, bank- és settings-azonosság, dirty jelző, export/külön mentés/projekt-recall, többpéldányos izoláció, hiányzó ROM és függő projekt védelme. A részleges implementáció nem a teljes elfogadás bizonyítéka.

### D3 Importált DX7 bankkönyvtár

Állapot: FEJLESZTÉS ALATT, jóváhagyva 2026-10-05; katalógus, projektadat és
processor-bekötés és importált bankkiválasztás is beolvadt (#154–#157).
Aktuális main alap: `ef60bd908bc0206a3bb51557a3f95a0e256e76ef`.
Windows/macOS main: 17/17 ROM-mentes CTest és 78 Python-teszt PASS
(37677145604, 37677145672); #157 head platform/sanitizer PASS.
Ez nem a teljes D3 vagy valódi host-elfogadás.
Külön Windows/macOS bedobós mappában több szabványos 32-hangszínes
bank legyen egyszerre használható. Új példány beolvassa, nyitott példányban
explicit Refresh banks; külön a Factory Banks mappától és USER.vub-tól.

#### Első katalógus részfeladat

Kiindulás: `1675c83`, #153 után mindkét main platformellenőrzés zöld.
Az első részfeladatban az önálló `VDX7ImportedBanks` modul csak tesztcélba
volt bekötve. A harmadik részfeladat már a processorhoz kapcsolja; startup
scan és új GUI-menü továbbra sincs.
Az alábbi könyvtárak a modul célútvonalai, még nem működő felhasználói funkció:
macOS `~/Library/Application Support/VDX7-JUCE/Imported Banks`, Windows
`%APPDATA%\VDX7-JUCE\Imported Banks`. A modul nem hozza létre őket magától.

Nem rekurzív, legfeljebb 512 könyvtárbejegyzést és 128 SysEx-jelöltet vizsgál.
Fájlonként legfeljebb 4104 bájt olvasható, bankonként 4096 bájt VMEM marad,
legfeljebb 128 egyedi bankkal (512 KiB nyers bankadat). A nem-SysEx elemek is
számítanak a könyvtárkorlátba. Túllépés, megszakítás vagy enumerációs hiba
üres, nem teljes eredményt ad; élő katalógust ilyenkor tilos lecserélni.
Szabályos hiányzó mappa üres első indítás; sérült/egyhangszínes/idegen fájl
látható figyelmeztetéssel kimarad. A megfigyelt szimbolikus link és nem szabályos
fájl kimarad. Ez nem fájlrendszer-biztonsági sandbox, és nem garantált I/O-időkorlát.

Azonosító a teljes validált VMEM SHA-256-ja; duplikációhoz a bájtok is egyeznek.
Fájlnév és MIDI-csatorna nem azonosító. Determinisztikus fájlnévsorrendben az első
másolat marad, későbbi másolat figyelmeztetést kap. Tartalom szerint felismert
gyári bank a Factory Banks mappába irányító figyelmeztetést kap; nem töltjük be
importált másolatként. Nincs adatnormalizálás, a korábbi legacy szabályok maradnak.
15 Unicode-kódpontnyi listanév (14 + `…`) mellett a teljes eredeti fájlnév és a
tartalmi azonosító is megmarad; az azonos rövid címke nem olvaszt össze bankokat.
A pixelalapú elrendezés, tooltip és azonos rövid nevek látható megkülönböztetése
a későbbi GUI-részfeladat része.

Helyi ASan/UBSan: 16/16 ROM-mentes CTest és 78 Python-regresszió PASS;
az új szintetikus tesztben validálás, legacy bájtmegőrzés, csatorna/névfüggetlen
azonosítás, átnevezés, Unicode, duplikátum, pontos limitek, megszakítás,
link/FIFO-kezelés és eredményizoláció szerepel. A privát ROM/bank adatai nem
kellenek ehhez a teszthez. #154 head `52fe766` Windows/macOS/sanitizer PASS
(37654489753, 37654489749, 37654489750), kód- és biztonsági review nem talált hibát.
Beolvadt `54347ce` main-nal; a merge utáni Windows/macOS ellenőrzés is PASS
(37656436530, 37656436532).

#### Importált bankok projektállapot formátuma

Második előkészítő részfeladat: `VDX7ImportedBankState`, önálló, validált
pillanatkép a processor-bekötéshez. #155 beolvadt `f0030d5` main-nal;
aktuális head Windows/macOS/sanitizer PASS (37658010320, 37658010359,
37658010318), lezárt kód- és biztonsági review, megoldatlan észrevétel nélkül.
Merge utáni Windows/macOS PASS (37660901822, 37660901878).

Az `ImportedBanks` gyermek 1-es verziója legfeljebb 128 teljes 4096 bájtos
bankot és egy opcionális kiválasztott tartalmi azonosítót tárol. Az azonosító
a mentett VMEM SHA-256-jával egyezzen; ismételt azonosító, hibás paraméteradat
vagy nem létező kiválasztás elutasítandó. A programszám és az aktuális,
akár szerkesztett munkahangszín/RAM külön, a processor állapotának része lesz:
az eredeti bank kiválasztása nem írhatja felül a projektben mentett szerkesztést.

A bankadat pontos méretű, kanonikus kódolása mellett a fájlnév UTF-8 bájtjai
is kanonikusan kódoltak, legfeljebb 1024 bájttal. Így az XML-ben tiltott,
POSIX-fájlnévben megengedett karakterek is adatvesztés nélkül megmaradnak.
A fájlnév csak címkeadat: visszaállításkor soha nem nyitunk meg belőle fájlt.
A rövid megjelenítési név nem mentett azonosító, hanem újraszámított címke.
Átnevezés, azonos rövid címke és a forrásfájl hiánya nem változtatja meg a
mentett bank tartalmát vagy kiválasztási azonosítóját.

Hiányzó gyermek a régi projektek támogatott, üres állapota; explicit üres
gyermek megkülönböztethető ettől. Hibás/hiányos mező, ismeretlen verzió,
dupla gyermek, túl sok bank, nem kanonikus hosszprefix vagy érvénytelen UTF-8
elutasítása nem módosítja a célpillanatképet és a jelenlétjelzőt.
Hibás mentési bemenet a többi projektmezőt sem változtatja meg.
A codec korlátja az adatdekódolásra vonatkozik; a külső XML/binary bemenet
méretkapuja a harmadik részfeladat része.

Helyi ASan/UBSan alatt a bővített komponens: XML/direct round trip,
legacy 127/100 értékek, 128-bankos 512 KiB tartalom, 129-bankos elutasítás,
1024/1025 UTF-8 bájtos névhatár, Unicode/vezérlőkarakteres név, hamis hash,
hibás adat és állapotmegőrzés. A hash-ellenőrzést ideiglenesen kihagyó hibás
kontrollt a teszt elutasította; a kontroll nem kerül commitba.
Ez nem valódi plugin/REAPER projekt-recall vagy kiadási elfogadás.

#### Processor katalógus és projektmentés

Harmadik részfeladat, beolvadt #156: explicit, audio callbacken kívüli frissítés
és példányonkénti változtathatatlan katalógus, a valódi processor projektmentési
és visszatöltési útvonalához kapcsolva. Nincs új hangszínkiválasztó, GUI,
induláskori beolvasás vagy automatikus mappalétrehozás ebben a körben.

A katalógus teljes bankadata és a munkahangszín/RAM együtt mentődik. A
visszatöltött importált eredet csak validált CUSTOM RAM-hoz tartozhat;
gyári bankjelölő vagy hiányzó RAM mellett elutasítandó. A könyvtár kezelése
nem tölti vissza az eredeti bank bájtjait a szerkesztett munkahangszín helyére.
Firmware nélkül a mentett projekt és a későbbi szerkesztések függőben maradnak;
a katalógusfrissítés nem írhatja felül ezt az állapotot.

Frissítéskor a kiválasztott, de a mappából eltűnt bank mentett másolata megmarad.
Ha ehhez nincs hely a 128-bankos korlátban, a teljes frissítés elutasítandó.
Hibás útvonal, limit vagy megszakítás szintén megőrzi az előző katalógust.
Generációellenőrzés akadályozza meg, hogy egy régebben indult scan felülírjon
újabb projektet, katalógust vagy bankeredet-váltást. Mentéskor a bankkönyvtár,
eredet és RAM egy zárolt pontban válik le; nagy másolás és kódolás ezen kívül
történik. Az audio-oldali eredettörlés számindexet módosít, nem nagy bankadatot.

Az állapotbemenet legfeljebb 2 MiB lehet, még az XML-feldolgozás előtt.
A maximális 128-bankos katalógus, 1024 UTF-8 bájtos fájlnevek és processor-adatok
beleférnek. Ez bájtméret-korlát, nem általános XML-mélység- vagy feldolgozási
időgarancia. Projekt-visszaállításkor a fájlnév továbbra is csak címkeadat,
nem fájlmegnyitási útvonal.

Helyi ASan/UBSan ellenőrzés: 17/17 ROM-mentes CTest, 10/10 célzott privát
CTest (a firmware-profillal együtt) és 78 Python-regresszió PASS.
A privát v1.8 firmware-rel, saját szintetikus bankokkal az importált katalógus
valódi processor-binary mentése/visszatöltése, eltűnt fájl, munkahangszín,
dirty-jelölés, későbbi szerkesztés és késleltetett ROM-betöltés is PASS.
Determinista határpontteszt ellenőrzi a mentés közbeni katalógusváltást,
függő projekt váltását és elavult scan elutasítását; ez nem teljes szálterhelési
stresszteszt. A generációellenőrzést ideiglenesen kihagyó hibás kontrollt
az új teszt elutasította; a kontroll nem marad a forrásban. A tényleges CTest
ROM-mentes, ROM-os és opt-in gyári bankos regisztrációja is megfelel a
leltárnak; utóbbi leltárellenőrzés nem futtat privát gyári bankokat.

Az állapotátmeneti privát teszt korábbi, általános ROM-hibaüzenet elvárása
elavult volt. Most pontosan a kombinált bank 8, hangszín 32, byte 12 hibás
detune értékét és határát ellenőrzi, az elutasítás és állapotmegőrzés mellett.
Az előzetes teljes reset-tesztsor ASan/UBSan alatt elérte a 180 másodperces
CTest-időkorlátot; ez a futás nem PASS, az időkaput nem növeltük meg.
Ugyanaz az előzetes reset-bináris közvetlen futtatással végigment, exit 0;
ez nem változtatja zöldre a CTest időkapuját. A végső forrás későbbi,
függő katalógusmásolást elkülönítő módosítását a fenti célzott tesztek fedik.
A harmadik részfeladat `5c948fd` head Windows/macOS/sanitizer PASS
(37665723645, 37665723720, 37665723881), lezárt kód- és biztonsági review,
megoldatlan észrevétel nélkül. Beolvadt `5e14c39` main-nal; merge utáni
Windows/macOS PASS (37669883259, 37669883349). Nincs kiadási elfogadás
vagy publikálás.

#### Importált bank kiválasztása

Negyedik részfeladat, #157-tel main-ba beolvasztva: explicit, audio callbacken kívüli
`selectImportedBank` művelet. A választás a teljes VMEM tartalmi azonosítójára
és az aktuális, változtathatatlan katalógus tokenjére épül; nem listapozícióra,
rövid névre vagy forrásfájl újbóli megnyitására. Az érvénytelen/hiányzó bank,
elavult token és 0..31-en kívüli programszám elutasítandó.

A token még a RAM-tranzakció zárolása alatt is ellenőrződik: a keresés közben
frissült vagy projekt-visszatöltéssel lecserélt katalógus választása nem
fogyaszthatja el a korábbi parancsokat és nem írhatja felül a munkahangszínt.
Függő projekt és hiányzó firmware mellett nincs importált kiválasztás.

Az elfogadott választás pontos bankbájtokat másol a szerkeszthető CUSTOM
RAM-ba és a kért programra lép; a gyári és importált forráskatalógus nem
módosul. Beállítások/performance megmaradnak. Kezdetben tiszta másolat,
a további szerkesztés dirty-jelölt munkahangszínként mentődik a projekttel,
az eredeti bank és kiválasztási azonosító mellett. A szokásos programváltás
megőrzi az importált eredetet; gyári váltás, külön SysEx-import vagy Init
új CUSTOM munkahangszíne törli az eredetet, nem a bankkönyvtárat.

A sikeres importált bankváltás generációt léptet, hogy a korábbi eredetet
rögzítő scan ne publikálhasson elavult kiválasztást. Nincs új audio-oldali
bankmásolás, menü, induláskori scan vagy mappalétrehozás ebben a körben.
A negyedik részfeladathoz külön helyi teszt, aktuális GitHub platformteszt,
hibakereső ellenőrzés és review szükséges.

Helyi végső ASan/UBSan ellenőrzés: 17/17 ROM-free és 10/10 célzott privát
firmware-es CTest PASS (a profil-előfeltétellel együtt); Python 78/78 PASS.
A tényleges CTest-regisztráció és az ellenőrző önellenőrzése is PASS.
A teszt a valódi kiválasztási API-t használja, nem kézzel létrehozott
kiválasztási eredetet. Lefedi a hibás/elavult/másolt token, ismeretlen bank,
hibás program, ROM nélküli és függő projekt elutasítását; a keresés és
RAM-tranzakció közötti katalógus-/projektcserét; a korábbi sorban álló edit
elkülönítését; pontos bankbájtokat és programot; változatlan forrásbankot,
gyári adatokat, performance/tune/channel megőrzést; szerkesztett patch
visszatöltését forrásmappa nélkül és késleltetett ROM-betöltés mellett;
programváltás eredetmegőrzését, gyári/SysEx/Init eredettörlését és scan
érvénytelenítését a könyvtár törlése nélkül.

Negatív kontroll: a második, zárolt tokenellenőrzés ideiglenes kihagyásával
a teszt elvárt módon hibára futott. Visszaállított forrással újrafordítva
a fenti végső tesztek PASS; a negatív módosítás nem része a fejlesztésnek.
A determinisztikus ütemezési teszt nem teljes körű többszálú stresszteszt.
A fent rögzített korábbi privát host-reset CTest-időtúllépés továbbra is
külön korlát; nincs teljes privát tesztcsomag-PASS vagy kiadási elfogadás.

A teljes D3 nincs kész: következő a GUI, induláskori scan/frissítés és
Windows/macOS host-elfogadás.
A privát firmware-es processor-teszt nem helyettesíti a valódi REAPER-próbát.

#### Felületi bekötés – aktuális munkakör

2026-10-08, `feat/imported-bank-ui`, alap `ef60bd9`: az LCD bankválasztó
Imported Banks csoportjának, rövid/megkülönböztethető címkéinek és teljes név
tooltipjének bekötése. UTILITY alatt külön mappanyitás és explicit Refresh;
frissítés nem cserél munkahangszínt. A felület a megjelenített változatlan
katalógustokenből választ, nem fájlt nyit újra; elavult listát nem alkalmazhat.
GUI-regresszió: két azonos hosszú Unicode-prefixű bank, nincs ROM,
katalóguscsere/üres frissítés, felületi állapot és a régi bankválasztók megőrzése.
Az induláskori automatikus scan külön következő részfeladat marad.
Helyi végső ellenőrzés: friss MSVC Release teszt- és Windows VST3-build,
17/17 ROM-mentes CTest
PASS, köztük a két Unicode-prefixű bank felületi listája, rövid/egyértelmű
címkéje, teljes tooltipje, sor-renderelése, ROM nélküli tiltás és üres frissítés.
A teszt a bekötés előtti `ef60bd9` felületen elvárt FAIL-t adott (banklista hiánya).
Privát v1.8 firmware-rel, saját szintetikus bankokkal két külön opt-in GUI-harness:
importált eredet kijelzése, elavult popup választásának elutasítása,
timer közbeni billentyűzetes választás és dirty megerősítés Mégse ága teljes
processor-state azonossággal. A standard JUCE popup nyitva tartása és a még
feldolgozatlan választás befagyasztja a megjelenített katalógust; nincs átírt
Combobox popup-életciklus. Az aszinkron ablakteszt konkrét ablaknévre vár,
2 másodperces watchdoggal, nem egy fix rövid idő után feltételezi a megjelenést.
Nem valódi REAPER vagy teljes firmware-tesztsor. A privát firmware SHA-256:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Végső ismételt opt-in eredmény: 5/5 választási és 5/5 Mégse teszt PASS;
utánuk a teljes 17/17 ROM-mentes CTest ismét PASS. Menü-sor PNG vizuális
ellenőrzése PASS; ez nem teljes popup-görgetési vagy valódi hostteszt.
Python: 78 teszt, 77 PASS / 1 jogosultság miatti SKIP; nincs FAIL.
Tesztleltár-ellenőrző önellenőrzés és diff-formaellenőrzés PASS.
Friss PR Windows/macOS/sanitizer és review még szükséges; helyi sanitizer és
valódi Windows/macOS REAPER elfogadás NOT RUN. Nincs új release vagy tag.

#### Megőrzött felhasználói koncepció

- ROM1A–ROM4B alatt Imported Banks csoport; ismeretlen érvényes bank nem kap hamis gyári besorolást. Lista fix szélességű és görgethető; keresés opcionális későbbi ötlet.
- Banknév a fájlnév `.syx` nélkül: legfeljebb 15 látható karakter (14 + `…`), szükség esetén pixelalapú rövidítés, Unicode karakter sérülése nélkül. Teljes név tooltipben; eredeti fájlnév változatlan.
- Azonosítás nem rövid név alapján: tartalmi azonosító és külön bejegyzéskezelés, azonos prefix/név és duplikátum egyértelmű megkülönböztetésével.
- Méret, fejléc, lezárás, hétbites tartalom, checksum és támogatott paraméterek validálása. Meglévő legacy szabályok maradnak. Hibás fájl látható figyelmeztetéssel kihagyandó; egyhangszínes/idegen fájl nem bank. Nincs forrásadat-átírás.
- Refresh nem módosítja a munkahangszínt. A projekt menti a szükséges saját bankadatot/azonosítót és szerkesztett hangszínt; eltűnt/cserélt/átnevezett fájl nem írhatja felül a visszaállított állapotot.
- Scan/validáció audio callbacken kívül, korlátozott memória és munka, definiált megszakítás/duplikátum/limit viselkedés, példányok izolációja.

Elfogadás: több bank, elutasított fájlok, gyári/USER adatmegőrzés, hosszú/Unicode/azonos kezdetű nevek, görgetés/tooltip, refresh közbeni edit, eltűnt/cserélt fájl melletti recall, limitek és megszakítás Windows/macOS alatt. Magazinos bankok privát tesztanyagként használhatók, eredetük nem automatikusan igazolt.

### D4 AU és Logic támogatás

Állapot: korábbi tulajdonosi jövőbeli igény, még nem kiadott támogatás. Az AU fordíthatósága nem Logic-host elfogadás. Következő lépés: pontos AU csomagolási, validációs és Logic-tesztterv, majd külön döntés a szállított formátumokról és letöltési rendről. Az 1.0.1 négy VST3-csomagját nem változtatjuk meg. Elfogadás: AU-validáció, valódi Logic működés/automatizálás/projekt-recall és megfelelő macOS installer/payload/source ellenőrzés. Ütemezése nem ígéret az összes funkció egyetlen kiadásba kerülésére.

### D5 SETTINGS Classic / Clean hangzási mód

Állapot: TERVEZETT, tulajdonos által jóváhagyott irány és dokumentálási kérés
2026-10-06. A kapcsoló helye a **SETTINGS menü**, nem a főpanel vagy UTILITY.
Ez a bejegyzés nem implementáció, mérési PASS vagy új kiadási engedély.
A következő kiadásba sorolás és a végleges műszaki megoldás külön rögzítendő.

#### Felhasználói működés és elnevezés

- SETTINGS → **Sound mode / Hangzási mód**, két egyértelmű választással:
  **Classic / Clean**. A kiválasztott érték a SETTINGS újranyitásakor látható,
  billentyűzettel és akadálymentes névvel is kezelhető legyen.
- **Classic**: a jelenlegi hardverhű kimenet és karakter, alapértelmezett.
  Meglévő hangszínek/projektek megszokott hangzását nem írjuk felül.
- **Clean**: opcionális, a hardvermodellezés bizonyos kvantálási és DAC/filter
  színezéseit megkerülő alternatíva. Nem általános „jobb DX7”, nem garantáltan
  aliasmentes FM-szintézis, és nem új sztereó motor vagy effekt.
- Rövid tooltip/HU–EN kézikönyv magyarázza a különbséget. A Famous / Modern
  alternatíva nem a tervezett felirat: a Classic / Clean pontosabb funkcióleírás.

#### DSP-vizsgálat és megőrzendő viselkedés

A rögzített dx7Lib mag `EGS::clean(bool)` / `OPS::clean(bool)` útja a prototípus
kiindulópontja; tényleges hatását és reset/betöltés utáni viselkedését ellenőrizni
kell. Nem elég pusztán új aluláteresztőt tenni a kész kimenetre. A meglévő
49 096 Hz-es belső időzítés, firmware/MIDI működés és bandlimited resampler
maradjon változatlan, amíg mérés nem indokol külön változtatást. Az SRC-teszt
eredménye nem teljes Classic/Clean hangminőségi vagy hardverhűségi bizonyíték.

- Classic azonos bemenet/kezdőállapot mellett azonos kimenetet adjon a kiadott
  alapmóddal; a Native/Correct mono-politika ettől független maradjon.
- Módváltás nem módosíthat ROM-ot, bankot, munkahangszínt, performance adatot,
  MIDI-csatornát, sustain/aktív hangok állapotát vagy a SysEx dirty jelzőt.
- GUI csak kérést ad át; a DSP-váltás biztonságos audio-tulajdonosi határon
  történjen. Nincs új callback-allokáció, fájl-I/O, firmware-warmup vagy blokkoló
  zárolás. Váltáskor nincs beragadt hang, hallható kapcsolási kattanás vagy
  indokolatlan szintugrás. Ez elfogadási cél, még nem bizonyított képesség.
- A filter/history állapotok miatt a sima bool-váltás nem tekinthető eleve
  kattanásmentesnek. Prototípussal válasszunk korlátozott átmeneti stratégiát
  (például ramp vagy megfelelően összehangolt útváltás), dokumentált hosszúsággal,
  CPU-, tranziensek- és tail-hatással. Nem indítunk két független firmware-motort
  pusztán egy crossfade kedvéért. Teljesen eltérő karakterek azonos hangerője
  nem ígéret; összehasonlításhoz külön loudness-matching kell.

#### Állapotmentés és kompatibilitás

- Példányonkénti, DAW-projektben mentett beállítás; egy példány kapcsolása nem
  változtat más példányt. Globális GUI-settings nem írhatja felül a visszahívott
  projekt hangzási módját.
- Hiányzó állapotmező esetén **Classic**, így régi projektek hangzása megmarad.
  Érvénytelen érték kezelése és állapotverziózás legyen explicit, tesztelt,
  ne aktiváljon véletlenül Clean módot és ne rontsa a meglévő állapotvédelmet.
- Hiányzó/eltérő ROM miatti függő projekt, első ROM-betöltés, újratöltés,
  prepare/release/reset után a kívánt mód megmaradjon. Projektmentés és
  visszanyitás közben nem kerülhet az audioállapottól eltérő érték a GUI-ba.
- Ez nem DX7 SysEx-paraméter; import/export és USER bankok bájtjai változatlanok.
  A meglévő 148 paraméter ID/sorrend és pluginazonosság megőrzendő. A kapcsoló
  hostautomatizálhatósága nincs még jóváhagyva; külön döntés kell, ha új paraméter
  hozzáadása indokolt. Régi paramétert nem nevezünk át és nem használunk új célra.

#### Mérési és regressziós elfogadás

1. ROM-mentes tesztek: alapérték, régi/hibás állapot, mentés/recall, példányizoláció,
   GUI választás/kijelzés és callback-korlátok, ahol szintetikus jel elegendő.
2. Privát kompatibilis ROM-mal: Classic alapvonal/null-difference, Clean működés,
   hiányzó ROM/függő projekt, reset és program/bankváltás, váltás tartott hang,
   sustain és lecsengés alatt, ismételt gyors kapcsolás és több példány.
3. Azonos MIDI/kezdőállapot: halk/hangos hangok, magas hangterjedelem, erős feedback,
   fényes/fémes hangszínek, akkordok, rövid tranziensek és halk lecsengések.
   Mérendő: spektrum, nem kívánt komponensek/zaj, peak/RMS, véges minták,
   túlvezérlési tartalék, késleltetés és CPU. A két mód közti különbség nem hiba;
   „jobb minőség” állítás csak meghatározott mért jellemzőre tehető.
4. 44.1/48/96 kHz, releváns buffer-mátrix, valós idejű/offline render Windows és
   macOS REAPER alatt; dokumentált hostarchitektúra. Hangerőben kiegyenlített
   A/B, lehetőleg vak meghallgatás; külön a műszeres eredmény és ízlésbeli preferencia.
5. Minden bizonyíték pontos SHA/mód/ROM-azonosító/host/beállítás és PASS/FAIL/NOT RUN
   státusz mellett; privát ROM, gyári/magazinos bank vagy hangfelvétel nem kerül
   automatikusan repositoryba/kiadásba. A dokumentációs kör nem futtatja ezeket.

Végrehajtás: upstream út és állapotok feltérképezése → mérési prototípus →
váltási/mentési specifikáció → minimális engine/processor implementáció és tesztek →
SETTINGS GUI → platform/sanitizer és valódi hostmérések → HU/EN útmutató és review.
Az 1.0.1 binárisok/assetek változatlanok; új funkció csak új, külön jóváhagyott
jelöltben és kiadási folyamatban szállítható.

## Teszt és karakterizálási backlog

Az alábbiak nem bizonyított hibák. Az 1.0.1-nél elfogadott halasztás nem PASS, és nem automatikus felmentés minden jövőbeli jelöltre.

- T1: teljes 44.1/48/96 kHz és 64/128/256/512/1024 buffer mátrix; valós idejű/offline 1x/teljes sebességű render azonos kezdőállapottal, hash/null-difference/első eltérés/peak/RMS/silent-block mérés (régi F7).
- T2: részletes paraméterautomatizálás, 1/4/8 példány, fizikai MIDI, transport/bypass/eszközrestart és tartós USER/CUSTOM/dirty állapot. Opcionális Write/Touch/Latch begin/value/end és középérték-karakterizálás (F2a).
- T3: öt fix GUI-méret, Windows/HiDPI, tényleges Settings/About/native Save As interakció és átfedésellenőrzés (F12); a már létező automatizált GUI-tesztek megmaradnak. Nincs GUI-újratervezés.
- T4: fizikai Intel Mac és dokumentált hostarchitektúra; hiányzó/eltérő ROM-mal régi projekt részletes helyreállítása. Korábbi általános Windows/M1 tulajdonosi PASS nem pótolja ezeket.
- T5: rövid/lassú release, nem nulla L4, sustain/pitch envelope, stop/régióvég és tail metadata (F15); sűrű MIDI/CC/edit, nagy blokkok, contention és új Note On/Off recovery (F16). SRC aliasing/frekvencia/latencia, fizikai MIDI időzítés és hosszú CPU/overload karakterizálás. Csak reprodukció alapján változtatunk motorviselkedést.

T1–T4 lefedettségi halasztásai a publikálási jegyzékben rögzítettek; T5 és F2a megőrzött korábbi karakterizálási ötletek, nem újonnan elfogadott teljesítési eredmények.

## Mérlegelendő karbantartás

- M1: `vdx7_all_tests` build-only elnevezésének tisztázása vagy külön build-and-run cél. CTest futtatás kötelező marad; nincs csendes parancsviselkedés-váltás.
- M2: egységes sajátkód-warning cél, harmadik fél kódjának és release buildnek indokolatlan `-Werror` terhelése nélkül.
- M3: Actions runtime/runner-image figyelmeztetések ellenőrzése a munka idején, pontos környezet/provenance és tesztelt változtatás. Régi figyelmeztetésből nem következik mai FAIL.
- M4: aktuális útmutatók/licencközlések/elérési utak karbantartása; történeti állítások nem kerülnek friss eredményként átírásra.

## Végrehajtás és kiadási kapuk

Javasolt sorrend: reprodukált blocker, ha lesz → D1 → D2 → D3; D4 külön host/packaging munkacsomag, D5 külön hangzási prototípus és SETTINGS munkacsomag. A következő kiadás pontos körét és D5 prioritását fejlesztés előtt rögzítjük. A rendezés nem indítja el automatikusan ezeket az implementációkat.

Minden kör: friss main → minimális változtatás és regresszió → érintett/full automatizált Windows/macOS és sanitizer ellenőrzések → review → zöld PR merge → külön main Actions ellenőrzés. Megőrzendő a pluginazonosság, 148 paraméter ID/sorrend, projektkompatibilitás, Native/Correct viselkedés, 12–120 hangterjedelem, bounded MIDI, állapotvédelem és jóváhagyott GUI. Callbackben nincs új fájl-I/O vagy nem korlátozott munka.

Új jelölt: pontos SHA/verzió, installer és manual payload, upgrade/uninstall, aláírási státusz, hash, megfelelő forrás/dependency manifest, érintett hostteszt vagy külön tételes halasztás, HU/EN dokumentáció és külön publikálási engedély. Nincs régi asset felülírás vagy Yamaha-adat terjesztés. Agent nem telepít plugin/REAPER-t és nem módosít valós DAW-projektet külön engedély nélkül.

## Források és állapotkezelés

[Korábbi összevont terv](../archive/EXECUTION_PLAN_1.0.md), [roadmap](../archive/ROADMAP_1.0.md), [régi kiadási checklist](../archive/RELEASE_CHECKLIST_1.0_RC.md) megőrzött döntés- és bizonyítéktörténet. Az élő feladatokat csak itt frissítjük, egyedi D/T/M azonosítóval. Megvalósításkor ide kerül státusz és PR/tesztlink; a részletes tesztjelentés külön marad. Régi checklist nem írhatja felül ezt a tervet vagy a kiadás tényét.

English summary: this is the sole active post-1.0.1 ledger. Approved deferred features, unexecuted coverage and optional maintenance are separate; old release gates are history, not new blockers or PASS claims. No release version/scope or publication authority is inferred from this consolidation.
