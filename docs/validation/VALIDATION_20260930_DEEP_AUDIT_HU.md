# VDX7 repository mély hibakeresési jelentés

Dátum: 2026. szeptember 30., Europe/Budapest.

Vizsgált forrás: `3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0`, a GitHub `main` frissen letöltött állapota, PR #107 után. A vizsgált Git-fa azonosítója `870f8101cbfe46446f6fcf8850d4014acc101149`.

A vizsgálat négy ténylegesen reprodukált működési hibát talált. Két hiba ROM-betöltéssel átfedő állapotkezelés során paramétermódosítást veszít el. Két további hiba a CC120 és CC121 új MIDI-kezelésének peremhelyzeteiben jelentkezik. Mindegyikhez sikeres kontrollfuttatás és hibát kimutató külön reprodukció tartozik. Az itt futtatható meglévő CTest-készlet ugyanakkor 39/39 PASS eredménnyel végzett: a jelenlegi tesztek ezeket a kiváltó helyzeteket nem fedik le.

A jelentés a jelenlegi fejlesztési forrásra vonatkozik. A publikált 1.0.0 telepítőket és telepített pluginokat ebben a körben nem futtattuk újra. A termékkódot, az elfogadott GUI-t és a GitHub-ot nem módosítottuk; a vizsgálathoz külön tesztprogramok és ez a helyi jelentés készültek.

## Vizsgálati területek és környezet

Átnézett területek: processor és engine, projektmentés és visszaállítás, hiányzó és eltérő ROM, USER/CUSTOM bankok és SysEx, paraméterek és publikációjuk, MIDI-admission és reset, késleltetett és virtuális billentyűzetes eseménysorok, MONO-korrekció, resampling, editor és PERFORMANCE/Settings/About, CMake és tesztregisztráció, Python-csomagolás és checksum, Actions workflow-k és telepítőforrások.

A helyi futtatás macOS 26.7, Apple Silicon arm64 környezetben történt. A megfigyelt eszközverziók Apple Clang 21.0.0, CMake 4.4.3, Python 3.9.6 és Git 2.54.0. A firmware-t igénylő tesztekhez a tulajdonos helyi, kombinált 48 KB-os v1.8 ROM-ja szolgált; ROM-adat nem került jelentésbe vagy repository-fájlba.

A meglévő `build-1.0` könyvtár a PR #107 fejének változatlan forrásából épült. `git diff HEAD origin/main` üres, és mindkét fa azonosítója a fent megadott hash. Az audit újraellenőrizte a buildet és futtatta a teszteket. Ez inkrementális buildellenőrzés, nem új, tiszta C++ build. A csomagolási vizsgálat külön, friss offline CMake-konfigurációt is végzett.

## Megállapítások

| Jelölés | Reprodukált hiba | Kiváltó helyzet | Következmény |
| --- | --- | --- | --- |
| S1 | Hiányos mentési pillanatkép | Az első ROM-betöltés befejeződik a no-ROM állapotmentés két része között | Újranyitáskor elveszik a már korábban elfogadott paraméterérték |
| S2 | Betöltés közben elfogadott edit törlése | Hiányzó ROM-ra váró projekt ROM-bemelegítése közben host-paraméter változik | A sikeresen elfogadott új értéket felülírja a korábbi projektérték |
| M1 | CC120 utáni események hibás idővonala | CC120 után ugyanabban a blokkban Note On, majd a következő blokkban új esemény érkezik | A sor téves sorrendhibát észlel, eldobja a hangjegyet és a következő eseményt |
| M2 | CC121 részleges controller-reset | A firmware felé menő 1024 elemű alkalmazási FIFO majdnem vagy teljesen tele van | Sustain, portamento vagy analóg vezérlő aktív maradhat, miközben a wrapper resetet jelez |

