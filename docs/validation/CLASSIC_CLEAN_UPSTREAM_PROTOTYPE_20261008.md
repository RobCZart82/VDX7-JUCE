# D5 Classic / Clean: ROM-mentes upstream mérési prototípus

Dátum: 2026-10-08. Kiinduló main:
`1568c2c112658d715ded26c00c76243e3e25a67e` (#161 után).
Ág: `test/classic-clean-dsp-prototype`.
Retromulator/dx7Lib rögzített commit:
`d5473776a0449d60a997b91bdc888598a33265ac`.

## Hatókör és döntés

Ez a D5 első, teszt-only részlépése: a meglévő upstream `OPS::clean(bool)` és
`EGS::clean(bool)` út feltérképezése és számszerű karakterizálása. A processzor,
engine, SETTINGS, 148 hostparaméter és állapotformátum nem változott.
Nincs szállított Classic/Clean kapcsoló vagy új kiadás. Firmware, Yamaha-bank,
hangfelvétel, installed plugin és REAPER nem kellett ehhez a körhöz.

**A prototípus alapján a nyers bool-kapcsoló önmagában még nem alkalmas a
kattanásmentes váltás elfogadására.** A Clean nem általános minőségjavítás:
a hardvermodellezés bizonyos részeit megkerüli, ez más karaktert eredményezhet.
Hivatalosan továbbra is kizárólag az eredeti DX7 Mk I v1.8 támogatott.

## Mit csinál a rögzített upstream kód?

- `OPS.h`, `ExpTab::invertLogSin`: a Classic a táblázatos nagyság alsó bitjeit
  vágja; 1024/2048/4096 felett 2/4/8 lépésű kvantálással. A Clean ugyanennek
  a táblázatnak a shift előtti egész értékét használja, az előjelet megőrzi.
  Ez nem új lebegőpontos FM motor; a többi táblázat/phase/envelope korlát marad.
- `EGS.h`, `filter`: a Classic 16 voice kimenetét sorban, `16/32768` gainnel
  hajtja át a modellezett `Filter`-en; az utolsó lépés eredményét adja ki.
  A Clean a 16 kimenetet közvetlenül összegzi, egyenként `1/32768` gainnel.
  Ez nem egyetlen végső low-pass lecserélése, és nem két azonos gainű jelút.
- A Clean ág nem lépteti a Classic filter történetét. Visszaváltáskor a régi
  filter-history folytatódik. A bool setter nem reseteli az OPS fázis/feedback
  vagy EGS filter állapotát. A korábbi Clean OPS működés is befolyásolhatja a
  visszatérő kimenetet; a mérés nem izolálja külön ezt és a filter-history hatását.
- EGS továbbra is 96 operator/voice clockonként ad egy mintát. A prototípus
  nem módosítja a VDX7 49 096 Hz belső óráját, firmware-t vagy resamplert.

## Teszt és reprodukció

Forrás: `Tests/VDX7SoundModePrototypeTests.cpp`. CMake cél:
`vdx7_sound_mode_prototype_tests`; CTest: `vdx7_sound_mode_prototype`,
`rom-free;prototype`, 30 s timeout. A normál CI build-kapuba, a ROM-mentes
tesztleltárba és ASan/UBSan kiválasztásába is bekerült. A workflow-regresszió
ellenőrzi a sanitizer build és futtatási kiválasztás összhangját.

1. A 16 384 inverse-log érték mindkét polaritása; a várt Classic kvantálás
   külön osztás/szorzás számítás, nem az upstream `shift()` meghívása.
2. OPS: 32 algoritmus × feedback 0/7 × attenuation 0/128/1023 = 192 eset.
   Saját regiszteradat: pitch 12000 + operatorindex × 37. 256 bemelegítő
   minta után 2048 mérési minta. A default bitpontosan egyezik az explicit
   Classic-kal; a már futó Classic állapoton, óraléptetés nélküli true/false
   körút sem változtatja meg a következő mintákat.
3. EGS: 6 párhuzamos carrier (algorithm 32), saját gyors envelope-regiszterek,
   1/16 aktív voice, fix pitch-regiszter 9000/14000 + operatorindex × 37.
   2048 bemelegítő, majd 8192 mérési minta. Default/explicit Classic egyezés,
   felfűtött állapoton óraléptetés nélküli körút, két külön Clean példány
   determinisztikus egyezése, véges és nem nulla kimenet, módok eltérése.
4. 2048 Classic bemelegítő minta után 512 Classic → 512 Clean → 512 Classic.
   Mért kapcsolási mintalépés, azonos időpontú megszakítás nélküli Classic
   kontroll-lépés és a visszatérés első mintájának eltérése. Egy másik Classic
   példány a saját kontrolljával továbbra is bitazonos marad.

Parancsok (Release konfiguráció szükséges multi-config buildben):

```text
cmake --build <build> --config Release --target vdx7_ci_checks
ctest --test-dir <build> -C Release -L rom-free --output-on-failure --no-tests=error
<build>/Release/vdx7_sound_mode_prototype_tests
<build>/Release/vdx7_sound_mode_prototype_tests --ignore-clean-negative-control
python -m unittest discover -s Tests -p "test_*.py" -q
python scripts/check_test_registration.py --self-test
```

A külön negatív mód szándékosan nem érvényesíti a Clean kérést az OPS
tesztadapterben. Elvárt eredmény **FAIL/exit 1**:
`Clean must change at least one nonzero OPS stimulus`.
Nem normál CTest, nem production mutáció és nem feltételezett termékhiba.

## Helyi Windows MSVC Release eredmények

PASS: teljes `vdx7_ci_checks` build, 18/18 ROM-mentes CTest, inverse-log
karakterizálás, 192 OPS eset, 4 EGS jelút/átmeneti mérés. PASS a negatív kontroll
elvárt hibája, tesztleltár önellenőrzése és a tényleges ROM-mentes inventory.
Python: 78 futott, 77 PASS / 1 SKIP (Windows symlink-jogosultság), 0 FAIL/ERROR.
Az új target helyi futása kb. 1 s; ez futási idő, **nem DSP CPU benchmark**.

Az inverse-log vizsgálat 4992 eltérő bejegyzést talált a két polaritás együtt.
Az OPS 192 esetéből 190 eltért; a legnagyobb nyers integer különbség RMS
kb. 16699. Ez feedbacket/modulációt is tartalmaz; nem zaj- vagy torzításmérték.

Az EGS adatok nyers upstream float-egységek, host output gain/resampler nélkül:

| Voice / pitch regiszter | Classic RMS / peak | Clean RMS / peak | Különbség RMS |
|---|---|---|---|
| 1 / 9000 | 0.0691983 / 0.321430 | 0.0693154 / 0.321594 | 0.00155798 |
| 1 / 14000 | 0.135903 / 0.421595 | 0.140652 / 0.438416 | 0.0849530 |
| 16 / 9000 | 1.10644 / 5.14100 | 1.10905 / 5.14551 | 0.0366716 |
| 16 / 14000 | 2.13706 / 6.72800 | 2.25043 / 7.01465 | 1.98350 |

Egyvoice/9000 esetben Clean → Classic határon a lépés 0.0664225, az ugyanott
folyamatos Classic kontroll lépése 0.00185815, a visszatérés első mintájának
kontrollhoz mért eltérése 0.0663624. Ez számszerű eltérés, nem meghallgatott
kattanásbizonyíték. Magas frekvencián a természetes mintalépés is nagy lehet;
az összes határértéket a teszt kiírja, de nem értékeli hangminőségi PASS-ként.

A 16 szinkron aktív voice saját, szándékosan erős inger mindkét módban 1 feletti
nyers csúcsot ad. Ez headroom-tervezési bemenet, nem bizonyított clipping bug a
kiadott VDX7-ben. Nincs automatikus normalizálás/limiter vagy gain-változás.
Nem használunk platformok között törékeny, pontos float-értékű golden tesztet;
az egy builden belüli páros null-difference és invariánsok a kapuk.

## Nyitott kapuk és következő részlépés

NOT RUN: új PR Windows/macOS/sanitizer CI és review (PR létrehozás után
ellenőrizendő); privát v1.8 teljes engine/processor null-difference; ROM
betöltés/reset/prepare/release módmegőrzés; projektmentés/GUI; spektrum/aliasing,
latencia/CPU és allokációs mérés; hallásos loudness-matched A/B, valós REAPER.
Egy új teszt sanitizer-kiválasztása önmagában nem instrumentált PASS.

Következő D5 kör: bounded átmenet prototípusa és explicit mode/history tulajdonlás
(egy firmware-motor, nem két CPU/emulátor), mért headroom és Classic változatlan
alapvonal. Utána példányonkénti állapot/legacy Classic fallback és audio-határi
kérés specifikációja, privát v1.8 lifecycle/regresszió, majd SETTINGS GUI.
Az új paraméter hostautomatizálása és a kiadási funkciókör külön döntés marad.
Valódi host és meghallgatás nélkül a teljes D5 nem tekinthető elfogadottnak.

English: test-only characterization of the pinned upstream Classic/Clean paths.
No production DSP, UI, parameter, serialization or firmware-support change.
Synthetic differences and raw peak/RMS are not a quality or click-free verdict.
