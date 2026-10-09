# D5 Classic / Clean állapot- és kérésátadási szerződés

Specifikációs checkpoint: 2026-10-08, main
`0a8b1bdaa5e99652b4aa2a1958dca04e9ba7f152` (#163 után).
Ág: `test/classic-clean-state-contract`. Irányadó feladatlista:
[D5 az egységes tervben](DEVELOPMENT_PLAN.md).

Ez a következő implementáció műszaki szerződésének jelöltje és egy hozzá tartozó
**teszt-only, soros állapotmodell**. Nem szállított SETTINGS kapcsoló, nem új
projektformátum a jelenlegi pluginban, nem thread-safe production megvalósítás.
Az 1.0.1 és a jelenlegi main hangzása változatlan. Csak az eredeti DX7 Mk I
v1.8 támogatott; nincs más firmware-re vonatkozó cél vagy ígéret.

## 1. Meglévő kódhoz illesztés

- `Source/PluginProcessor.cpp`: `kStateType = VDX7STATE`, `kParameterStateType =
  PARAMETERS`. `getStateInformation()` az engine-mutex alatt leválasztott
  RAM/ROM/bank/paraméter pillanatképet a lockon kívül kódolja XML/binary formába.
  Érvényes `pendingRestore_` esetén megőrzött projektmásolatot ment.
- `setStateInformation()` a root, bankok, RAM és policy ellenőrzése után,
  engine-lock alatt telepíti a pending projektet. A mono-policy konfigurálása
  már mellékhatást okozhat: a D5 validációja **e pont elé** kell kerüljön.
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
