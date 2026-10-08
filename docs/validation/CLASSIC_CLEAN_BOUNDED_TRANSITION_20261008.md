# D5: korlátozott Classic / Clean váltási prototípus

Dátum: 2026-10-08. Kiinduló main:
`e528fe8b4f8e067dc6877aa5495a23f91a5ac813` (#162 merge után).
Ág: `test/classic-clean-bounded-transition`. Előzmény:
[upstream jelútkarakterizálás](CLASSIC_CLEAN_UPSTREAM_PROTOTYPE_20261008.md).

## Hatókör

Új, **csak tesztben használt** állapotgép:
`Tests/VDX7SoundModeTransitionPrototype.h`; kibővített
`Tests/VDX7SoundModePrototypeTests.cpp`. A normál és ASan/UBSan CI-be már
bekötött `vdx7_sound_mode_prototype` teszt végzi az ellenőrzést. Nincs új
production hangzási mód, SETTINGS elem, hostparaméter vagy projektmező.
E kör nem változtat firmware-en, bankon, hangszínen, MIDI-n, resampleren,
kiadáson, tagen vagy asseten. Nem használ Yamaha ROM-ot vagy hangfelvételt.

Ez egy lehalkításos stratégia mérési jelöltje, **nem véglegesen elfogadott
hangzási megoldás**, nem általános minőségjavítás és nem „click-free” minősítés.
A következő kiadás funkciókörét vagy publikálását nem dönti el.

## Algoritmus és korlátok

- Egyetlen EGS motor fut; nincs két firmware-emulátor vagy DSP crossfade.
- A vezérlő az audio-tulajdonos által használt helyi állapot. A kért és az aktív
  mód külön bool. Az új kérés felülírja a régit; nincs kéréslista vagy várakozó
  queue. Ez **nem** thread-safe GUI/processor mailbox implementáció.
- A gain egész szintből képződik: `level / 256`, `level` 0–256. Ha eltér a
  kívánt és az aktív mód, mintánként egy lépéssel lehalkít; csak a nulla szinten
  vált. A következő mintáktól egy lépéssel visszaerősít. Azonos kérés nem indítja
  újra a rámpát. Korai visszavonás a meglévő gainről erősít vissza váltás nélkül.
- Ha új kérés érkezik visszaerősítés közben, az aktuális gainről fordul meg a
  rámpa. Sűrű ellentétes kérések hosszabban halk/mute állapotban tarthatják a
  kimenetet; **a kérések megszűnése után** a legutolsó érték legfeljebb 512
  natív mintán belül érvényes és teljes gainű. Nincs minden egymást felülíró
  kérés egyenkénti alkalmazására vonatkozó ígéret.
- A két félrámpa 256 + 256 minta. A VDX7 49 096 Hz natív óráján ez kb.
  5.21 + 5.21 ms, összesen 10.43 ms. Ez szándékos rövid hangerőcsökkenés,
  nem azonos hangerőjű vagy észrevehetetlen átmenet ígérete. A prototípus
  közvetlen EGS-clockokat használ, nem host mintavételi frekvenciát.
- A natív EGS mintát szorozza a gainnel, **SRC előtt**. Nulla a módváltás natív
  mintája, de a resampler korábbi mintái miatt a megfelelő host minta nem
  automatikusan nulla. A teljes host-kimeneti átmenet még nincs bizonyítva.
- A Classic filter-history továbbra sem lép Clean alatt. A prototípus nem
  javítja/reseteli ezt, hanem a visszatérést is rövid rámpával fedi. A későbbi
  tranzienst vagy hosszabb filtertörténet-hatást nem zárja ki. A nyers upstream
  headroom nem változik; nincs limiter, normalizálás vagy automatikus gain-match.

A `request()` és `next()` forrása kizárólag fix méretű állapotot/frissítést
tartalmaz; nincs heap-művelet, fájl-I/O vagy lock. A mérési vektort előre
lefoglaljuk, egy EGS mintát 96 clockkal számolunk. Ez kódvizsgálat és tesztadapter,
**nem** teljes plugin-callback allokációs/CPU/határidő- vagy versenyhelyzetmérés.

## Reprodukció és ellenőrzések

```text
cmake --build <build> --config Release --target vdx7_ci_checks
ctest --test-dir <build> -C Release -L rom-free --output-on-failure --no-tests=error
<build>/Release/vdx7_sound_mode_prototype_tests
<build>/Release/vdx7_sound_mode_prototype_tests --unmuted-switch-negative-control
<build>/Release/vdx7_sound_mode_prototype_tests --ignore-clean-negative-control
python -m unittest discover -s Tests -p "test_*.py" -q
python scripts/check_test_registration.py --self-test
```

Az állapotgép tesztje: idle Classic/unity, oda-vissza egy váltás és pontos
némítási időpont, azonos kérés minden mintán sem indítja újra, gain 0–1 és
mintánként legfeljebb 1/256 lépés; 96 minta után visszavont kérés nem vált;
4096 sűrű változó kérés a legutolsó értéket alkalmazza, csak nulla gainen vált,
majd a kérések után mindkét végső mód 512 mintán belül teljes gainre jut.

EGS: 1/16 voice × 9000/14000 pitch-regiszter; ugyanaz a saját szintetikus inger,
mint az előző körben. 2048 Classic bemelegítő minta; 2048 mérési minta, Clean
kérés a 0., Classic a 1024. mintán. Váltási időpontok 255 és 1279; rámpa végén
visszaáll teljes Classic. Váltás nélkül a wrapper bitazonos a folyamatos EGS-sel.
A requestek **azonos abszolút natív mintaoffseten** 1/7/64/511/2048 méretű
szintetikus partícióban azonos teljes trace-et adnak (raw/output/gain/mód/váltás).
Ez nem valódi host-buffer vagy GUI scheduling mátrix. A tiszta állapotgépen
sűrű kérés vizsgált, a teljes EGS trace-ben két jól elkülönített kérés szerepel.

A natív módváltási minta gainje és kimenete pontosan nulla. Az előző minta
gainje legfeljebb 1/256. Az adapter nem erősíti fel a saját nyers EGS mintáját;
a kimenet véges. Mérések upstream float egységekben, SRC és host output gain
nélkül; a ramped oszlopban a prototípus rámpagainje már alkalmazva van:

| Voice / pitch | Clean-re raw → ramped határlépés | Classic-ra raw → ramped határlépés |
|---|---|---|
| 1 / 9000 | 0.000942551 → 0.000346409 | 0.0861538 → 0.0000407696 |
| 1 / 14000 | 0.221377 → 0.0000994306 | 0.0885123 → 0.000760317 |
| 16 / 9000 | 0.0168719 → 0.00554953 | 1.53185 → 0.000652313 |
| 16 / 14000 | 4.46596 → 0.00519996 | 2.39851 → 0.0121651 |

A raw a rámpát megkerülő **ugyanazon** EGS-mintafolyam határlépése; a két oldal
természetes hullámforma-mozgást is tartalmaz. A teszt nem állítja, hogy minden
lehetséges hangszínen kisebb lesz minden mintalépés. A 16 voice / 14000 trace
ramped peak 5.1674; a prototípus továbbra sem teljes host-clipping ellenőrzés.
Pontos platformközi float golden értékeket nem írunk elő.

Negatív kontroll: `--unmuted-switch-negative-control` a tesztadapterben
a váltási mintán megkerüli a nulla gaint. Elvárt **FAIL/exit 1**:
`mode changes must be rendered at zero output`. A korábbi ignore-clean mód
továbbra is az elvárt OPS-difference ellenőrzésnél ad **FAIL/exit 1**.
Ezeket a hibás módokat nem regisztráljuk normál CTestként; nem production hibák.

## Helyi eredmények és következő kapuk

PASS: Windows MSVC Release teljes CI-tesztcél build, 18/18 ROM-mentes CTest,
mindkét negatív kontroll elvárt hibája, tesztleltár önellenőrzés/inventory és diff.
Python: 78 futott, 77 PASS / 1 SKIP (Windows symlink-jogosultság), 0 FAIL/ERROR.
NOT RUN: új PR végső head Windows/macOS/ASan/UBSan CI/review elfogadása (külön
ellenőrizendő), privát v1.8 integráció, sűrű teljes engine váltások, MIDI/sustain/
tail/mono, cold start/ROM reload/reset/prepare, projektállapot és SETTINGS,
allokációs és CPU mérés, resampler/host mátrix és loudness-matched meghallgatás.

### Következő, production előtt megoldandó specifikáció

1. Projektbe a **kívánt** mód mentendő, nem a rámpa átmeneti gainje vagy az
   éppen aktív köztes mód. Legacy mezőhiány → Classic; hibás/idegen érték ne
   kapcsoljon véletlenül Clean-re. GUI a projekt kívánt értékét mutassa.
2. Audio-tulajdonosi kérés és per-instance állapot; nem ValueTree/fájl olvasás
   callbackben. Egy régi kérés nem írhat felül későbbi projekt-recallt.
   Nincs új hostparaméter/automatizálási engedély ebben a prototípusban.
3. Explicit lifecycle: nincs ROM/első render/prepare/reset/reload esetén kívánt
   mód megőrzése, induló gain és filter-history politikája. A Classic alapvonal
   és régi projektek változatlanok; nincs firmware-warmup a módváltó callbackben.
4. Mute-dip hossz/észlelhetőség, gain/headroom és SRC utáni átmenet mérés; szükség
   esetén más bounded stratégia. A 256 lépés **kísérleti érték**, nem végleges
   felhasználói szerződés. A tényleges v1.8 lifecycle/állapot regresszió után
   jöhet minimális production út, majd SETTINGS GUI és host/hallásos elfogadás.

English: experimental single-EGS fade-down/switch/fade-up controller in Tests
only. Deterministic finite native-sample behavior is proven for the synthetic
inputs, not click-free host output or a production-ready Sound mode feature.
