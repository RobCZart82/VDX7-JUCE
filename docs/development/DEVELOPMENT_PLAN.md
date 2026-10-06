# VDX7 egységes fejlesztési terv

Frissítve: 2026-10-06. Ez az egyetlen irányadó lista az 1.0.1 utáni munkákhoz.
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

Állapot: HALASZTOTT, jóváhagyva 2026-10-02. Következő lépés a közös validátor részletes eredményének specifikálása: hibakategória, bank/hangszín, mező/bájtpozíció, érték és megengedett tartomány. Különüljön el olvasási/mérethiba, hibás kombinált ROM és kihagyott opcionális bank. Érvénytelen méret/pointer mellett nincs mezőolvasás; teljes ROM és személyes útvonal nem kerül naplóba.

Optimalizálás csak mérés után: érvényes és korai/késői hibás adat, companion/kombinált kép, ismételt validáció és zárolási idő. Közvetlen motorbetöltés sem maradhat ellenőrzés nélkül. Elfogadás: változatlan elfogadási szabályok és tranzakcionális állapotvédelem; bankhatár-, hétbites-, szemantikai-, diagnosztikai és függőprojekt-regressziók. Nincs néma javítás vagy általános 99-re vágás.

### D2 UTILITY Init Preset

Állapot: HALASZTOTT, jóváhagyva 2026-10-05. UTILITY → Init Preset után megerősítő ablak figyelmeztet a munkahangszín nem mentett szerkesztéseinek elvesztésére. Mégse teljes állapotmegőrzés; megerősítés csak a munkahangszínt inicializálja. Kijelző: `Init Preset*`; a csillag nem része az exportált DX7-névnek. A 10 karakteres tárolt név és a kijelzett felirat külön kezelendő.

A gyári/USER bankhely nem írható felül. ROM, bankkatalógus, MIDI/performance és GUI settings változatlan: ez nem teljes settings reset. Init szerkeszthető, exportálható, külön menthető, DAW-projekttel visszaállítható. A kapott magazinos patch csak privát referencia, nem igazolt Yamaha INIT adat; nem kerül csomagba/Gitbe.

Elfogadás: cancel/confirm, bank- és settings-azonosság, dirty jelző, export/külön mentés/projekt-recall, többpéldányos izoláció, hiányzó ROM és függő projekt védelme. Ellenőrzött paraméter-specifikáció szükséges; jelenleg nincs implementáció vagy új PASS.

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