Javasolt besorolás mind a négy esethez: P2, reprodukált működési hiba. Az S1/S2 ritka időzítési átfedést igényel, de tényleges paramétervesztést okoz. Az M1 kis számú szabályos eseménnyel előidézhető. Az M2 sűrű controller-forgalmat igényel. Ezeket az 1.0.1 elfogadása előtt érdemes célzottan javítani és regressziós teszttel lezárni.

## S1 Mentés és első ROM betöltés átfedése

Forráshely: [PluginProcessor.cpp 1097](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/PluginProcessor.cpp#L1097), különösen a `loaded` rögzítése a 1132. sorban, a lock elengedése az 1152. sorban és a dirty maszkok későbbi olvasása a [1206. sortól](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/PluginProcessor.cpp#L1206).

Reprodukció:

1. Új processor, automatikus ROM-keresés nélkül.
2. A host a voice feedback paramétert 6-ra állítja.
3. Állapotmentés kezdődik. A meglévő, csak tesztbuildben aktív hook megállítja a mentést, miután `loaded=false` és a ROM/RAM-adatok rögzültek, de a késleltetett edit-adatok még nem.
4. Az első ROM-betöltés sikeresen befejeződik; a futó voice feedbackje 6.
5. A mentés folytatódik. A betöltés már elfogyasztotta a dirty maszkokat, így a mentésbe sem RAM, sem `DeferredVoiceEdits` nem kerül.
6. A mentés új processorban történő visszaállítása és ugyanazon ROM betöltése után feedback=7 áll elő a várt 6 helyett.

Megfigyelt kimenet:

```text
control=save-before-load liveFeedback=6 reopenedFeedback=6
control=load-before-save liveFeedback=6 reopenedFeedback=6
loaded=1 liveFeedback=6 savedHasRam=0 savedDeferred=0
expectedFeedback=6 reopenedFeedback=7
```

A két soros kontroll PASS. Az átfedő változat három ismétlésben hibázott; a fő audit külön is reprodukálta. A program a paramétermegőrzési elvárás sérülésekor 1-es exitkóddal tér vissza. A hook kizárólag a szálak ütemezését szabályozza, a product-adatokat nem változtatja meg. Nem igényel egyidejű teljes `setStateInformation`/`getStateInformation` műveletet: host-mentés és UI ROM-betöltés átfedése elegendő.

Javítási irány: a no-ROM állapot explicit editjeinek maszkjai és értékei ugyanazon leválasztott mentési pillanatkép részei legyenek, mint a `loaded`/RAM/ROM-identitás. Az XML-kódolás továbbra is a mutexen kívül történjen. Regresszió: első ROM telepítésének átfedése mentéssel; operator és voice mezők, mentés előtti és utáni kontrollok.

## S2 Automatizálás hiányzó ROM pótlása közben

Forráshely: [parameterChanged 975](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/PluginProcessor.cpp#L975) és [loadRomData 1444](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/PluginProcessor.cpp#L1444). A loader a bemelegítés előtt egyszer rögzíti a pending editjeit; a 1463–1465. sorok utána feltétel nélkül nullázzák a maszkokat. A `restoreSavedStateLocked` az 1403–1405. sorokban szintén nulláz.

Reprodukció:

1. Érvényes, ROM-identitást és packed RAM-ot tartalmazó projektmentés készül.
2. A memóriaállapot ROM-elérési útját egy nem létező helyre állítjuk, és új processorba visszaállítjuk. A projekt most a megfelelő ROM-ra vár.
3. Elindul az eredeti ROM betöltése.
4. A firmware bemelegítése közben a host egyszer feedback=6 értéket küld. A projekt ekkor még pending; az APVTS azonnal 6-ot mutat.
5. A ROM-betöltés befejezésekor az engine és a publikált host-paraméter visszaáll 7-re.

Megfigyelt kimenet:

```text
pendingControl=edit-before-load finalFeedback=6
pendingControl=edit-after-load finalFeedback=6
hasSavedRam=1 loaded=1 pendingAtEdit=1 acceptedFeedback=6 expectedFeedback=6 finalFeedback=7
```

A valódi mentett RAM-os eset három ismétlésben hibázott; a fő audit külön is reprodukálta. No-RAM pending állapottal ugyanez háromszor előállt. A betöltés előtti és utáni edit-kontrollok megtartják a 6-ot. Ez a reprodukció a változatlan normál API-t használja, a loaderbe nem került teszthook. A warm-up alatti időablakot helyi szálütemezéssel keresi meg, és a `pendingAtEdit` visszaellenőrzi, hogy a releváns átfedés megtörtént-e.

Javítási irány: a loader csak a már rögzített edit-generációt fogyassza el; a ROM-inicializálás alatt érkező későbbi host-edit maradjon alkalmazható a visszaállítás után. A változtatás tartsa meg a lock nélküli paramétercallbacket és az eredeti projekt-RAM elsőbbségét a betöltés előtt rögzített edit nélküli mezőknél.

A feltétel nélküli maszknullázás hasonló kódja a publikált 1.0.0 forrásában is megtalálható. Az audit azonban ennek az esetnek a futtatását a jelenlegi mainből épített processoron végezte; a kiadott 1.0.0 bináris érintettsége nincs ebben a körben igazolva.

## M1 CC120 után a következő blokk téves sorrendhibát okoz

Forráshely: [PluginProcessor.cpp 550](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/PluginProcessor.cpp#L550) és [VDX7DeferredMidi.h 20](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/VDX7DeferredMidi.h#L20).

48 kHz, 64 mintás blokkokkal, működő processoron:

1. Az első blokkban CC120 érkezik a 8. mintán, majd Note On 62 a 24. mintán.
2. A CC120 resetet indít, a későbbi Note On a deferred sorba kerül.
3. A következő blokkban CC11=64 érkezik a 0. mintán.
4. Az első, közvetlen feldolgozást késleltetett feldolgozásra váltó ág nem léptette a deferred `inputTime_` számlálóját. Ezért az új blokk eseménye 0 időbélyeget kap a korábbi 24 után.
5. A sor sorrendhiba miatt `panic` állapotba lép, és mindkét esemény elveszik.

Megfigyelt kimenet:

```text
after CC120 reset=1 deferred=1
first queued=144/62/100 pos=24 bytes=3
second panic
final reset=0 blocks=0 deferred=0 active62=0 keyboard62=0 expression=1
```

Üres következő blokkal a kontroll megtartja a Note Ont: `active62=1`, `keyboard62=1`. A hibás változatnál CC11=64 sem hatott; a várt 64/127 helyett az expression 1 maradt. A deferred sor másolatának kiolvasása a tesztben csak megfigyelés, az eredeti feldolgozást nem módosítja. Mindössze három releváns MIDI-esemény elegendő, így ez nem kapacitástúllépési korlát.

Javítási irány: a közvetlen útból a deferred útba váltáskor ugyanúgy rögzíteni kell a blokkok monoton idővonalát és a CC120 megállítási pozícióját. Regresszióként a reset utáni, azonos és következő blokkbeli Note On/Off és CC-eseményeket, eltérő blokkpartíciókkal is ellenőrizni kell.

## M2 CC121 esetén telített FIFO mellett részleges a reset

Forráshely: [VDX7Engine.cpp 164](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/VDX7Engine.cpp#L164) és [613](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/Source/VDX7Engine.cpp#L613).

A CC121 kikerüli a szokásos kapacitásfoglalást. A reset hat sustain/portamento/analóg vezérlőüzenetet közvetlenül küld a firmware alkalmazási FIFO-jába. A core ezen az útvonalon nem ad vissza ellenőrzött sikert; a megtelt FIFO-ba érkező üzenetek elvesznek. A pitch/mod wheel ezzel szemben tartós pending intentet kap, ezért azok később helyesen resetelődnek.

Reprodukció: aktív sustain, portamento és aftertouch után egy sustainnel tartott hang és 1024 elfogadott CC2=127 üzenet kerül a rendszerbe renderelés nélkül. Ez megtölti az alkalmazási FIFO-t. Ekkor CC121 érkezik. Hosszú renderelés után a FIFO már üres, de a valódi firmware-állapot így marad:

```text
pre sustain wrapper=1 pedal=1
fifo full=1 overload=0
post CC121 wrapper=0 fifo full=1 overload=0
final fifo empty=1 pedal=1 porta=1 breath=254 aftertouch=254 pitch=128 mod=0 sustained=1
```

A wrapper szerint sustain=0, a firmware-ben viszont a pedal bit 1 és egy sustainnel tartott voice tovább él. Pitch és mod resetje helyes; a többi controller részben elveszett. A MIDI-overload számláló 0 maradt. A bemenet szabályos és az eseményeket az engine elfogadta.

Kontrollok: 1023 feltöltő üzenet mellett sustain már resetelődik, de portamento és több analóg érték nem; 1020 mellett aftertouch még aktív marad; 1018 vagy 0 mellett minden vizsgált reset helyes. Normál CC64=0 üres FIFO mellett kioldja a sustainnel tartott hangot, a portamento/aftertouch értékeket helyesen nem reseteli. Ez az elkülönítő kontroll igazolja, hogy a vizsgálat a CC121 teljes controller-resetjét méri.

Javítási irány: az összes CC121-resetérték kapjon garantált, sorrendet őrző pending kezelést, vagy a teljes resetcsomag csak ellenőrzött kapacitás mellett legyen fogyasztottnak tekinthető. A regresszió a FIFO kiürülése után a tényleges firmware-értékeket és a sustainnel tartott voice kioldását is ellenőrizze, ne csak a wrapper flagjeit.

## Meglévő tesztek és csomagolás eredménye

Futtatott parancsok, a helyi build- és dependency-útvonalak helyőrzőkkel:

```text
cmake --build <build> --target vdx7_ci_checks -j 4
ctest --test-dir <build> --output-on-failure --no-tests=error -E '^vdx7_processor$' -j 2
python3 -m unittest discover -s Tests -p 'test_*.py' -v
```

Eredmények:

- Inkrementális buildellenőrzés: PASS, nincs újrafordítandó fájl.
- CTest: 39/39 PASS, 237.37 másodperc. Ebben a helyi ROM-os tesztek és a headless GUI-header teszt is lefutottak.
- A 40 regisztrált tesztből a meglévő, valódi desktopot/mentési dialógust igénylő `vdx7_processor` kimaradt. Emiatt a teljes 40 tesztes készletre nem állítunk PASS-t.
- Python packaging/checksum: 12/12 PASS, nincs skip.
- Friss offline CMake konfiguráció: PASS.
- ROM-free tesztregisztráció és hét negatív kontroll: PASS.
- Az aktuális commitból készített corresponding-source ZIP: 5089 fájl, manifestellenőrzés PASS.
- Kétszeri forráscsomag-generálás ugyanazon gépen azonos SHA-256-ot adott: `06c8d160b0cb4945aa24dc342080639b7973b0d1d29f7f64af5585fc396cd91e`.
- A wrapper 248 commitolt fájljának payload-ellenőrzése és a kis/nagybetűs útvonalütközések ellenőrzése PASS.

A forráscsomag eredménye ugyanazon gépen reprodukálható ZIP-et igazol; nem állít bitazonos telepítőbuildet, új platform-elfogadást vagy kiadási készültséget.

## Elmentett reprodukciós bizonyítékok

A külön auditprogramok a változatlan termékkódhoz kapcsolódtak; nem kerültek be a termék buildjébe. A helyi, nem terjesztett `deep-audit-evidence-20260930` bizonyítékmappában megmaradt az S1/S2 `no_rom_save_race.cpp`, az M1 `cc120_timeline_repro.cpp`, az M2 `cc121_fifo_repro.cpp` és a tíz hibás/kontroll eset kimenetét rögzítő `RESULTS_HU.txt`. A lényegi kimenetek fent szerepelnek, így a jelentés nem függ a helyi mappa elérhetőségétől.

A későbbi javítási kör külön, repositoryban megőrzött regressziói: [állapotátmenetek](../../Tests/VDX7StateTransitionTests.cpp), [CC120 idővonal](../../Tests/VDX7CC120TimelineTests.cpp), [CC121 controller-reset](../../Tests/VDX7ControllerResetTests.cpp). A [javítási validáció](VALIDATION_20260930_DEEP_AUDIT_FIXES.md) külön rögzíti az elfogadási eredményeket; nem módosítja a jelen audit történeti, hibás baseline-eredményeit.

Mindhárom program első argumentuma a felhasználó saját kompatibilis ROM-fájljának útvonala. A ROM nincs a bizonyítékmappában. A két processoros program a jelenlegi tesztbuild fordítási beállításait és könyvtárait használta; az S1 program a meglévő `VDX7_TEST_STATE_BOUNDARY`-vel fordított processor objektumhoz kapcsolódott. M2 a jelenlegi engine/core forrásból épült. A hibás elvárás exit=1, a sikeres kontroll exit=0 eredményt ad; a szálütemezési reprodukciók időkorlátosak, és az átfedés elmaradását külön, nem hibabizonyítékként jelzik.

## Mi nem lett új hibaként igazolva

A GUI geometria, vezérlőbindingok és panel-élettartam átnézéséből ebben a körben nem született új reprodukált GUI-hiba. A forráscsomagoló, checksum-kezelés, CMake-regisztráció és kiadási folyamat vizsgálatából sem született új reprodukált csomagolási hiba.

A korábban nyilvántartott A4–A7 feladatok továbbra is a [közös fejlesztési tervben](https://github.com/RobCZart82/VDX7-JUCE/blob/3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0/docs/release/EXECUTION_PLAN_1.0.md) maradnak: elfogadott product/packager/workflow összerendelése, Windows uninstall-hely és migráció vizsgálata, toolchain-verziók, valamint a kiadási fájlok és bizonyítékok egyeztetése. Ezek nem az új négy runtime találat duplikátumai.

A korábban elfogadott nulla tail metadata, a try-lock alatt elnémuló blokk, a bounded queue-k kapacitása és a még le nem futtatott hostmátrix ebben a körben nem lett önmagában hibává minősítve. A teljes upstream JUCE/core, minden hardver és DAW kombináció, illetve hosszú valódi hostteszt nem volt része a futtatásnak.

Ebben a körben nem készült új ASan/TSan build, nem futott új Windows-bináris teszt, és nem indult valódi REAPER-hostteszt. A sikeres meglévő tesztek ezért nem jelentenek általános hibamentességi bizonyítékot; a négy találat a megadott helyi reprodukciókkal igazolt.

## Javasolt következő javítási kör

Először az S1/S2 állapot- és ROM-tranzakciós hibákat érdemes regresszióval javítani, majd az M1 idővonal-átmenetet és az M2 tartós controller-resetet. Az új tesztek a jelenlegi forráson bukjanak, a javított forráson pedig menjenek át. A termékkód megváltoztatása után a releváns helyi ROM-os regressziók és a Windows/macOS Actions következzenek. Az elfogadott GUI és a plugin paraméterazonosítói a javításokhoz nem igényelnek változtatást.

Az audit nem implementálta ezeket a javításokat és nem hozott létre PR-t. A javítások majd a meglévő egyetlen execution plan megfelelő tételeibe illeszthetők; ez a fájl a reprodukciók bizonyítéka.
