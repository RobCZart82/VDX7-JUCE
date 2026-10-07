# VDX7 egységes fejlesztési terv

Frissítve: 2026-10-07. Ez az egyetlen irányadó lista az 1.0.1 utáni munkákhoz.
Az 1.0.1 már megjelent; a régi kiadási kapuk nem új nyitott feladatok.
A következő kiadás verzióját és pontos funkciókörét később rögzítjük.
Ez a dokumentum nem új kiadás publikálási engedélye.

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
A már beolvadt PR/CI kapukat nem nyitjuk újra. SER7 külön halasztott munka;
#147 Draft marad, teljes beolvasztása nem indokolt. A következő kiadásba kerülő
D1-kör külön döntést igényel; a publikált 1.0.1 csomagok nem változnak.

#### Korábbi részfeladatok és átadás

Az alábbi dátumozott checkpointok történeti bizonyítékok. A bennük szereplő
„hátravan”, „következő kapu” és NOT RUN az adott körre vonatkozik, nem írja
felül a fenti aktuális összefoglalót.

PR-kezelési átadás ChatGPT Work/Codex számára:
[A 147 számú Draft PR és a halasztott SER7 diagnosztika](../validation/HANDOFF_20261007_PR147_DEFERRED_SER7_HU.md).
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
Az új audio-tesztág ellenőrzése külön kapu.
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

Állapot: HALASZTOTT, jóváhagyva 2026-10-05. Külön Windows/macOS bedobós mappában több szabványos 32-hangszínes bank legyen egyszerre használható. Új példány beolvassa, nyitott példányban explicit Refresh banks; külön a Factory Banks mappától és USER.vub-tól. Pontos útvonal és fájl/bájt/bankszámkorlát a specifikációban rögzítendő.

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
