# D5 Classic / Clean állapot- és kérésátadási szerződés

Specifikációs checkpoint: 2026-10-08, main
`0a8b1bdaa5e99652b4aa2a1958dca04e9ba7f152` (#163 után).
Ág: `test/classic-clean-state-contract`. Irányadó feladatlista:
[D5 az egységes tervben](DEVELOPMENT_PLAN.md).

Kiadási scope döntés, 2026-10-09: a tulajdonos a következő release-t a teljes
Classic/Clean funkcióval együtt kéri. Az irányadó terv új release-scope kapui
érvényesek; az alábbi szerződés/tesztkomponensek önmagukban nem teljesítés.
SETTINGS-integráció, valódi DSP, projektkompatibilitás és új jelölt elfogadása
még szükséges; verziószám és publikálási engedély e döntésből nem következik.

Ez a következő implementáció műszaki szerződésének jelöltje és egy hozzá tartozó
**teszt-only, soros állapotmodell**. Nem szállított SETTINGS kapcsoló, nem új
projektformátum a jelenlegi pluginban, nem thread-safe production megvalósítás.
Az 1.0.1 és a jelenlegi main hangzása változatlan. Csak az eredeti DX7 Mk I
v1.8 támogatott; nincs más firmware-re vonatkozó cél vagy ígéret.

## 1. Meglévő kódhoz illesztés

2026-10-09 admission-részlépés (#170 utáni `2e42412...` baseline): a codec
readerének valódi processor-bekötése már megelőzi az összes restore-mutációt.
Legacy/explicit Classic betölthető; hibás séma és **érvényes Clean** elutasított,
amíg nincs működő Clean renderer/lifecycle. A lentebb leírt valid Clean desired-
mode recall a végleges integráció célja, nem e részlépés mai működése.
Nincs új writer/owner/SETTINGS/audio bekötés; normál save továbbra is legacy,
pending explicit Classic mezőit megőrzi. A Clean negatív kontrollját a hangút
elkészültekor helyes pozitív recall/regresszió váltsa fel, ne pusztán töröljük.
[Processor-admission bizonyíték](../validation/CLASSIC_CLEAN_PROCESSOR_ADMISSION_20261009.md).

- `Source/PluginProcessor.cpp`: `kStateType = VDX7STATE`, `kParameterStateType =
  PARAMETERS`. `getStateInformation()` az engine-mutex alatt leválasztott
  RAM/ROM/bank/paraméter pillanatképet a lockon kívül kódolja XML/binary formába.
  Érvényes `pendingRestore_` esetén megőrzött projektmásolatot ment.
- `setStateInformation()` a root, bankok, RAM és policy ellenőrzése után,
  engine-lock alatt telepíti a pending projektet. A mono-policy konfigurálása
  már mellékhatást okozhat: a D5 admission-reader **e pont elé** került;
  a teljes kívántmód-ownership még külön integrációs feladat.
- Hiányzó/eltérő ROM mellett a saved projekt pending marad; a kompatibilitást
  a meglévő ROM-identity és `savedStateMatchesRom()` szabály igazolja, nem D5.
- `processBlock()` már `try_to_lock`-ot használ, a state epochot a lock megszerzése
  után is ellenőrzi. D5 nem vezethet be új blokkoló audio-lockot.
- `VDX7Engine::render()` a resamplertől kér mintát; `nextNativeSample()` a natív
  tárolót fogyasztja. `generateNative()` CPU-instrukciókat léptet a következő
  EGS-kimenetig, az instrukcióhatári overshoot mintáit megtartja. D5 nem dobhatja
  el ezeket, nem renderelhet előre új nagy tömböt, és nem ronthatja MIDI időzítését.
- A Native/Correct mono-UI út firmware/RAM tranzakciót is végezhet. **Nem**
  másolandó Classic/Clean váltáshoz: D5 nem reboot vagy firmware-policy váltás.

Ezek kódolvasási tények a fenti SHA-n; az alábbi D5 mezők/API-k még javaslatok.

## 2. Mentendő érték és séma

Példányonkénti, wrapper-szintű **kívánt** mód. Belső enum: Classic=0, Clean=1.
Nem SysEx/performance paraméter, nem USER-bankadat, nem globális GUI-preferencia.
Nem bővítjük/átnevezzük a 148 APVTS paramétert; hostautomatizálás nincs engedélyezve.

Javasolt root-metaadatok a meglévő `VDX7STATE` alatt:

| Mező | Írás | Olvasás |
|---|---|---|
| `soundModeVersion` | egész 1 | pontos decimális `1` |
| `soundMode` | egész 0 vagy 1 | pontos `0` / `1` |

A JUCE XML round-trip szöveges propertyket adhat vissza. Production olvasó
csak int/int64/string reprezentációt és teljes, fenti rövid szöveget fogadjon;
ne használjon permisszív prefix/int/bool konverziót. A tesztmodell csak az
előzetesen leválasztott property-szöveget vizsgálja, **nem** a `juce::var` típusát.
XML-ben az eredeti numerikus var típusa már nem állítható vissza biztosan.

- **Mindkét mező hiányzik:** érvényes legacy projekt → Classic, akkor is,
  ha e példány előzőleg Clean-t kért.
- Mindkét mező jelen van és érvényes: a megadott kívánt mód.
- Fél pár, üres/hibás érték, ismeretlen verzió: az **egész új restore-kísérlet
  elutasítása** a meglévő/pending projekt és kívánt mód megváltoztatása nélkül.
  Nem csendes Classic fallback hibás jelenlévő mezőnél. `01`, `+1`, szóköz,
  `1.0`, `1x`, `true`, `Clean` nem megfelelő string reprezentáció.
- Nincs globális state-verzió átnevezése. Régi plugin nem ismeri a Clean-t;
  downgrade esetén nem ígérünk Clean-hangzásmegőrzést. Ezt a szállított funkció
  HU/EN útmutatójába később bele kell írni; ma nincs ilyen szállított funkció.

Mentéskor nem kerülhet a projektbe a rámpa gainje, pillanatnyi aktív mód,
filter/phase history, UI-token, revision, queue vagy félbehagyott request.
Az érvényes mentés mindig a legutolsó **elfogadott kívánt** értéket tartalmazza.
A Mode/Version pár ugyanabban a lockolt pillanatképben rögzítendő, mint RAM/ROM;
utána csak a leválasztott másolatot szabad kódolni. Save nem lépteti vagy
fejezi be idő előtt a rámpát, és nem hív `egs.clean()`-t.

## 3. Tulajdonlás és stale-request védelem

Javasolt egyszerű illesztés az **existing engine-mutex** tranzakcióihoz:

1. Processor-owned `desiredMode` és per-instance `soundModeProjectRevision`
   engine-lock védelemmel. UI pillanatkép a kívánt módot és azonos revisiont
   adja; átmeneti aktív érték nem helyettesíti a kívánt GUI-értéket.
2. SETTINGS művelet mode + **az eredeti snapshot revisionje**. UI oldalon
   `try_to_lock`; foglalt engine esetén BUSY és nincs módváltozás/rejtett queue.
   Revision-eltérés esetén STALE és nincs automatikus újrapróbálás új tokennel.
   A kijelzést frissítjük, új művelethez új pillanatkép kell. Nem fogadunk el
   érvénytelen enumot, és a callbacket sem értesítjük elutasított requestről.
3. Elfogadott UI request csak a skalár kívánt értéket írja; nem állít engine-t,
   bankot, dirty maskot, MIDI ownershipöt vagy rámpát. A host non-parameter-state
   dirty értesítése **lock felengedése után** történjen a reentráns save miatt.
4. Érvényes projekt-install mindig új revision, még azonos payload ismételt
   recalljánál is. Hibás projekt nem változtat revisiont. Új revision nem
   használhat korábbi tokent wrap után; a modell overflownál mutation nélkül
   elutasít. Production kimerülési policy legyen explicit, ne implicit wrap.
5. Az audio callback csak sikeres existing `try_to_lock` után figyeli a **mai**
   kívánt értéket; nincs pre-lock snapshotból később újrapublikált régi request.
   Aktív átmenet kizárólag renderer/engine-owner állapot. Nincs ValueTree, XML,
   fájl/hash, heap vagy firmware-warmup az új hot-pathban. Nincs új lock/CAS loop.
6. Stopped-device boot/prepare/load/restore ugyanilyen tulajdonosi védelemmel
   és dokumentált quiescence-szel kezelhet engine-állapotot. E kör soros modell;
   valódi mutex/thread ordering még külön integrációs tesztet igényel.

Ez nem lock-free UI-mailbox ígéret. Az existing mutex tulajdonlási rendjét
használja, az audio továbbra sem vár. Egy új atomic mailbox csak külön,
együtt atomi mode/generation protokollal és determinisztikus race-tesztekkel
vezethető be; két külön atomic mode/dirty bit nem a fenti rendezés bizonyítéka.

## 4. Hiányzó ROM és pending projekt

Kívánt mód már ROM nélkül állítható/menthető. Érvényes Clean projekt missing
vagy mismatched ROM mellett Clean-ként őrzendő és újramenthető; nem szabad
default Classic engine-állapotból felülírni. Pending módszerkesztés elfogadható
friss tokennel, de nincs Sound-mode dispatch az esetleg még futó, más ROM-hoz
tartozó engine-re. A meglévő kritikus pending-ROM státusz elsőbbsége megmarad.

`getStateInformation()` pending-copy ágán a leválasztott másolatba a captured
legfrissebb kívánt mezőpár kerül. A pending hangszín/bank/ROM-adat változatlan.
`restoreSavedStateLocked()` későbbi RAM-installja nem olvashat vissza újra egy
régi `soundMode` értéket a tree-ből az időközben elfogadott desired helyére.

Kompatibilis restore befejezése csak a **jelenlegi projekt revisionjére**
érvényes, és a meglévő identity-ellenőrzésnek is teljesülnie kell. Régebbi
tranzakció befejezése nem tehet egy új pending projektet engine-readyvé.
Ekkor a legutolsó desired kérhető a renderertől. A tesztmodell külső eredményként
kapja a kompatibilitást: nem végez ROM-felismerést, fájlbetöltést vagy valódi bootot.

## 5. Renderer és lifecycle — még integrálandó

Jelölt a #163 egy-motoros 256+256 natív mintás mute-rampje. Hossza kísérleti,
nem végleges UX döntés; SRC utáni és hallásos elfogadása még nyitott.

- Normál live request nem reseteli a firmware-t, resamplert, native buffert,
  aktív hangokat/sustaint, MIDI-t, voice dirtyt vagy Native/Correct politikát.
- Mód és gain **ugyanahhoz a natív mintához** tartozzon. Az already-generated
  instruction-overshoot vagy SRC history nem dobható el vagy rossz módhoz címkézhető.
  Az EGS módváltás a nulla gainű natív mintához legyen rendezve; a puszta
  `nextNativeSample()` utáni scalar multiply nem oldja meg magában e sorrendet.
- Cold load/kompatibilis project install, még nincs hallható régi jel: a kívánt
  mód első új mintától stabilan, unity gainnel indulhat, boot előtt/után explicit
  újraalkalmazva. Ezt firmware-integrációval bizonyítani kell, nem a modellből.
- Prepare/release/host reset/audio-state reset megtartja a desired módot.
  Ha meglévő reset-gate/mute miatt stabil mód beállítása történik, régi natív/SRC
  history, gate megnyitása és rámpaállapot együtt specifikált/testelt legyen.
  Nem engedélyezünk rejtett plusz rebootot vagy callback-warmupot.
- Bank/program/Init nem módváltó művelet; volume/sustain/dirty/SysEx bájtok és
  148 paraméter sorrend változatlan. A latencia/tail nem változhat új mérés nélkül.

## 6. Következő implementáció és kapuk

1. JUCE property/typed-value/binary codec + ROM-mentes processzor-regresszió:
   legacy/strict/pending mentés, invalid restore teljes snapshot-megőrzés,
   save-during-transition, több példány, old UI-token és old completion race.
2. Processor writer/audio-lock integration; determinisztikus pre/post-lock
   hookokkal valódi state/UI/save versenyek. A soros modell nem helyettesíti ezt.
3. Engine natív mode/gain ordering és lifecycle, privát eredeti v1.8-mal
   Classic null-difference, no-ROM/ROM reload/reset, sustained/tail/mono és sűrű
   váltás. SRC 44.1/48/96 kHz mérések, headroom/CPU/allokáció ellenőrzés.
4. SETTINGS Classic/Clean UI, friss token, BUSY/STALE/invalid visszajelzés,
   billentyűzet/akadálymentesség, kívánt mód kijelzése és pending-status prioritás.
5. Final-head Windows/macOS/ASan/UBSan és review; valódi REAPER/loudness-matched
   hallásos elfogadás vagy külön, tételesen engedélyezett kiadási halasztás.
   HU/EN dokumentáció csak megvalósult funkciót nevezhet szállítottnak.

Rész-PR csak tesztelt, önálló komponenssel is haladhat, de a production restore
vagy SETTINGS nem fogadhat el/jelezhet működő Clean módot, amíg a hozzá tartozó
renderer/pipeline nincs integrálva és megfelelően ellenőrizve. Egy codec-helper
elkészülte nem engedély a DSP nélküli Clean-metaadat szállítására.

[A szerződésmodell reprodukciója és eredménye](../validation/CLASSIC_CLEAN_STATE_CONTRACT_20261008.md).
Ez nem release/tag/asset változtatási engedély vagy teljes D5 elfogadás.

## 7. Megvalósítási checkpoint — JUCE codec, 2026-10-08

A #164 szerződés `328d93992a764dbd1ac89a50feb3ad4e1e2692b3` main-ba került.
Az erre épülő `test/classic-clean-juce-state-codec` ág önálló,
`Source/VDX7SoundModeState.h` helperrel és `Tests/VDX7SoundModeStateTests.cpp`
teszttel ellenőrzi a 2. pont típusait, elutasításait, detached írását és valódi
JUCE XML/AudioProcessor-binary körútját. A teljes idegen payload megőrzési
fixture szintetikus, nem semantikailag teljes érvényes projekt vagy valódi ROM.

`read()` csak a rootot és a D5 párt validálja, siker esetén ad kívánt enumot.
Nem validál RAM/bank/ROM-identitást, és nem installál projektet. A
`writeDetached()` coherens, caller által már captured tree + desired értékből
új deep másolatot készít; rossz root/enum esetén invalid tree-t ad mutation
nélkül. Mindkét művelet nem-audio-threadre való; nem thread-safe snapshot/
mailbox vagy engine-dispatch. A pending tree régi párját a captured desired
felülírhatja, de csak az új másolatban; restore-admission ettől külön művelet.

A helper nincs bekötve production restore/save-be. A 3–5. pont integrációja,
valódi thread/reentráns save és hallásos kapuk nyitva maradnak. XML az eredeti
bool/double típust nem őrzi meg: a helper a ténylegesen dekódolt scalar
reprezentációt ellenőrzi, nem elveszett típusprovenance-t rekonstruál.
[E kör pontos eredményei](../validation/CLASSIC_CLEAN_JUCE_CODEC_20261008.md).

## 8. Önálló zárolásos tulajdonos

Az önálló `Source/VDX7SoundModeOwner.h` egy caller által átadott, a payloadot is
védő `std::mutex` referenciáját használja; nem hoz létre második engine-lockot.
Az entry pointok a lockon kívül hívandók. A payload commit callback lock alatt
fut, nem dobhat kivételt, nem értesíthet hostot és nem léphet vissza az ownerbe.
A host-notification callback viszont lockon kívüli és reentráns lehet.

`installValidated()` az enumot és a revíziókimerülést ellenőrzi: az immutable
teljes projektvalidáció a caller dolga, a mutable engine/ROM készültséget viszont
egy nem dobó, readonly predikátum **a megszerzett lock alatt** frissen ellenőrzi,
a payload commit előtt. Mismatch mellett a valid projekt pendingként települ.
`completePending()` a mai revíziót/pending állapotot saját lock alatt ellenőrzi,
majd ugyanazon lock alatt futtatja a nem dobó kompatibilitási predikátumot.
Mismatch nem commitolhat vagy változtathat snapshotot. A predikátum és a commit
között nincs unlock; plain bool vagy dobó predicate nem elfogadott API-argumentum.
2026-10-09: ez a korábbi pre-lock bool API javítása a #167 review alapján.

A valódi ROM-admissiont továbbra is a caller adja: cache-ben tárolt, mai,
ugyanezzel a mutexszel védett engine/ROM identityt hasonlítson a validált
projekt elvárásához. Korábban számolt mutable bool lambda mögé rejtése sem
helyes integráció; az új negatív kontrollok ezt kimutatják. Nincs hash/fájl-I/O,
host-értesítés, reentry vagy más engine-mutáció a readonly bounded predicate-ben.
A payload commit nem változtathatja meg az admission alapjául szolgáló ROM-
kompatibilitást. Minden identity-writer ugyanazt a mutexet köteles használni.
A későbbi ROM-reload miatti ready invalidation a production lifecycle-adapter
külön feladata, nem e check/commit atomi határának teljes lifecycle bizonyítéka.

A capture ugyanazon
lock alatt mély másolatot készít a payloadról és befogja a kívánt módot; XML/
binary kódolás utána történik. Az audio-visit csak ready, nem-pending állapotban,
egyetlen try-lock után hívja a nem dobó renderer callbacket a mai kívánt móddal.

A komponens nincs bekötve a PluginProcessorba vagy a renderelőbe. A valódi
processor már megszerzett lockjából nem szabad újra meghívni ezeket a lockoló
entry pointokat. Epoch, mono-policy, tényleges ROM-admission és native/SRC mód/
gain ordering integrációja továbbra is külön ellenőrzendő. A szintetikus
payloados, vezérelt valódi szálas teszt nem igazol teljes plugin race-biztonságot,
hangzási módot vagy új kiadás elfogadását.

[Szálas ownership reprodukció és korlátok](../validation/CLASSIC_CLEAN_THREADED_OWNERSHIP_20261008.md).

## 9. Processor project owner részintegráció 2026-10-09

Baseline: `541c39cd8ee6bba0643df52dbfd90716c3a6b268` (#174 után),
`feature/processor-mode-project-state` ág,
[PR #175](https://github.com/RobCZart82/VDX7-JUCE/pull/175). E checkpoint az előző, önálló
komponensre vonatkozó „nincs bekötve” állapotot részben felváltja: az owner
most a valódi processor Classic kívánt módját, projektgenerációját és pending
completion állapotát kezeli. Nem fut mellette második projekt-revízió.

A `*Locked` adapterek a már megszerzett, pontosan az owner engine mutexéhez
tartozó `unique_lock`-ot kérik; nem lockolnak újra. Hibás vagy felengedett token
nem hívhat readiness predicate-et vagy payload commitot. A predicate/commit
nem dobhat és nem értesíthet hostot. A teljes state/mono-policy admission
továbbra is caller feladat; a tényleges ROM-kompatibilitás a processor meglévő
identity-ellenőrzéséből származik, frissen, ugyanazon lock alatt.

Az elfogadott új payload először pending. A megfelelő aktuális ROM mellett
a RAM-install és owner completion együtt történik; loaded, de mismatched
engine nem ready projekt. Nem-pending friss boot/reload readiness frissítése
nem emeli a projektgenerációt, és nem oldhat fel pending state-et.
A save a módot a payload capture-rel együtt rögzíti, majd a lockon kívül kódol.
Még nincs új normál writer: csak már explicit Classic pending pár íródik a
captured desired értékből. Legacy save nem kap új mezőpárt vagy runtime tokent.

Ez nem a végleges Clean recall. Érvényes Clean változatlanul fail-closed;
nincs production request/audio-visit, új UI-kapcsoló vagy DSP. Az összes APVTS/
host-publikáció atomikus restore-ja és readiness-változtató audio-lifecycle út
külön integrációs kapu. A dokumentum 2–6. pontjának Clean/live-request/renderer
szerződése továbbra is a végső funkció követelménye.
[Actual processor ellenőrzések](../validation/PROCESSOR_MODE_PROJECT_OWNER_20261009.md).

## 10. Natív motorátmenet részlépése 2026-10-09

A `feature/engine-sound-mode-transition` ág ([PR #176](https://github.com/RobCZart82/VDX7-JUCE/pull/176)) a #175 utáni
`edb0de367cc6b909af5f7e6f493fa8e0d005c23a` main-ra épül. A valódi
`VDX7Engine::generateNative()` a `VDX7NativeSoundMode` adapterrel lépteti az EGS-t.
Egy minta teljes 6×16-os, 96 órajelű kör; módváltás csak e kör **elején**
történhet, nem az elkészült minta fogyasztásakor vagy egy félkör végén.
A rámpa gainje az elkészülő mintára egyszer kerül rá, még a tárolás/SRC előtt.
Az instrukció összes EGS-órája és overshoot kimenete megmarad. Új kérés nem
változtat már tárolt mintát vagy SRC historyt, és nem reseteli a firmware-t.

Az alapértelmezett Classic unity ága a korábbi EGS-hívást változatlanul végzi.
Átmenetkor a clock csak a scan-határoknál darabolódik. A friss kívánt érték
coalesced scalar; a ramp nem indul újra azonos kéréstől, és visszavont kérés
nem vált régi kívánt módra. Nincs új allokáció, queue, mutex vagy I/O ebben
az adapterben. A core EGS-fázisa CPU boot és audio-reset alatt megmarad,
ezért az adapter fázisát/gainjét sem reseteljük önállóan.

Ez még nem a 5. pont teljes lifecycle megvalósítása: a processor audio-owner
dispatch, cold/project install unity-start, mono-policy/reset/restore közös
integráció, Clean writer/admission és SETTINGS továbbra is nyitott.
A natív mute-ramp nem jelent hallásos vagy kattanásmentességi elfogadást;
Clean alatt az upstream Classic analóg szűrő nem lép, historyja megmarad.
Az 1.0.1 csomag és telepített plugin nem változik.
[Natív motor ellenőrzési jegyzőkönyve](../validation/NATIVE_SOUND_MODE_TRANSITION_20261009.md).

## 11. Meglévő audio-owner alatti kérésátadás — 2026-10-10

A processBlock meglévő try-lockja alatt, a projekt-/host-reset megfigyelés
után a `withAudioOwnerLocked` a jelenlegi kívánt módot adja át a motornak.
Hibás vagy nem birtokolt lock token, pending vagy nem ready állapot nem hívja
a renderert. Nincs második mutex, új queue, XML/hash vagy host notification.
Ez nem nyitja meg a Clean projekt-admissiont vagy UI-t; cold install és
teljes lifecycle még szükséges. A teszt-only Clean owner injekció nem pozitív
Clean project-recall bizonyíték.
[Processor audio-dispatch ellenőrzése](../validation/PROCESSOR_AUDIO_MODE_DISPATCH_20261010.md).
