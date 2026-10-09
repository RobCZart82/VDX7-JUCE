# M5 JUCE 9.0.3 kompatibilitási kísérlet

Dátum: 2026-10-09. Baseline main: `bc8c1664ad13a11208773d5b227cb113033555ec`.
Ág: `maintenance/juce-903-compatibility`. Státusz: FEJLESZTÉS / ELLENŐRZÉS ALATT.
Nem kiadási vagy publikálási elfogadás; az 1.0.1 korábbi assetjei változatlanok.

## Előfeltétel és változtatás

#168 a jelen kör indulásakor már beolvadt. Végleges head `ccbc64d6d4657039dfc1deff630eb9f59415bcb9`:
Windows `37892234835`, macOS `37892235031`, ASan/UBSan `37892234833` PASS,
nincs nyitott review-szál. Merge utáni Windows `37896599208`, macOS `37896599203`
PASS. Az M5 provenance előfeltétele teljesült.

Régi JUCE: `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` (9.0.1).
Jelölt: `be29c81492b6151c8ea8d14c840e1311963b3a83` (9.0.3).
Mindkét valódi Git-checkout tiszta. A CMake és a source packager creation pin
együtt változik; az aktuális dependency útmutató és THIRD_PARTY is ezt követi.
Retromulator változatlan `d5473776a0449d60a997b91bdc888598a33265ac`.
Nincs production Source-, DSP-, pluginazonosító-, parameter- vagy SETTINGS-változás.
Dev identitás és a kizárólagos original v1.8 támogatás megmarad.

Indokolt kompatibilitási kísérlet: standalone CoreAudio-eszközkezelés,
Windows ablak/fókusz javítások. A saját Source/Tests a megváltozott ThreadPool,
eltávolított MIDI-selector vagy pontosított OS-architektúra API-t nem használja.
Ez nem teljes wrapper/host QA. Az új codec/OpenGL/VST3 hosting funkció nem
automatikus jobb FM-hangzás vagy VDX7-pluginjavítás. A MP3 alapértelmezés és új
formátumok build/footprint változást okozhatnak, licenceik a teljes upstream
corresponding-source-ban megmaradnak.

## Reprodukált packaging regresszió és javítás

A pin egyszerű cseréjének szimulációjánál a régi 9.0.1 source-archive új
verifierrel történő ellenőrzése FAIL (`Invalid corresponding-source package identity`).
Ez megszakítaná a már fagyasztott 1.0.1 product/packager páros archívumának
ellenőrzését az új approval toolingban.

Csak a **verifikáció** fogadja el a jelenlegi és az explicit történeti 9.0.1
JUCE/core tuple-t. Új **creation** továbbra is kizárólag az aktuális pint
kéri. Ismeretlen JUCE/core SHA és új toolból történeti pinű creation negatív
kontrollként elutasított. Inventory/hash/mode/approval/source/packager védelem
nem kerül ki. Archívumintegritás nem firmware-támogatás, upstream hitelesítés
vagy új publikálási engedély. A committed release approval JSON változatlan.

## Privát processor-összehasonlítás

Új opt-in mód: `vdx7_processor_tests --compatibility-fingerprint <v1.8.bin> <fresh-private-dir>`.
Nem CTest-regisztráció és nem public workflow parancs. Explicit firmware-only
v1.8 precondition; firmware nem másolódik az outputba. Saját VDX7 Init seedből
készült szintetikus katalógus és üres, izolált bankmappák; nincs factory-bank dump,
GUI vagy audioeszköz megnyitás. A PCM/state fingerprint privát, repo-n kívüli
mappában marad, sem commitba, sem CI artifactba nem kerül.

30 eset: 44.1/48/96 kHz × 64/128/256/512/1024 buffer × Native/Correct mono-policy.
Minden esetben MONO hang, sustain, sub-block MIDI, host reset és új hang,
save/recall, firmware nélküli pending, utána edit és matching load, valamint
direct firmware reload. A firmware-only reload nem tartalmaz hangszíneket:
az új hang előtt explicit újratelepítjük/kiválasztjuk a saját seedet; ez nem
factory-preset megőrzési állítás. Pending fixture-ben csak a `romPath` üres,
az identity megmarad; nem hagyjuk az automatikus saved-path reloadot véletlenül
kiváltani a pending próbát. Minden aktív kontroll nem néma és minden minta véges.

