# Leválasztott ROM-betöltés projektgeneráció-védelme

Dátum: 2026-10-09. Baseline: `53b9589d669dd905cce4f1762421128ca397291a`
(#173 után). Javítási ág: `feature/processor-project-ownership`,
[PR #174](https://github.com/RobCZart82/VDX7-JUCE/pull/174).
Tesztelt kódcommit: `daf8d73c0c2fad7f22adaa2b23f6ed88bcc35a4a`;
a következő commit csak a PR-hivatkozást és e tesztazonosítást rögzíti.
Ez a D5 Classic/Clean valódi projektállapot-integrációjának előfeltétele,
nem a teljes ownership vagy a Clean hangút megvalósítása.

## Reprodukált hiba

A régi projekt ROM-fájljának leválasztott beolvasása közben egy újabb projekt
teljesen visszatöltődhetett ugyanabba a processor-példányba. A régi kérés ezután
újraindította a motort a régi ROM-mal, és felülírhatta az új projekt hangszínét
és firmware-útvonalát. A régi restore a közös pending állapothoz is visszatért.

A teszt két külön, változatlan tartalmú privát original v1.8 firmware-másolatot
használ. Az A projekt feedbackje 2, MIDI-csatornája 3; a B projekté 6 és 9,
eltérő ROM-útvonallal. A teszt-only rendezvous a fájl- és companion-beolvasás
után, közvetlenül a motor lock/admission előtt telepíti B-t. Ezután A folytatódik.
A változatlan production-viselkedéssel, csak a reprodukáló hook hozzáadásával:

```text
FAIL: stale ROM completion must not overwrite the newer project or firmware path
```

Exit 1. A hiba tényleges processor és érvényes firmware mellett reprodukált;
nem pusztán az önálló owner-modell feltételezett problémája.

## Javítás

Az elfogadott projekt példányonkénti, belső `projectRevision_` generációt kap
a meglévő `engineMutex_` alatt. Csak a validált és mono-policy szempontból is
elfogadott restore lépteti; az azonos tartalmú új restore is új generáció.
Mentés vagy elutasított state nem lépteti. A számláló nem fordulhat körbe:
maximumon a következő restore még policy/payload/epoch mutáció előtt elutasított.

A saved-ROM restore az eredeti telepítési generációt adja tovább. A kézi ROM-load
és az automatikus keresés egyszer rögzíti a kérő projekt generációját; az összes
autodetect-jelölt ugyanazt használja. A leválasztott I/O utáni, engine-lock alatti
ellenőrzés stale kérésnél még pending edit capture, firmware-boot, RAM, engine
epoch és metadata-módosítás előtt visszatér. A kézi hívás ilyenkor külön
`superseded` hibát kap. A régi restore pending-completion ága is ellenőrzi a
saját generációját, és nem folytatja az új projekt feldolgozását.

Nincs új hostparaméter, mentett séma vagy módmező; a 148 paraméter ID/sorrend,
GUI, Classic DSP és pluginazonosság változatlan. Az audio callbackben nincs új
lock, allokáció vagy fájl-I/O. A számláló nem hangmód-owner: a későbbi D5
integrációnak ezt a projektgenerációt kell közösen használnia, nem egy második,
független projektazonosítást építenie.

## Regressziók és helyi eredmények

macOS ARM64, JUCE 9.0.3, Release; célzott ASan/UBSan RelWithDebInfo.

- Baseline privát v1.8 negatív kontroll: FAIL, exit 1, a fenti állapotkeveredés.
- Javítás után a privát v1.8 stale-completion mátrix: PASS. Régi teljes restore
  és kézi ROM-load; reentráns és valódi kétszálas változat, összesen négy eset.
  Az új projekt pontos bináris mentése, feedbackje, routingja és ready állapota
  megmarad.
- ROM-free saját 16 KB nullás/érvénytelen firmware-jelölt: PASS. Új pending
  projektet telepítve a stale load nem változtatja annak pontos mentését,
  státuszát vagy generációit; `superseded`, nem firmware-failure eredményt ad.
  Reentráns és kétszálas kontroll is fut a meglévő CI-tesztben.
- Generáció: PASS az azonos restore-ok, példányizoláció, elutasított Clean/hibás
  mód, valamint `uint64_t` maximumon policy/payload megőrzés és no-wrap kontroll.
- Teljes ROM-free CTest: 21/21 PASS; Python: 94/94 PASS, nincs SKIP.
- Tényleges CTest-regisztráció és checker self-test: PASS.
- Diff whitespace és a két módosított dokumentum 24 helyi hivatkozása: PASS.
- `vdx7_ci_checks`, macOS VST3/AU/Standalone development build: PASS.
- Célzott snapshot/admission/generation tesztek: Release és ASan/UBSan PASS,
  firmware nélkül és privát v1.8-cal. A privát célzott sanitizer-kör a négy
  stale-completion esetet is futtatta.
- Privát v1.8 state-transition runner: PASS; első/pending ROM telepítése közbeni
  voice/operator edit és késleltetett routing-publication kontrollokkal.
- Privát pre-ROM state recall és legacy/Classic/Clean admission: PASS.
- Privát legacy 127-es EG / 100-as fine bank import/export és pending/loaded
  projekt-visszatöltés: PASS.

A két szálas teszt promise/future rendezvous-t használ, nem időzítési sleepet;
5 másodperces watchdog védi a CI-t a holtponttól. Ez meghatározott versenyhelyzet
ellenőrzése, nem általános concurrency vagy ThreadSanitizer bizonyíték.
ASan: `detect_leaks=0:halt_on_error=1`; UBSan:
`halt_on_error=1:print_stacktrace=1`. Nem teljes helyi sanitizer mátrix vagy
LeakSanitizer-elfogadás.

A privát firmware 16 384 bájt, SHA256:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Firmware/factory bank vagy annak másolata nem kerül Gitbe, CI-be vagy kiadási
csomagba. A helyi tesztmásolatok izolált, ideiglenes adatok.

## Korlátok és következő kapuk

A teljes privát imported-bank runner a 16 KB-only fixture-rel a meglévő
factory-bank-selection előfeltételnél FAIL-lel megállt; ez nem sikeres teljes
bankkatalógus-elfogadás. A pending-ROM-identity runner szintén FAIL-lel jelezte,
hogy combined firmware+factory-bank fixture szükséges. E két teljes kör nincs
e jelentésben PASS-ként elszámolva. A célzott új stale tesztnek elegendő a
validált original 16 KB firmware, és az sikeresen lefutott.

Ez a javítás a másik projekt telepítése miatt elavult, **ROM-admission előtti**
befejezést védi. Nem teszi egyetlen atomikus tranzakcióvá a teljes APVTS
publikációt, save/restore-t vagy az önálló automation írásokat. Az ugyanazon
projektgeneráción belül párhuzamosan indított két kézi ROM-load sorrendjére
nem vezet be latest-wins szabályt. Az admission utáni összes metadata/host
publikáció versenyhelyzetének lezárását sem állítja.

A kívánt mód/payload/pending/ROM-epoch valódi ownership, renderer/lifecycle és
SETTINGS integráció továbbra is az [egységes terv](../development/DEVELOPMENT_PLAN.md)
nyitott D5 feladata. Clean admission változatlanul fail-closed; nincs működő
Clean-jelzés DSP nélkül. Final-head Windows/macOS/sanitizer CI és review,
majd merge után main Windows/macOS külön kapu. REAPER, hallásos/null-difference
elfogadás, telepítő/source csomag és új release: NOT RUN. A dev-build nincs
telepítve, a publikált 1.0.1 és annak assetei változatlanok.
