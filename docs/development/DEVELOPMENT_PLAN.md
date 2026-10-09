# VDX7 egységes fejlesztési terv

Frissítve: 2026-10-09. Ez az egyetlen irányadó lista az 1.0.1 utáni munkákhoz.
Az 1.0.1 már megjelent; a régi kiadási kapuk nem új nyitott feladatok.
A következő kiadás része a D5 Classic/Clean funkció a 2026-10-09-i tulajdonosi
döntés szerint. A verziószám és a további funkciók pontos köre még nincs rögzítve.
Ez a dokumentum nem új kiadás publikálási engedélye.

## Következő release: Classic/Clean kötelező funkció — 2026-10-09

Közvetlen tulajdonosi döntés: **„Classic/Clean funkcióval együtt kiadni.”**
Ellenőrzött main: `000ee97b19763b23c569ff6ac68080c4b6165094` (#169 után).
A D5 nem opcionálisan későbbre hagyott ötlet: a következő release csak a
teljes, tesztelt motor/processor/projektmentés és SETTINGS integrációval készülhet.
Az önálló modell, codec vagy owner elkészülte nem kész funkció, és a zöld CI
nem helyettesíti a hangzási/host-elfogadást. A production Clean renderer/mód-
ownership/SETTINGS integráció még nyitott; az alábbi admission-részlépés nem
jelenti a teljes funkció megvalósítását.

Megőrzendő koncepció: **SETTINGS → Sound mode → Classic / Clean**;
Classic alapértelmezett, legacy projektben is, a korábbi hangzás megőrzésével.
Clean opcionálisan választható, példányonként projektbe mentett mód. Native/Correct
mono-policy független marad; 148 hostparaméter ID/sorrend és pluginazonosság
változatlan. A döntés nem engedély új automatizálható hostparamétert vagy új
firmware-támogatást. Kizárólag original DX7 Mk I v1.8 a támogatott tesztalap.

### D5 valódi processor-admission részlépés — 2026-10-09

Baseline: `2e42412c7da50eea10a17f222983e959a2e76310` (#170 után), külön ág:
`feature/classic-clean-processor-admission`,
[PR #171](https://github.com/RobCZart82/VDX7-JUCE/pull/171).
A valódi `setStateInformation()`
már a mono-policy, imported/pending state, MIDI epoch és APVTS módosítása
**előtt** használja a D5 readerét. Hiányos/hibás/újabb sémapár elutasítandó.
Legacy és explicit Classic megmarad. **Érvényes Clean is teljes elutasítás**,
amíg renderer/lifecycle nincs: nem telepítjük a payloadot Classic-ként, és
nem tárolunk működő Clean kívánt módot DSP nélkül. Ez átmeneti fejlesztési kapu,
nem a végleges Clean restore implementáció és nem downgrade-ígéret.
Az új writer nem aktív; a normál mentés nem kezd új módmezőket létrehozni.
Megőrzött pending explicit Classic mezőpár változatlanul továbbmenthető.

Teszt először: legacy/Classic kontroll PASS, első hiányos párnál a generáció-
megőrzés FAIL a változatlan production kódon. Javítás után 21 ROM-free CTest
PASS, 8 célzott privát v1.8 teszt PASS, Windows VST3/Standalone és ci_checks
build PASS; Python 94 futott, 93 PASS / 1 symlink SKIP. A 30 korábbi Classic
kontrollhoz mért 5 760 512 float minta, projektállapot és metrika azonos: PASS.
#171 beolvadt; a végleges `159326073d31603af196fa6be39693301f04658b` fej
Windows/macOS/ASan–UBSan CI-je PASS. A `31d39032c4d8aa53a88a0cee1003e47b0bdbeec2`
merge utáni main Windows/macOS ellenőrzése is PASS. A review és a pontos futások
a részletes jelentés utólagos lezárásában szerepelnek. Ez nem teljes D5 vagy új
release-elfogadás; a korábbi helyi tesztkör korlátai változatlanok.

Következő nyitott lépés: kívánt mód és payload koherens valódi save/restore/
pending/ROM-epoch ownership, majd native/SRC renderer és átmenet közös
integrációja. Clean admission csak e működő hangút bizonyítéka után nyitható;
a jelen fail-closed negatív kontroll akkor pozitív Clean recall/lifecycle
regresszióval cserélendő, a hibás séma rejection-tesztek megmaradnak.
[Részletes reprodukció, ellenőrzések és korlátok](../validation/CLASSIC_CLEAN_PROCESSOR_ADMISSION_20261009.md).

### D5 előfeltétel a leválasztott projektmentéshez

Baseline: `31d39032c4d8aa53a88a0cee1003e47b0bdbeec2`; külön javítási ág:
`fix/coherent-project-snapshot`, [PR #172](https://github.com/RobCZart82/VDX7-JUCE/pull/172).
A mentés rögzítése utáni módosítás vagy másik
projekt visszatöltése eddig új hangerő/kerék értékeket, firmware nélkül pedig
új hangszín-paramétereket keverhetett a korábban rögzített payloadba.
A reprodukáló actual-processor teszt a változatlan kódon FAIL, a javítás után
PASS. Most mind a 148 paraméter értéke a capture-ből származik; a loaded
hangszín továbbra is a motorból, a no-ROM hangszín az APVTS atomikus értékeiből.
A pending megőrzési út változatlan, külön kontrollal. Nincs új módmező vagy
Clean-elfogadás, és nincs új lock vagy hosthívás az audio callbackben.

Ez a capture **utáni** keveredést javítja, nem teszi az önálló automatizációs
írásokat vagy egy capture közben zajló teljes restore-t atomikus tranzakcióvá.
A valódi desired-mode/payload/pending/ROM-epoch ownership továbbra is nyitott.
A #172 végső fejének Windows/macOS/sanitizer CI-je PASS, és a PR
`6d66610109a867e9c1494f95081c594a2a8506ea` merge-commitként main-ba került.
A külön main Windows/macOS kör is PASS; ez a részlépés lezárt,
nem a teljes D5 funkció elfogadása.
[Reprodukció és célzott ellenőrzések](../validation/PROJECT_SNAPSHOT_CAPTURE_20261009.md).

### Reprodukált projektparaméter blocker javítása

Baseline: `6d66610109a867e9c1494f95081c594a2a8506ea` (#172 után);
ág: `fix/validate-project-parameters`, [PR #173](https://github.com/RobCZart82/VDX7-JUCE/pull/173).
A hibás PARAMETERS értékek eddig
NaN-ként bekerülhettek az APVTS-be, és a projekt más beállításai is módosultak.
Most a teljes számformátum, double/float végesség és ismert ID-k egyértelműsége
ellenőrzött minden restore-módosítás előtt. Régi részleges/RAM-only állapot,
defaultok és véges tartománykorlátozás megmarad; a legacy EG/fine nyers bankadat
változatlan. Az audio callback, hostparaméterek és Clean admission nem változik.

A guard nélküli actual-processor negatív kontroll FAIL; a javítás után
21 ROM-free CTest, 94 Python teszt, célzott no-ROM/private v1.8 ASan–UBSan,
legacy bank round trip és macOS dev-build PASS. A végső PR-fej
`5b76ac1` Windows/macOS/ASan–UBSan ellenőrzése PASS; #173 beolvadt
`53b9589d669dd905cce4f1762421128ca397291a` main-commitként. A külön main
[Windows](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37955955166) és
[macOS](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37955955317) kör PASS.
E javítás után a korábban rögzített D5 valódi állapot/renderer integráció
következik; nincs új release vagy publikálási engedély.
[Részletes ellenőrzések](../validation/PROJECT_PARAMETER_ADMISSION_20261009.md).

### D5 előfeltétel: leválasztott ROM-completion projektgenerációja

Baseline: `53b9589d669dd905cce4f1762421128ca397291a`; külön ág:
`feature/processor-project-ownership`, [PR #174](https://github.com/RobCZart82/VDX7-JUCE/pull/174).
Reprodukált actual-processor hiba:
egy régi projekt leválasztott ROM-beolvasása az időközben teljesen visszatöltött
új projekt hangszínét és firmware-útvonalát felülírhatta. Privát original v1.8
baseline kontroll FAIL; generációvédett admission után PASS a kézi ROM-load és
teljes restore reentráns/valódi kétszálas mátrixa.

A meglévő engine lock alatt telepített projektgenerációt a saved-ROM restore,
kézi load és autodetect végig megőrzi. Stale kérés még boot/RAM/epoch mutáció
előtt elutasított; a régi restore completion sem dolgozza fel az új pending
projektet. Elutasított state és mentés nem lépteti a számlálót; nincs wrap.
21 ROM-free CTest, 94 Python, macOS VST3/AU/Standalone és célzott privát/ROM-free
ASan–UBSan PASS. A teljes factory-bank/combined-ROM privát kör nem kapott
PASS-t a 16 KB-only fixture-rel; a jelentés tételesen megőrzi a korlátot.

Ez **csak előfeltétel**, nem a teljes D5 owner beillesztése. A teljes APVTS/
payload/mód tranzakció, admission utáni publikáció és ugyanazon generáción belüli
kézi load-ok sorrendje nincs e javítással lezárva. Clean még fail-closed;
renderer/SETTINGS nincs bekapcsolva. Final-head platform/sanitizer CI, review és
merge utáni main külön kapu. Ezután a valódi desired-mode/payload ownership
integrációja következik az alábbi sorrendben; nem új release-publikálás.
[Reprodukció, regressziók és korlátok](../validation/PROJECT_ROM_GENERATION_20261009.md).

### Végrehajtási sorrend és kiadási megállási kapuk

1. **D5 processor-állapot integráció:** az elkészült codec/owner a valódi
   save/restore/pending/ROM-admission és engine-epoch tranzakciókba illesztendő.
   Hibás state és stale request nem mutálhat projektet; reentráns mentés,
   példányizoláció, hiányzó/eltérő ROM és mono-policy regressziók szükségesek.
2. **D5 motor és átmenet:** a valódi Classic/Clean út, instruction overshoot,
   SRC/history és prepare/release/reset/reload együtt kezelendő. Bounded,
   kattanásmentes átmenet; nincs új callback-allokáció, fájl-I/O, blokkoló lock
   vagy indokolatlan hangerőugrás. DSP nélküli működő Clean-jelzés nem szállítható.
3. **Privát original v1.8 műszeres elfogadás:** Classic referencia/null-difference,
   működő és véges Clean, módváltás tartott hang/sustain/tail közben, gyors
   ismételt váltás, lifecycle és külön példányok. Spektrum, peak/RMS, headroom,
   CPU/latencia; az eltéréseket értékeljük, nem nevezzük automatikusan javulásnak.
4. **SETTINGS és platformkapuk:** kapcsoló csak a működő DSP/állapot után;
   billentyűzet, hozzáférhető név, helyes pending/BUSY/STALE kijelzés, régi
   projektek és öt GUI-méret. Friss Windows/macOS és sanitizer, rendezett review.
5. **Verzió és kiadási csomagok:** a választott új verzióra a CMake/installer,
   source packager címke/validáció, tesztek és approval összhangban frissítendők
   (a jelen tooling 1.0.0/1.0.1 címkékre korlátozott). Négy felhasználói letöltés:
   Windows EXE + Manual ZIP, macOS PKG + Manual ZIP; teljes megfelelő forrás,
   hash/provenance, offline build, telepítés/upgrade/uninstall az exact jelölthöz.
   A csomagba kerülő HU/EN README/kézikönyv és Classic/Clean/default/recall/
   downgrade/aláírási tájékoztató ekkor már legyen végleges. **Host-elfogadás
   előtt** rögzítjük a product/packager SHA-t, a tényleges kijelzett verziót és
   az elkészült telepítő/manual/source fájlok SHA256-ját: ez a fagyasztott jelölt.
6. **Új jelölt valódi host/hallásos elfogadása:** az 5. lépésben rögzített,
   változatlan telepítő/manual csomagokból telepített binárisokkal Windows/macOS
   REAPER; sample-rate/buffer, offline render, mentés/újranyitás, több példány,
   váltás kitartott hang alatt, hangerőben egyeztetett Classic/Clean meghallgatás.
   Jegyzőkönyvben csomaghash, pluginverzió és host/OS/architektúra legyen.
   Korábbi dev-build mérése előzetes bizonyíték, nem e csomag végső elfogadása.
   Helyi DAW nélkül e kapuk NOT RUN; külön tételes halasztás csak új, explicit
   tulajdonosi döntéssel, nem az 1.0.1 régi halasztásaiból. Nem halasztjuk
   automatikusan magát a D5 funkciót vagy annak helyességi bizonyítékát.
   Ha teszt után kód, compiled verzió, dependency, build/installer vagy payload
   változik, új jelölt/hash kell és az érintett package/host/hallásos kapuk
   megismétlendők. Egy változott csomag nem örökölheti a régi végső PASS-t.
7. **Elfogadási dokumentáció és publikálási döntés:** a fenti pontos,
   változatlan csomagokra rögzített SHA/verzió/host/beállítás és
   PASS/FAIL/NOT RUN/SKIP jegyzék, HU/EN release notes és jóváhagyás.
   Ez a lépés nem cseréli újra a már tesztelt csomagba ágyazott dokumentumokat.
   Csak ezután külön publikálási engedély; régi tag/asset nem írható felül.

REAPER nélkül az 1–4. lépés automatizálható részei és a csomagoló/dokumentáció
előkészítése végezhetők. A valódi host- és hallásos eredmény nem szimulálható
CI-sikerrel. D4 AU/Logic, további D1 boot-diagnosztika és minden karbantartási
ötlet nem válik automatikusan e release kötelező részévé. Nincs új verziószám,
publikálási vagy valós gépre telepítési engedély ebből a döntésből.

E döntés dokumentálási körének ellenőrzése: PASS a háromfájlos, kizárólag
Markdown-diff és formaellenőrzése, 24 helyi fájlhivatkozás; Python 94 futott,
93 PASS / 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR. Új C++/DSP build,
privát firmware-render és REAPER ebben a dokumentációs körben NOT RUN.
A fenti #169 adatok ellenőrzött korábbi körből valók, nem új D5 teszteredmények.

#170 review-korrekció: a két tervellentmondás külön ellenőrzése javítás előtt
FAIL (aktív végrehajtási rész újra ütemezi a lezárt M5-öt; host kapu megelőzi
a verzió/csomag fagyasztást). Az alábbi aktuális sorrendben M5 már kész,
és végső host/hallásos PASS csak az 5. lépés exact jelöltjéhez tartozhat.
A korrekció dokumentációs; nem történt új csomagolás vagy hostteszt.
Javítás után PASS: öt sorrend/azonosság/státusz ellenőrzés, három dependency/
source-documentation guard teszt, 24 helyi fájlhivatkozás és diff-formaellenőrzés.
A teljes 94-es helyi Python-kör fent az előző `0a89ba3` head eredménye;
a korrekció új final-head platform/sanitizer CI-je és review külön kapu.

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
vitte a kizárólag dokumentációs támogatási döntést és ezt a PR-rendezési
checkpointot. Beolvadt `4b9e6864df868a1ce21a030ea3810f2b8a0244a3` main-nal;
a végleges `356ef05` head Windows/macOS/sanitizer CI-je PASS
(37735596350, 37735596360, 37735596354), nincs megoldatlan review-szál.
A merge utáni Windows/macOS futás is PASS (37737116633, 37737116561).
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
(nincs működésváltozás). A friss PR platform/sanitizer CI-je azóta a fenti
futásokkal PASS; a helyi eredmények önmagukban nem helyettesítették azt.

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
processor-bekötés, importált bankkiválasztás, GUI és induláskori scan beolvadt
(#154–#158, #160). A kapacitás-GUI tesztkör aktuális fejlesztési alapja:
`8d84de7bf10acf88dfdab722eef2221a38aa8428` main, #160 után.
Windows/macOS main ellenőrzés PASS (37742318734, 37742318665).
Ez nem a teljes D3 vagy valódi host-elfogadás.
Külön Windows/macOS bedobós mappában több szabványos 32-hangszínes
bank legyen egyszerre használható. Új példány beolvassa, nyitott példányban
explicit Refresh banks; külön a Factory Banks mappától és USER.vub-tól.

#### Első katalógus részfeladat

Kiindulás: `1675c83`, #153 után mindkét main platformellenőrzés zöld.
Az első részfeladatban az önálló `VDX7ImportedBanks` modul csak tesztcélba
volt bekötve. A harmadik részfeladat a processorhoz, #158 a GUI-hoz kapcsolta;
az induláskori scan az alábbi új checkpoint része. Az alábbi könyvtárak
az új fejlesztési buildek célútvonalai, nem a publikált 1.0.1 funkciói:
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

A teljes D3 nincs kész: #157 idején a GUI, induláskori scan/frissítés és
Windows/macOS host-elfogadás volt hátra; az újabb checkpointokat lásd alább.
A privát firmware-es processor-teszt nem helyettesíti a valódi REAPER-próbát.

#### Felületi bekötés – beolvadt #158

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
Azóta #158 beolvadt `d8e0da69e8822a67cbd032facc813600559a6e7b` main-nal,
a platform/sanitizer CI-kapuk ellenőrzése után. Helyi sanitizer és valódi
Windows/macOS REAPER elfogadás NOT RUN. Nincs új release vagy tag.

#### Induláskori bankscan – beolvadt #160, 2026-10-08

Ág: `feat/imported-bank-startup-scan`; alap a fenti `4b9e686` main.
Új production processor létrehozásakor egyszer, szinkron módon és az
audiofeldolgozáson kívül lefut a meglévő, csak olvasó scan. Nem hoz létre
mappát, nem rekurzív, az 512 bejegyzés / 128 SysEx / fájlonként 4104 bájt
korlát változatlan. Ez munka-/memóriakorlát, nem falióra szerinti időgarancia;
lassú vagy elakadt háttértár lassíthatja a példány létrehozását. Nem új szálon
fut, ezért késői worker-eredmény nem írhatja felül a host projekt-visszatöltését.
Az izolált `detectRom=false` tesztprocessor csak külön beadott mappával olvas.

Teljes scan kiválasztás nélküli, változtathatatlan katalógust publikál.
Érvényes részhalmaz hibás/duplikált fájl kihagyásával megjelenhet; limit- vagy
útvonalhiba esetén részleges katalógus nem kerül be. Hiányzó mappa szabályos,
üres könyvtár. Nincs automatikus bankváltás vagy RAM/Factory/USER módosítás.
Prepare/process/release, timer és későbbi ROM-betöltés nem indít új scan-t.
Új fájlokat új példány, vagy a nyitott példány explicit Refresh művelete lát.

A későbbi projekt-visszatöltés bankadatát használjuk, nem az aktuális
mappatartalmat. A régi projekt hiányzó `ImportedBanks` gyermeke is elsőbbséget
kap: null/legacy könyvtárat állít vissza, nincs hallgatólagos újrabeolvasás.
Az induláskori jelentés nem projektadat: az állapotszöveg tooltipjében ROM
nélkül is olvasható, betöltött ROM mellett UTILITY → Imported Banks → Startup
scan report alatt is. Legfeljebb nyolc részletes figyelmeztetés és a többi
száma látható. Kritikus állapotjelzés elsőbbsége megmarad; sikeres explicit
Refresh törli az induláskori jelzőt, sikertelen nem. A jelentés történeti marad.
HU/EN README és kézikönyv frissítve; nincs támogatási kör vagy release-módosítás.

Ellenőrzések:

- FAIL (elvárt reprodukció): csak a harmadik konstruktorparaméter beadását
  előkészítve, scan nélkül az új példány banklistája hiányzott. Futó teszt,
  nem fordítási hiba; a javítás után ugyanez PASS.
- PASS: MSVC Release `vdx7_ci_checks` és Windows `VDX7_VST3` build a meglévő,
  rögzített függőségű buildmappában; a módosított processor/editor/tesztek
  újrafordítva. Nem új, teljesen üres build vagy macOS helyi fordítás.
- PASS: végső 17/17 ROM-mentes CTest, köztük az induláskori lista, pontos bankadat,
  kiválasztás hiánya, missing/path/limit, részhalmaz/duplikáció/figyelmeztetés,
  két példány izolációja, projekt/legacy elsőbbség és explicit Refresh;
  GUI-szinten a startup lista és hibás fájl tooltipje ROM nélkül is.
- PASS: Python 78 tesztből 77 PASS / 1 SKIP (Windows symlink-jogosultság),
  0 FAIL/ERROR; tesztleltár-ellenőrző önellenőrzése és diff-formaellenőrzés.
- FAIL (teszt-előfeltétel): a 16 KiB-os privát v1.8 fájllal a teljes importált
  processor-integráció a gyári bankváltásnál megállt, mert nincs gyári bankadat.
  Ez nem teljes privát-suite PASS és nem bizonyított startup-regresszió.
- PASS: az ugyanilyen firmware-prefixű helyi 48 KiB-os, érvényes bankadatú
  kontrollal a teljes importált processor-integráció; startup után ROM-betöltés,
  kiválasztás, RAM/settings/factory megőrzés, recall és késleltetett visszatöltés.
  Firmware-prefix SHA-256: `6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
  A kontroll nem igazolja a módosítatlan Yamaha gyári bankok teljes elfogadását.
- PASS: külön 16 KiB-os v1.8 GUI-harness 5/5 bankválasztás és 5/5 dirty Mégse,
  startup tooltip, elavult popup és billentyűzetes ütemezés ellenőrzésével.
- PASS: #160 végleges `9c4f3c1` head Windows/macOS/sanitizer CI
  (37740080181, 37740080187, 37740080208); nincs megoldatlan review-szál.
  Beolvadt `8d84de7bf10acf88dfdab722eef2221a38aa8428` main-nal;
  a merge utáni platformellenőrzések is PASS (37742318734, 37742318665).
- NOT RUN: valódi Windows/macOS REAPER, AU/Logic, telepítő- és kiadási QA.

Hátravan D3-hoz: a következő jelölt
valódi Windows/macOS új példány/projekt-recall/Refresh/tooltip/görgetés próbái,
több bankkal és több példánnyal, régi projekt megnyitásával, eltűnt/cserélt
forrásfájl mellett is. A helyi tesztek nem új kiadás publikálási engedélyei.

#### Maximális banklista és példányizoláció – aktuális tesztfejlesztés, 2026-10-08

Ág: `test/imported-bank-capacity-ui`; alap a fenti `8d84de7` main.
Csak a meglévő `vdx7_gui_header` teszt és ez a terv változik. Nincs új
production viselkedés, DSP-/firmware-módosítás vagy új támogatási ígéret.
E körben nem reprodukáltunk új működési hibát: a hiányzó határeset-lefedettséget
pótoljuk, nem egy feltételezett bug kedvéért írjuk át a működő kódot.

Új automatikus lefedettség saját, külön ideiglenes mappában generált 128
szabványos bankkal, azonos hosszú Unicode-fájlnévprefix mellett:

- 137 választható banklistaelem (8 gyári + USER + 128 importált), változatlan
  ROM nélküli tiltás; minden importált sor ID/név/hash összerendelése.
- 1/2/3 számjegyű sorszám, legfeljebb 15 karakteres egyértelmű címke;
  teljes fájlnév és tartalmi azonosító tooltip; minden sor 260 × 26 méretű
  komponens-pillanatképe érvényes. Egyoszlopos popup konfiguráció megmarad.
- A hibás 129. SysEx-jelölt is beleszámít a limitbe: Refresh megőrzi a teljes
  processor-state-et és a 128 soros listát, startup nem publikál részlistát.
- A hibás startup utáni mentett projekt 128 bankot állít vissza, és a kritikus
  függőprojekt-jelzés elsőbbséget kap az induláskori figyelmeztetéssel szemben.
- Két példány külön, teljes és tartalmilag azonos induláskori katalógusa;
  fájltörlés és csak a második példány Refresh
  művelete nem módosítja az első processor/editor 128 bankját.
- Tényleges binary projekt-recall az összes szintetikus forrásfájl eltávolítása
  után is 128 bankot mutat, minden bank bájtjai és metaadatai egyeznek az
  eredeti katalógussal; az editor mind az öt támogatott méretben renderel,
  gyermekkomponensei a felület határain belül maradnak.
- Külön privát v1.8 harness-ben a tényleges editor választási útja a legutolsó
  (128.) és első bankot is helyesen kapcsolja, eltűnt forrásmappa mellett.

Eredmények: PASS az újrafordított helyi MSVC Release GUI-teszt (meglévő core/
plugin könyvtárakkal; nem új teljes pluginbuild), 17/17 ROM-mentes CTest,
78 Python-tesztből 77 PASS / 1 SKIP (Windows symlink-jogosultság), 0 FAIL/ERROR,
tesztleltár-ellenőrző önellenőrzése és diff-formaellenőrzés. Privát v1.8
kapacitás-harness 5/5 teljes futás PASS (minden futás első/utolsó választással).
Firmware SHA-256 az előző checkpointtal azonos; nincs privát adat commitban.

Negatív kontroll: `--imported-bank-capacity-negative-control` szándékosan
WRONG címkét állít be az utolsó menüelem szövegmezőjében; a teszt a várt címke-összerendelési
ellenőrzésnél FAIL/exit 1. A normál futás változatlan PASS. Ez önálló tesztmód,
nem production módosítás és nem a normál CTestbe regisztrált hibás teszt.
Privát opt-in parancs: `vdx7_gui_header_tests --imported-bank-capacity-rom <privát-v1.8-ROM>`.

#161 review-javítás, 2026-10-08: a kezdeti `9e0a9df` head mindhárom CI-je
PASS (37744007512, 37744007466, 37744007450), de két jogos P2 tesztlefedettségi
észrevétel blokkolta a merge-et. A puszta pointer-különbség nem bizonyította
a második példány teljes startup katalógusát, és a recall csak a bankszámot
és az utolsó bank bájtjait ellenőrizte. Ezek nem bizonyított production hibák.

A javítás a második startup pillanatkép létezését, 128 bankját, külön tulajdonát
és teljes egyezését is ellenőrzi, még az editorok létrehozása előtt. A forrás
nélküli recall és a hibás startupot felülíró project-recall minden bank
`packed`, `contentId`, `fileName`, `displayName` mezőjét, a bankok sorrendjét
és a katalógus `selectedId` mezőjét összehasonlítja az `original` értékkel.
Null pillanatkép nem dereferálódik, hanem az összehasonlítás elutasítja.

Reprodukció: a régi bankszám/utolsó-bank összehasonlítás mellé beillesztett
kontroll az első bank egy bájtját módosítja, a bankszám és az utolsó bank
változatlan. Az új követelmény a régi összehasonlítással elvárt FAIL/exit 1
eredményt adott (`catalog comparison rejects nonfinal bank data changes`).
A teljes összehasonlítással a normál teszt PASS. Nyolc beépített negatív
kontroll védi: null, hiányos katalógus, nem utolsó bank bájteltérése, bankok
felcserélése, hibás azonosító/fájlnév/megjelenítési név és eltérő kiválasztás.
Az egyező külön másolat pozitív kontroll; a kontrollok csak saját, memóriabeli
tesztadatot módosítanak, nem a processort, fájlt vagy privát firmware-t.

Review-javítás végső helyi eredménye: PASS újrafordított MSVC GUI-teszt,
17/17 ROM-mentes CTest, 77 Python PASS / 1 jogosultsági SKIP, 0 FAIL/ERROR,
5/5 privát v1.8 kapacitás-harness; a korábbi címke-negatív mód továbbra is a
várt címke-ellenőrzésnél FAIL/exit 1. Tesztleltár-önellenőrzés és diff-forma PASS.
Az új head CI/review kapuja külön szükséges; a régi head zöld eredménye nem
engedélyezi az új head beolvasztását. Production kód és release változatlan.

#161 lezárási checkpoint, 2026-10-08: a végleges `79c89b0` head mindhárom
ellenőrzése PASS (Windows 37748503315, macOS 37748503316, sanitizer 37748503319),
mindkét review-szál megoldott. A PR beolvadt
`1568c2c112658d715ded26c00c76243e3e25a67e` main-nal; a külön main Windows
37749995687 és macOS 37749995747 futás is PASS. Az alábbi NOT RUN platform-PR
kapu e régi körre már lezárult; a valódi host és instrumentált GUI továbbra sem PASS.

NOT RUN: e kör új PR platform/sanitizer CI-je és végleges review (PR után
ellenőrizendő); instrumentált GUI-harness (a meglévő sanitizer workflow
nem futtat GUI-tesztet); valódi popup-görgetés/kijelző-DPI, billentyűzetes navigáció
128 bank között, audio többpéldányos terhelés, REAPER/Logic, telepítő/kiadási QA.
A sorok és az editor érvényes renderképe nem vizuális vagy hallásos hostelfogadás.
A D3 fenti valódi hostkapui továbbra is nyitottak; nincs release/tag/asset változás.

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
A 2026-10-09-i döntés alapján a következő release kötelező része; a fenti
release-scope kapuk irányadók. A végleges műszaki megoldás még megvalósítandó.

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

#### D5 első upstream mérési prototípus — 2026-10-08

REAPER nélkül elvégezhető, teszt-only részlépés az engedélyezett D5 irányhoz;
nem a következő kiadás funkciókörének eldöntése. Friss baseline:
`1568c2c112658d715ded26c00c76243e3e25a67e`. Ág:
`test/classic-clean-dsp-prototype`. [Részletes mérés és reprodukció](../validation/CLASSIC_CLEAN_UPSTREAM_PROTOTYPE_20261008.md).
Pull request: [#162](https://github.com/RobCZart82/VDX7-JUCE/pull/162), beolvadt.
Végleges `698ab5b` head Windows 37751934159, macOS 37751934313 és sanitizer
37751934179 PASS; nincs review-szál vagy review-tiltás. Merge main:
`e528fe8b4f8e067dc6877aa5495a23f91a5ac813`. Külön main Windows 37755305211
és macOS 37755305412 PASS. Az alábbi korábbi PR NOT RUN már e körre lezárt.

Az új `vdx7_sound_mode_prototype` ROM-mentes teszt a rögzített upstream
inverse-log táblázatot mindkét polaritással, 192 OPS ingert és 4 szintetikus
EGS jelút/átmeneti esetet vizsgál. Default/explicit Classic és már futó állapoton
óraléptetés nélküli true/false körút bitazonos; Clean determinisztikus és
mérhetően eltér, a peer példány változatlan. A Clean-kérést figyelmen kívül hagyó
negatív mód az elvárt ellenőrzésnél FAIL/exit 1. A két EGS út eltérő gainnel
dolgozik; Clean nem lépteti a Classic filter-historyt. A visszatérés a folyamatos
Classic kontrolltól eltér, ezért nyers bool-váltás még nem kattanásmentes elfogadás.

PASS helyben: MSVC CI-tesztcél build, 18/18 ROM-mentes CTest, Python 77 PASS /
1 symlink-jogosultsági SKIP, 0 FAIL/ERROR; tesztleltár és negatív kontroll.
Az új cél normál CI és ASan/UBSan kiválasztásában szerepel, de az új PR CI/review
eredménye külön ellenőrizendő, egyelőre NOT RUN. Production kód, SETTINGS,
paraméterek/állapotformátum, firmware, resampler és kiadás változatlan.

NOT RUN: privát v1.8 teljes pipeline/reset/betöltési/módmegőrzési teszt,
állapotmentés/GUI, spektrum/CPU/latencia/allokációs mérés, loudness-matched
meghallgatás, valódi host mátrix. Következő részlépés: bounded átmenet/history
és headroom prototípus, majd állapot/audio-határi kérés specifikációja;
csak ezek után minimális production implementáció és SETTINGS. A részletes
jelentés nem javult hangminőség, teljes D5 elfogadás vagy publikálási engedély.

#### D5 korlátozott váltási prototípus — 2026-10-08

Kiinduló main: `e528fe8b4f8e067dc6877aa5495a23f91a5ac813`; ág:
`test/classic-clean-bounded-transition`. [Részletes mérés és kapuk](../validation/CLASSIC_CLEAN_BOUNDED_TRANSITION_20261008.md).
Pull request: [#163](https://github.com/RobCZart82/VDX7-JUCE/pull/163), beolvadt.
Végleges `6299bcc` Windows 37758436342, macOS 37758436321 és sanitizer
37758436437 PASS; nincs review-szál vagy review-tiltás. Merge main:
`0a8b1bdaa5e99652b4aa2a1958dca04e9ba7f152`. Külön main Windows 37760679834
és macOS 37760679901 PASS. Az alábbi régi PR NOT RUN e körre már lezárt.
Teszt-only egy-EGS lehalkítás → nulla gainen váltás → visszaerősítés jelölt.
Kísérleti 256 + 256 natív mintás rámpa (~10.43 ms), nincs második firmware-motor.
Latest request wins; ismételt kérés nem indítja újra, visszavonás/gyors kérés
korlátozott gain-lépésű. A kérések megszűnése után legfeljebb 512 mintán belül
teljes gainen a kívánt mód fut. Tartósan sűrű kérések hosszan halkíthatnak.

PASS helyben: teljes MSVC CI-tesztcél build, 18/18 ROM-mentes CTest, Python
77 PASS / 1 Windows symlink SKIP, 0 FAIL/ERROR; mute-megkerülés és ignore-clean
negatív kontroll elvárt FAIL/exit 1. Idle wrapper bitazonos Classic EGS-sel;
négy szintetikus EGS inger kétirányú váltása véges, nulla natív váltási mintával.
Azonos native-offset requestek mellett 1/7/64/511/2048 partíció trace-egyezés.
Ez nem valódi host-buffer/GUI scheduling vagy sűrű teljes EGS váltás elfogadása.

Production, SETTINGS, állapotformátum, 148 paraméter és kiadás változatlan.
NOT RUN: új PR végső CI/review, privát v1.8 lifecycle/állapot/MIDI/tail,
SRC utáni átmenet, CPU/allokáció/host és meghallgatás. Rövid mute-dip és régi
Classic filter-history marad: nincs általános kattanásmentességi PASS. Következő
részlépés az explicit kívánt/aktív mód és projekt/audio-tulajdonosi/lifecycle
specifikáció, majd az érintett privát regresszió és csak utána production/GUI.
Kísérleti hossz és gain-politika nem végleges hangzási szerződés.

#### D5 állapot- és kérésátadási szerződés — 2026-10-08

Baseline: `0a8b1bdaa5e99652b4aa2a1958dca04e9ba7f152`; ág:
`test/classic-clean-state-contract`. [Részletes műszaki szerződés](CLASSIC_CLEAN_STATE_CONTRACT.md)
és [soros tesztmodell validációja](../validation/CLASSIC_CLEAN_STATE_CONTRACT_20261008.md).
Pull request: [#164](https://github.com/RobCZart82/VDX7-JUCE/pull/164), beolvasztva
2026-10-08: `328d93992a764dbd1ac89a50feb3ad4e1e2692b3`.
Végleges head `5386aafc049f2e43a1375ac4e5134651e956fec7`: Windows
`37766081260`, macOS `37766081404`, sanitizer `37766081269` PASS;
nem volt nyitott review thread vagy elutasító review.
Ez nem production codec/lock/SETTINGS, hanem annak explicit specifikációja
és teszt-only modellel ellenőrzött invariánsai. A meglévő `VDX7STATE` save/
pending/restore és engine native/SRC belépési pontokat a fenti SHA-n áttekintettük.

Jelölt feature-séma: `soundModeVersion=1`, `soundMode=0/1` root-metaadat.
Mindkettő hiánya legacy Classic; jelenlévő hibás/fél pár vagy ismeretlen verzió
egész restore-elutasítás, nem fallback-mutation. A kívánt mód mentendő, nem
rámpa/aktív mód. Per-instance current-project revision védi a stale UI-kérést
és későn befejezett restore-t; pending módszerkesztés menthető, de unrelated
engine-re nem dispatcholható. UI try-lock BUSY/STALE nem rejtett queue; audio
a már meglévő owner-lock határán figyeli a latest desired értéket. E valódi
lock/codec/admission még implementálandó, nem a soros modellből bizonyított.

PASS helyben: teljes MSVC CI-tesztcél build, 18/18 ROM-mentes CTest; Python
77 PASS / 1 symlink-jogosultsági SKIP, 0 FAIL/ERROR; három negatív kontroll,
tesztleltár és diff. Az új stale-token-megkerülés elvárt FAIL/exit 1; a normál
modell PASS. Minden production mező/paraméter/hangmotor és release változatlan.
E checkpoint idején NOT RUN: JUCE typed/binary codec, valódi thread/
reentráns save, natív overshoot/SRC ordering, privát v1.8 lifecycle és SETTINGS/
REAPER/hallásos kapuk. Következő kör: valódi codec és processor ownership
regresszió, majd engine/firmware integráció és UI, e sorrendben.

#### D5 valódi JUCE property/XML/binary kódoló — 2026-10-08

Baseline: `328d93992a764dbd1ac89a50feb3ad4e1e2692b3` (#164 merge);
ág: `test/classic-clean-juce-state-codec`.
[Részletes reprodukció és átadási jelentés](../validation/CLASSIC_CLEAN_JUCE_CODEC_20261008.md).

Elkészült az önálló `Source/VDX7SoundModeState.h`: a valódi `juce::var`
int/int64/string típusát és pontos értékét ellenőrzi, legacy root esetén
Classic-ot ad vissza; hibás/fél pár, ismeretlen verzió vagy rossz root esetén
elutasít és nem módosítja a caller kívánt módját. A writer deep detached
ValueTree-másolatba csak a kívánt Version/Mode párt írja, az eredeti rootot és
gyermekeket nem módosítja. **Nem kötöttük be a PluginProcessorba:** a plugin
nem ment/alkalmaz új Clean állapotot, SETTINGS/DSP és 148 paraméter változatlan.

PASS helyben: MSVC teljes CI-tesztcél build; 19/19 ROM-mentes CTest, benne
`vdx7_sound_mode_state`; tényleges JUCE XML és AudioProcessor binary körút,
szintetikus RAM és az összes egyéb tree-adat megőrzése; mindkét negatív kontroll
elvárt FAIL/exit 1; CTest leltár és checker self-test. A sanitizer workflow az
új cél fordítását és futtatási kiválasztását is tartalmazza; ez konfigurációs
ellenőrzés, nem helyi ASan/UBSan eredmény.

Python regresszió: 77 PASS / 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR.

Nyitva: e rész-PR final-head Windows/macOS/sanitizer és review; valódi processor
restore/save/pending/UI/audio ownership és determinisztikus versenytesztek;
engine overshoot/SRC/lifecycle, privát v1.8 Classic null-difference, SETTINGS,
valódi REAPER és hallásos elfogadás. A soros modell és e codec együtt sem
bizonyít production race-biztonságot vagy jobb hangminőséget. Következő logikus
kör a processor owner tranzakció és regressziói, nem a kapcsoló korai aktiválása.

#### D5 önálló zárolásos állapottulajdonos

2026-10-08, baseline `9ee7c9d6ff85381e853fb0fd7c2e7c13f78322d3`;
ág: `test/classic-clean-threaded-ownership`. A baseline Windows/macOS CI-je
PASS (37832216599, 37832216642); #165 és #166 beolvadt. A fenti codec-kör
korábbi PR-kapui teljesültek, a valódi processor/DSP/host-kapuk nem.
Pull request: [#167](https://github.com/RobCZart82/VDX7-JUCE/pull/167), nyitott;
friss final-head platform/sanitizer CI és review szükséges a merge előtt.

Új önálló `Source/VDX7SoundModeOwner.h` az átadott engine-mutexet használja:
kívánt mód és projekt-revízió egy tranzakcióban; UI/audio try-lock BUSY esetén
nincs várakozás vagy rejtett queue; stale UI/completion és revíziótúlcsordulás
nem mutálhat projektet. A host-értesítés a lockon kívül fut. A mode/payload
mentési pillanatkép ugyanazon lock alatt mély másolat, a kódolás lockon kívüli.
A pending completion nem írja vissza a tree-ben maradt régi módot.

Ez **nem PluginProcessor-integráció**: a komponens teljes projekt/ROM admissiont
a callertől vár, a teszt saját szintetikus payloadot használ. A valódi processor
state epochja, mono-policy mellékhatásai, ROM-installja és engine/SRC útja még
nincs ehhez kötve. Nincs Clean-metaadat vagy kapcsoló a működő pluginban.
[A pontos tesztkör és eredmények](../validation/CLASSIC_CLEAN_THREADED_OWNERSHIP_20261008.md).

Valódi szálak és vezérelt rendezvous teszteli a pre/post-lock audio-olvasást,
stale UI-t és completiont, pending editet, külön példányokat, reentráns save/
recallt és a capture után érkező új projekt melletti JUCE binary-mentést.
Két hibás tesztadapternek elvárt FAIL-t kell adnia. A platform/sanitizer CI és
review új kapu; a szálas harness sem teljes production versenybiztonsági bizonyíték.
Helyi PASS: teljes macOS ARM64 CI-tesztcél fordítás, 20/20 ROM-mentes CTest
ASan/UBSan mellett, új ownership teszt 50 ismétlésben, mindkét negatív kontroll
elvárt FAIL/exit 1; 78 Python-teszt, leltár és 138 helyi dokumentációs hivatkozás.
ThreadSanitizer és tényleges processor/DSP/host-integráció NOT RUN.
Az első #167 final-head macOS/sanitizer build fordításkor elbukott: Xcode 15.4
nem biztosított `std::jthread`-et. A teszt ezt automatikusan joinoló `std::thread`
wrapperre cseréli, új normál/kivételes élettartam-próbákkal; a többszálú és
negatív kontrollok változatlanok. A fenti helyi tesztkör a javítás után ismét
PASS; az új final-head platform/sanitizer CI külön, kötelező megerősítés.
Következő kör: a tényleges processor tranzakcióihoz illesztés és epoch/mono/
ROM/pending regresszió, a DSP nélküli Clean-szállítás tilalmának megtartásával.

#### #167 ROM-admission versenyhelyzet javítása — 2026-10-09

Main baseline: `9ee7c9d6ff85381e853fb0fd7c2e7c13f78322d3`; PR kiinduló head:
`2ebe4bed1cea3cc03b043cc24c68e3b190bb12cf`. Helyi javítóág:
`fix/pr167-rom-admission`, a javítás a meglévő #167
`test/classic-clean-threaded-ownership` ágára kerül, nem új competing roadmap.

A kiinduló head Windows `37839598022`, macOS `37839598025` és sanitizer
`37839598014` CI-je PASS, de a review valós komponenshibát talált: a lock előtt
számolt `compatible`/`engineReady` bool elavulhatott egy ROM-csere miatt, miközben
a projekt revisionje nem változott. A két új vezérelt szálas reprodukció a
javítás előtt külön-külön FAIL/exit 1: téves completion, illetve téves ready
project install. Ez nem kiadott pluginhibára vonatkozó állítás: nincs production
bekötés, nincs valódi ROM a fixture-ben.

Mindkét owner API most nem dobó predikátumot kér, amely a mai védett ROM/engine
identitást **a közös mutex alatt** olvassa. Az admission és a payload commit
között a lock nem oldódik fel. Bool és dobó predikátum fordításkor elutasított.
Stale/non-pending/exhausted/invalid művelet nem futtathat checket vagy commitot;
mai mismatch completion snapshot-mutation nélkül elutasított. Mismatch mellett
egy teljesen valid új projekt installja továbbra is megengedett pendingként,
nem tévesen readyként. A predikátum bounded, readonly; nincs file/hash/host/reentry,
és a commit nem érvénytelenítheti a vizsgált kompatibilitást.

PASS eddig helyben: mindkét eredeti repro javítás után, teljes owner teszt,
50 egymást követő ismétlés; négy negatív kontroll elvárt FAIL/exit 1 (köztük
a két új cached-ROM adapter); same-lock predicate/commit és guard tesztek.
PASS: teljes Windows MSVC CI-tesztcél build és 20/20 ROM-mentes CTest; Python
78 futott = 77 PASS / 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR;
leltár/checker self-test, 23 helyi dokumentumlink és diff-ellenőrzés.
Új final-head Windows/macOS/ASan/UBSan és review még külön merge-kapu;
a korábbi zöld head nem az új javítás tesztje. Helyi sanitizer/TSan NOT RUN.
[Részletes reprodukció és friss eredmények](../validation/CLASSIC_CLEAN_THREADED_OWNERSHIP_20261008.md).
Production processor ROM-readiness érvénytelenítés egy későbbi ROM-cserénél,
epoch/mono/DSP/SRC/REAPER továbbra is külön integrációs kapu; nincs új D5 vagy
release elfogadás, dependency pin vagy firmware-policy változás.

### D5 #167 lezárási checkpoint — 2026-10-09

A fenti #167-kör korábbi nyitott PR/CI státuszait felülírja: a végleges
`4239959d18dfcbf60ee339f723c10e70647ab830` head Windows `37886851289`, macOS
`37886851290`, ASan/UBSan `37886851376` ellenőrzése PASS, review rendezett;
#167 beolvadt `e4000390188618d63acce1301a1d13a6616ae9ed` main-ba.
A merge-t a mai folytatás indulásakor már készen találtuk. Merge utáni main
Windows `37888604500` és macOS `37888604585` PASS. A production D5 integráció,
Classic null-difference, SETTINGS és valódi host/hallásos kapuk változatlanul
nyitottak; az önálló owner nem kész Clean funkció.

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
- M5: JUCE-frissítés külön, visszavonható munkacsomagban, előzetes haszon/kockázat
  mérlegeléssel és az alábbi kompatibilitási kapukkal. A 9.0.3 update #169-cel
  main-ban, automatizált kapui PASS; új bináris valódi host-elfogadása még nyitott.

### M5 JUCE-frissítés: előzetes mérlegelés és regressziós kapuk

#### Aktuális M5 JUCE 9.0.3 kompatibilitási kísérlet — 2026-10-09

Baseline `bc8c1664ad13a11208773d5b227cb113033555ec`; ág
`maintenance/juce-903-compatibility`. #168 már beolvadt: végleges head
Windows/macOS/sanitizer és merge utáni main Windows/macOS PASS, nincs nyitott
review. A provenance előfeltétel lezárult; az alábbi régi #168 checkpoint
nyitott PR-kapuja teljesült, nem új blocker.

Külön jelöltben a CMake + creation packager + aktív dependency útmutató és
THIRD_PARTY JUCE pinje 9.0.3 / `be29c81492b6151c8ea8d14c840e1311963b3a83`.
Main-ba #169-cel bekerült; új release-elfogadás még nincs. Reprodukált és javított historical source
verification regresszió: az új verifier olvashatja az explicit régi 9.0.1 tuple-t,
de új creation nem használhat történeti vagy ismeretlen pint. Az approval és
a kiadott assetek változatlanok.

Új privát processor comparator: 30 rate/buffer/mono-policy eset, közvetlen
PCM null-difference, binary state/recall hash, reset/reload/pending és véges,
nem néma kontrollok. Régi JUCE ismétlési kontroll PASS 5 760 512 mintára.
Tiszta régi/új build és összevetés PASS: mind a 30 esetben nincs PCM/state/
recall/metric eltérés. Új Windows VST3 + Standalone build, 20/20 ROM-mentes
CTest, 50 ownership ismétlés és source configure/Git guard PASS; Python
94 tesztből 93 PASS / 1 Windows symlink SKIP. További 10/10 célzott privát v1.8
CTest PASS; a régi JUCE-val is hibázó Init kontroll saját seed-fixture-jét
javítottuk, production működést nem módosítottunk. A teljes source csomag
első próbáját blokkoló két személyes dokumentációs buildútvonal javítva,
guard változatlan. Új 5704 fájlos source-archive + bundled checker + friss
disconnected VST3 build PASS; a valódi korábbi 5131 fájlos 1.0.1 source archive
az új verifierrel is PASS. #169 beolvadt `000ee97b19763b23c569ff6ac68080c4b6165094`
main-nal; végleges head `cc4a0e016edeaf37e42f0587c57dbfe3edce1dc9` Windows
`37900539006`, macOS `37900539025`, ASan/UBSan `37900539002` PASS; nincs nyitott
review-szál. Merge utáni Windows `37902135282` és macOS `37902135295` PASS.
Ezek lezárják e dependency-PR automatizált kapuját, nem a D5-t vagy új host-QA-t.
[Pontos kör és eredménykövetés](../validation/JUCE_903_COMPATIBILITY_20261009.md).
E kísérlet nem D5 DSP integráció, T1–T4 host-elfogadás vagy release approval.

#### M5 előkészítő checkpoint (#168) — 2026-10-09

Baseline `e4000390188618d63acce1301a1d13a6616ae9ed` main; ág
`fix/build-provenance-source-binding`. A #167 önálló részlépés lezárult.
Pull request: [#168](https://github.com/RobCZart82/VDX7-JUCE/pull/168),
nyitott; final-head platform/sanitizer és review a merge-kapu.
JUCE 9.0.3 jelölt teljes SHA ellenőrizve:
`be29c81492b6151c8ea8d14c840e1311963b3a83`; **GO a külön kompatibilitási
kísérlethez, nem pin-merge/kiadás elfogadás**. A dependency pin ma még 9.0.1.

Előbb reprodukált provenance-rés javítása: a jelentéshez megadott helyes
Git-másolat eddig nem volt a CMake által ténylegesen kiválasztott source-hoz
kötve. Három mismatch-repro javítás előtt FAIL, utána PASS. Új configure
megfigyelési rekord, path/cache kötés és változatlan Git SHA/tisztaság guard;
régi projecthez az exact approval/workflow checkoutból származó CMake hook.
Ez nem írja át a régi product/packager/approval azonosságot vagy release assetet.

Helyi PASS: provenance 15/15, tényleges CMake kombinációk, valódi pinelt Git
dependency konfiguráció és mismatch-kontroll, Windows CI-tesztbuild, 20/20
ROM-mentes CTest; Python 85 futott = 84 PASS / 1 Windows symlink SKIP.
Új PR CI/review és installer dispatch még külön kapu; új JUCE build/audio/
host elfogadás NOT RUN. [Bizonyíték, hatókör és következő lépések](../validation/BUILD_PROVENANCE_SOURCE_BINDING_20261009.md).
E guard lezárása után külön pin/csomagolás frissítési kísérlet következik az
alábbi M5 kapukkal; nincs Classic/Clean production változtatással összevonás.

#### Eredeti M5 specifikáció és kapuk — 2026-10-08

Felhasználói kérés és tervezési checkpoint: 2026-10-08. Ellenőrzött main:
`0dafa214d75d6882939b51148d9ec46ec6734710` (a #166 main-frissítése után).
Státusz: **TERVEZETT**.
Ebben a dokumentációs körben nincs CMake/dependency/build/source-package vagy
production módosítás; a frissítés, új build és runtime elfogadás **NOT RUN**.

**Kiindulás és céljelölt.** A main `CMakeLists.txt` JUCE pinje
`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`, amelynek upstream verziója 9.0.1.
A vizsgálandó jelölt 9.0.3, **nem** mozgó `master`/`develop` vagy tagre hagyatkozó
automatikus frissítés. Végrehajtáskor ismét ellenőrizni és teljes commit SHA-val
rögzíteni kell a jelöltet; e terv még nem választ új csomagolási pinértéket.
Ez azonos 9.0-s sorozaton belüli update, de ettől nem kockázatmentes.

**Haszon, nem ígéret.** A hivatalos 9.0.2/9.0.3 változáslista CoreAudio
sample-rate/buffer/default-device és Multi-Output javításokat, Windows ablak/
fókusz javításokat, valamint MIDI/grafikai/hosting változásokat tartalmaz.
CoreAudio-eszközkezelés elsősorban a standalone változatnál érdekes; REAPER-ben
az audioeszközt a host kezeli. A VST3 **hosting** javítás nem automatikusan a
VDX7 mint VST3 **plugin** hibajavítása. OpenGL/UMP/egyéb funkció csak akkor
indok a frissítésre, ha a használt wrapper/standalone út ténylegesen érintett.
A VDX7 FM-motorja és SRC-je külön komponens: nincs automatikus jobb hangzás,
kisebb CPU vagy kattanásmentesség állítás. Először az érintett upstream diff és
a saját használat összevetése alapján kell GO / NO-GO döntést rögzíteni.

**Előre figyelendő kockázatok.** Ezek lehetséges regressziók, nem talált VDX7-hibák:

- API/fordítás: a 9.0.2 változtatja a `ThreadPool::addJob` callable szerződését
  és eltávolítja a `getMidiInputSelectorListBox` API-t; 9.0.3-ban a
  `SystemStats::isOperatingSystem64Bit()` jelentése pontosodik. Saját Source,
  tesztek és a ténylegesen használt JUCE wrapper útjai ellenőrzendők, nem csak
  verziószámcsere. Régebbi 9.0.1 migration tételek nem új 9.0.3 regressziók.
- GUI: betűméret/szövegelhelyezés, SVG/PNG, HiDPI, öt fix méret, fókusz,
  billentyűzet, popup és natív fájlválasztó eltérhet. Nem GUI-újratervezés:
  screenshot/teszt összehasonlítás és célzott adaptáció kell, ha eltérés lesz.
- Wrapper/projekt: 148 paraméter ID/sorrend/normalizálás, pluginazonosítók,
  VST3/AU csatorna/MIDI/lifecycle, XML/binary restore és host-notification
  regresszió kockázat. Régi project, ROM nélkül/pending restore, USER/IMPORTED/
  dirty állapot és több példány nem sérülhet.
- Audio/standalone: device start/stop/default switch, buffer/sample-rate,
  latency/tail/első blokk, MIDI időbélyeg és sustained voice ellenőrzendő.
  Változatlan Retromulator/SRC mellett is lehet eltérés a wrapper kimenetében;
  a Classic null-difference hiányát nem lehet motorváltozás hiányából levezetni.
- Build/csomag: compiler/SDK/minimum OS, universal arm64+x86_64, függőségi
  forráscsomag, notices és provenance eltérések. A CMake cache
  `FETCHCONTENT_SOURCE_DIR_JUCE` vagy vendored `third_party/JUCE` felülírhatja
  a letöltési pint: a tényleges fordított forrás identityjét is igazolni kell.
  Egy régi cache-sel zöld build nem az új JUCE tesztelése.

**Logikus sorrend és megállási pontok:**

1. Zárjuk le a folyamatban lévő önálló D5 részlépést, és rögzítsünk friss,
   zöld baseline-t. Az M5 update külön PR/ág legyen, ne ugyanabban a commitban
   Classic/Clean DSP/SETTINGS, bankfunkció vagy Retromulator-frissítés.
   Jelölthöz közeledő release közben ne kezdjünk indokolatlan dependency cserét.
2. Jelölt SHA + changelog/breaking diff + érintett VDX7 útvonalak és indoklás.
   Ha nincs releváns előny vagy elfogadható ellenőrzési környezet, a halasztás
   érvényes NO-GO; a jelenlegi JUCE nem pusztán korától bizonyított hibás.
3. Tiszta külön build a régi és az új pinhez, azonos VDX7 kód/konfiguráció/
   fixture mellett. Csak a szükséges kompatibilitási adaptáció engedett.
   CMake pin, `scripts/package_source.py` `JUCE_SHA`, a manifest/acceptance/
   provenance ellenőrzések, source-archive tartalom és a README-ből hivatkozott
   [nyilvános függőségi útmutató](../guides/SOURCE_DEPENDENCIES.md) JUCE-verziója,
   teljes revision SHA-ja és fordítási útmutatása legyenek összhangban.
   Rögzítsük a CMake által ténylegesen használt JUCE-forrás elérési útját és
   azonosságát: Git checkoutnál a tiszta fa teljes commit SHA-ját, kicsomagolt
   corresponding-source esetén az ellenőrzött manifestet és fájlhasheket.
   A cache `FETCHCONTENT_SOURCE_DIR_JUCE` és a vendored `third_party/JUCE`
   felülírásait is ellenőrizzük; eltérő forrás mellett a build nem igazolja
   a jelölt tesztelését. Az útmutató szerinti tiszta és offline buildnek is
   ténylegesen az új SHA-hoz tartozó forrást kell használnia.
   Régi release/tag/asset vagy elfogadási bizonyíték nem írható át; történeti
   útmutatók verzióadatai nem keverhetők az új jelölt fordítási lépéseivel.
4. Windows x64 és macOS universal VST3/AU/Standalone build; teljes regisztrált
   ROM-free CTest (nem fix régi tesztszám), Python leltár/csomagolási tesztek,
   ASan/UBSan, GUI és régi/új XML-binary állapotkörút. Az aktuális D5 codec
   tesztet is futtatni kell, ha addig main-ba került. E kör REAPER nélkül
   megkezdhető, de CI nem bizonyít teljes valódi hostkompatibilitást.
5. Privát eredeti v1.8-mal azonos Classic bemenet/állapot mellett determinisztikus
   összevetés: hangkimenet hash/null-difference, első eltérés, peak/RMS,
   44.1/48/96 kHz, reset/reload/pending és MIDI/mono/dirty regresszió. Eltérés
   esetén okfeltárás és dokumentált elfogadási döntés, nem automatikus PASS
   vagy tolerancia lazítása. Yamaha-adat/audio nem kerül repositoryba/artifactba.
6. Új bináris valódi standalone és REAPER VST3/AU smoke: betöltés, hang/MIDI,
   save/reopen, GUI, több példány, buffer/sample-rate és offline render az
   érintett T1–T4 körből. Hiányzó helyi DAW esetén **NOT RUN**, régi csomag
   felhasználói PASS-a nem az új binary eredménye. Valódi hostteszt vagy külön,
   tételes maintainer-halasztás a kiadási kapunál szükséges.
7. Final-head Windows/macOS/sanitizer zöld + rendezett review után component
   merge, majd main Actions külön ellenőrzése. Nyitott hiba, indokolatlan hang/
   projekt/GUI eltérés vagy csomag-provenance mismatch mellett STOP/NO-GO.
   Kiadás csak az új binary csomag-/host-kapuin és külön publikálási engedélyen
   keresztül; merge és publication nem ugyanaz a döntés.

**Ütemezési hatás és visszaút.** Igen, a JUCE-csere késleltethet mérföldkövet:
API-adaptáció, mindkét platform teljes rebuildje, új regresszió diagnózisa és
a valódi hostkörnyezet elérhetősége plusz munkát okozhat. Pontos időígéret
csak az elővizsgálat/első build után adható. A D5 hasznos, független munkája
közben haladhat, de ugyanazon builden ne keverjük az M5 és D5 változtatásokat;
merge után a még nyitott feature PR-eket friss main-en ismét ellenőrizzük.
Ha a céljelölt előnye nem arányos a kockázattal/idővel, M5 halasztandó, nem
kell miatta teljes release-t feltartani, kivéve igazolt releváns blocker esetén.
Merge előtt branch félretehető; utána szokásos revert PR állítsa vissza együtt
a korábbi JUCE pint/adaptációkat/csomagolási adatokat, új ellenőrzéssel.
Nincs force push, main-reset, régi tag mozgatása vagy asset felülírás.

Végrehajtáskor ide rögzítendő: baseline és jelölt SHA, érintett diff/indok,
GO/NO-GO és halasztás oka, PR, **PASS/FAIL/NOT RUN/SKIP** eredmények, tényleges
dependency provenance, host/architektúra/csomagazonosság, végső kockázati és
visszaállítási döntés. Ma nincs elvégzett update vagy új binary elfogadás.

Források: [rögzített 9.0.1 upstream verzió](https://github.com/juce-framework/JUCE/blob/e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8/CMakeLists.txt),
[9.0.3 változáslista](https://github.com/juce-framework/JUCE/blob/9.0.3/CHANGE_LIST.md),
[breaking changes](https://github.com/juce-framework/JUCE/blob/9.0.3/BREAKING_CHANGES.md).

## Végrehajtás és kiadási kapuk

Aktuális sorrend a 2026-10-09-i döntéssel: új reprodukált blocker, ha lesz →
D5 valódi processor/állapot integráció → motor/átmenet és privát mérések →
SETTINGS → platformkapuk → új verzió/csomagoló és csomagba kerülő HU/EN útmutató →
exact jelölt build/csomagteszt/fagyasztás → ugyanazon csomagból valódi
host/hallásos elfogadás → elfogadási jegyzék és külön publikálási döntés.
D1/D2/D3 már elkészült működését regresszióval megőrizzük; D4 külön jóváhagyandó
host/packaging munkacsomag. A D5 a következő kiadás része, nem későbbi opcionális
release-feature. A fenti részletes kiadási kapuk felülírják a korábbi nyitott
scope/prioritás kérdést; a dokumentálás nem megvalósítás vagy publikálás.

M5 JUCE 9.0.3 dependency-update: **#169-cel már main-ban, automatizált kapui
lezártak**; nem ütemezünk új pin-cserét vagy GO/NO-GO kísérletet a D5 után,
és nem kell megismételni a lezárt dependency-fejlesztést. A korábbi sorrend
történeti indoklása az M5 dátumozott specifikációjában marad, nem aktív feladat.
M5-ből csak az új kiadási jelölt tényleges standalone/REAPER host-elfogadása
maradt nyitott; a release-scope 5–6. lépéséhez kapcsolódik, nem akadálya a
D5 implementáció indításának. Régi bináris vagy régi halasztás nem igazolja
az új jelöltet; nincs új publikálási engedély az update beolvadásából.

Minden kör: friss main → minimális változtatás és regresszió → érintett/full automatizált Windows/macOS és sanitizer ellenőrzések → review → zöld PR merge → külön main Actions ellenőrzés. Megőrzendő a pluginazonosság, 148 paraméter ID/sorrend, projektkompatibilitás, Native/Correct viselkedés, 12–120 hangterjedelem, bounded MIDI, állapotvédelem és jóváhagyott GUI. Callbackben nincs új fájl-I/O vagy nem korlátozott munka.

Új jelölt: pontos SHA/verzió, installer és manual payload, upgrade/uninstall, aláírási státusz, hash, megfelelő forrás/dependency manifest, érintett hostteszt vagy külön tételes halasztás, HU/EN dokumentáció és külön publikálási engedély. Nincs régi asset felülírás vagy Yamaha-adat terjesztés. Agent nem telepít plugin/REAPER-t és nem módosít valós DAW-projektet külön engedély nélkül.

## Források és állapotkezelés

[Korábbi összevont terv](../archive/EXECUTION_PLAN_1.0.md), [roadmap](../archive/ROADMAP_1.0.md), [régi kiadási checklist](../archive/RELEASE_CHECKLIST_1.0_RC.md) megőrzött döntés- és bizonyítéktörténet. Az élő feladatokat csak itt frissítjük, egyedi D/T/M azonosítóval. Megvalósításkor ide kerül státusz és PR/tesztlink; a részletes tesztjelentés külön marad. Régi checklist nem írhatja felül ezt a tervet vagy a kiadás tényét.

English summary: this is the sole active post-1.0.1 ledger. The owner explicitly
requires Classic/Clean in the next release (2026-10-09); production DSP/state/UI
integration and its acceptance remain open. Other future features and optional
maintenance are separate. Old release gates are history, not new blockers or
PASS claims. No new version, host-test waiver or publication authority is inferred.