Kimenet: stereo-interleaved little-endian float32 és JSON leltár, PCM SHA256,
mentett/recalled project binary SHA256, peak/RMS/silent-block szám. A comparator
**közvetlenül minden mintabájtot** összevet, első eltérő frame/channel és maximum
abszolút eltérés; state/metric-only eltérés is FAIL. Hiányzó/duplikált/ismeretlen
eset, rossz hash és nem véges minta elutasított. Négy ROM-mentes Python-kontroll
apró, saját float fixture-t használ, nem privát hangot.

Az első harness-próbák hibás fixture-feltételek miatt FAIL-ek voltak (kiválasztás
nélküli seed, saved-path autoload, firmware-only reload utáni seed hiánya), és a
private profile API közvetlen hívása fordítási hibát okozott. Ezeket a meglévő
friend teszt-hozzáféréssel és explicit fixture-lépésekkel javítottuk, a küszöböt
nem lazítottuk. Nem állítjuk ezeket bizonyított VDX7 production regressziónak.

Összevethető tesztkód commit: `00a477b446a47707f6a97298bf35a1c4c1ae1c18`.
Ez külön, tiszta 9.0.1 kontrollforrásban rögzített; a jelölt ugyanazt a processor/
engine/harness kódot használja, külön source/build/checkouttal. Nem állítunk
bitazonos EXE-t (útvonal/toolchain metadata eltérhet); csak a tesztelt outputokat.

## Eredmények és megállási kapuk

- PASS: korábbi 9.0.1 build két ismétlése, 30/30 eset, 5 760 512 sample value;
  eltérő minta 0, maximum difference 0, első eltérés nincs, state/recall/metric
  eltérés nincs. Ez determinisztikussági kontroll, nem az új JUCE elfogadása.
- PASS: 9.0.3 configure, tényleges resolved source record, tiszta Git SHA guard.
- PASS: a korábbi archive-repro javítás utáni célzott packaging suite (16/16).
- PASS: tiszta 9.0.1 processor-kontrollbuild; a korábbi builddel 30/30 esetben
  ugyanaz a PCM/state/recall/metric eredmény. Tiszta 9.0.3 Windows x64 build:
  `vdx7_ci_checks`, `VDX7_VST3`, `VDX7_Standalone`, MSVC 19.44 / VS 2022.
- PASS: tiszta 9.0.1 → 9.0.3 összevetés, 30/30 eset, 5 760 512 sample value;
  eltérő mintabájt 0, maximum abszolút eltérés 0, első eltérés nincs,
  binary state/recall és peak/RMS/silent-block eltérés nincs.
- PASS: új 9.0.3 ROM-mentes CTest 20/20, köztük GUI/state/mono/prototype;
  tulajdonosi konkurenciateszt 50/50 ismétlés, tényleges tesztregisztráció
  és checker pozitív/negatív önellenőrzés. Nem privát teljes CTest-suite.
- PASS: teljes Python suite 93 tesztből 92 PASS, 1 SKIP (Windows symlink
  jogosultság), 0 FAIL/ERROR. A CMake-konfigurációs integrációteszt lefutott.
- NOT RUN még: macOS universal és ASan/UBSan final-head CI/review,
  teljes valódi source-archive/offline build; következő checkpointban vezetendő.
- NOT RUN: új binary REAPER VST3/AU/standalone smoke, fizikai MIDI/Intel Mac,
  hallásos és tényleges offline DAW-render elfogadás. Ez nem T1–T4 lezárás.

Merge előtt: friss final-head Windows/macOS/sanitizer és rendezett review,
automatizált compatibility kapuk. Eltérésnél STOP, diagnózis; nincs automatikus
tolerancia lazítás. A hiányzó valódi host QA továbbra kiadási kapu, nem örökölt
1.0.1 PASS. Publikálás külön engedélyt igényel. Visszaút: szokásos revert PR
a pin/creation/docs változtatással együtt, nem force push/tag/asset mozgatás.

Forrás: [9.0.3 kiadás](https://github.com/juce-framework/JUCE/releases/tag/9.0.3),
[changelog](https://github.com/juce-framework/JUCE/blob/be29c81492b6151c8ea8d14c840e1311963b3a83/CHANGE_LIST.md),
[breaking changes](https://github.com/juce-framework/JUCE/blob/be29c81492b6151c8ea8d14c840e1311963b3a83/BREAKING_CHANGES.md).
