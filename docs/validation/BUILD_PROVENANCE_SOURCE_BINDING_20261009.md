# M5 előkészítés: a build és a dependency-forrás összekötése

Dátum: 2026-10-09. Baseline: `e4000390188618d63acce1301a1d13a6616ae9ed` main.
Ág: `fix/build-provenance-source-binding`. Ez nem JUCE-verziócsere,
hangmotor-módosítás vagy kiadási elfogadás.
Pull request: [#168](https://github.com/RobCZart82/VDX7-JUCE/pull/168).
Implementációs commit: `3aa0932`; final-head CI/review még szükséges.

## Előző mérföldkő lezárása

#167 már beolvadt; a merge-t a jelen kör indulásakor készen találtuk.
Végleges head: `4239959d18dfcbf60ee339f723c10e70647ab830`.
Windows `37886851289`, macOS `37886851290`, ASan/UBSan `37886851376`: PASS.
A ROM-admission review-szál rendezett. Merge utáni main Windows `37888604500`
és macOS `37888604585`: PASS. Ez az önálló D5 owner elfogadása, nem a
PluginProcessor/engine/SRC/SETTINGS integráció vagy valódi hostteszt lezárása.

## Reprodukált hiányosság

A `write_build_provenance.py` eddig ellenőrizte a caller által megadott JUCE és
Retromulator Git-másolat pinjét és tracked tisztaságát, de nem vetette össze
ezeket a CMake által ténylegesen kiválasztott forrásmappákkal. A wrapper
CMakeLists is származhatott a buildtől eltérő checkoutból. Emiatt egy helyes
pinű ellenőrzési másolat mellett másik forrásból konfigurált buildhez is
kiállíthatott volna helyesnek látszó dependency-adatot.

Ez bizonyított ellenőrzési rés, nem bizonyított hiba a publikált 1.0.1 binárisban.
Három új teszt először az eredeti script ellen futott: mindhárom FAIL
(`ValueError not raised`), míg a nyolc korábbi kontroll PASS maradt.
Külön hibás JUCE, core és wrapper forrásválasztás a reprodukció.

## Javítás és régi termékforrások

- A CMake a configure végén `VDX7DependencySources.cmake` fájlt generál a
  feloldott wrapper, JUCE és `VDX7_CORE_DIR` útvonalával. Nem a letöltési pinből
  vagy az alapértelmezett `_deps` mappanévből következtet a használt forrásra.
- A script csak literális mezőket olvas, nem hajt végre CMake-kódot. A cache
  `CMAKE_HOME_DIRECTORY`, a generált wrapper és mindkét dependency útja egyezzen
  a megadott fákkal; ezt követik a meglévő SHA/tisztaság/toolchain ellenőrzések.
  Hiányos/relatív/eltérő adat STOP; sikertelen ellenőrzés nem írja felül a régi
  jelentést. A konfigurációs fájl nem kerül a nyilvános BUILD-INFO szövegbe.
- A packaging workflow mindkét platformon a **pontos approval/workflow
  checkoutból** adja át a CMake hookot. A korábbi, fagyasztott 1.0.1 packager
  még nem tartalmazza ezt a fájlt, ezért nem abból kérjük. Régi termékforrásnál
  is lehet új megfigyelési rekordot írni annak módosítása nélkül. A script
  továbbra is a rögzített packagerből fut; a régi script nem kap utólag új
  ellenőrzési képességet. Az új guard elfogadását nem állítjuk régi csomagokra.
- A hook és a mai termék include-ja együtt is csak egy rekordot generál.
  A teszt tényleges CMake configure-t futtat régi/mai project-stílus és
  vendored/FetchContent útvonalválasztás négy szintetikus kombinációjával.

A rekord **konfigurációs megfigyelés**, nem aláírás, nem bizonyíték arra, hogy
minden régi objektum újrafordult, és nem védi az utólag kézzel meghamisított
buildmappát. A dependency-cserénél továbbra is tiszta, külön build kell.
A helper önmagában nem hitelesíti a vendored forrás fájlhasheit; Git checkout
szükséges a jelen packaging provenance CLI-hez, offline archívumnál a megfelelő
manifest/hash ellenőrzés külön kötelező.

## Helyi eredmények

| Ellenőrzés | Eredmény |
|---|---|
| Három reprodukció a javítás előtt | FAIL, elvárt hibabizonyíték |
| Célzott provenance suite a javítás után | PASS: 15/15 |
| Tényleges CMake hook négy szintetikus kombinációja | PASS, a fenti suite része |
| Valódi MSVC configure tiszta, pinnek megfelelő JUCE/core Git-másolattal | PASS |
| E configure útvonalai + meglévő Git SHA/tisztaság guard | PASS |
| 9.0.1-re konfigurált build mellé másik, 9.0.3-as JUCE-másolat megadása | PASS: elutasítás a forrásútvonalon |
| Windows MSVC Release `vdx7_ci_checks`, meglévő build újrakonfigurálásával | PASS |
| Teljes ROM-mentes CTest | PASS: 20/20 |
| Python teljes suite | 85 futott: 84 PASS / 1 Windows symlink-jogosultsági SKIP / 0 FAIL/ERROR |
| Registration checker self-test | PASS |
| Tényleges ROM-free inventory, 20 helyi dokumentumlink és diff-ellenőrzés | PASS |
| Új PR Windows/macOS/sanitizer és review | NOT RUN / megnyitás után szükséges |
| Installer workflow tényleges dispatch, új csomag és régi approval visszajátszás | NOT RUN |
| JUCE 9.0.3 pluginbuild, Classic audio-összevetés, privát ROM és REAPER | NOT RUN |

A helyi C++ suite a korábbi, manifestes corresponding-source JUCE 9.0.1
mappával futott; ez **nem** 9.0.3-teszt vagy Git-provenance CLI elfogadás.
A külön friss konfiguráció valódi tiszta 9.0.1 Git-checkoutot használt; ott
nincs teljes pluginbuild állítás. A MSBuild meglévő környezeti
`CMAKE_GENERATOR_PLATFORM` figyelmeztetése nem teszt-FAIL; a konfigurált
generátor továbbra is Visual Studio 2022 x64.

## JUCE-frissítés elővizsgálat és következő kapuk

A 9.0.3 upstream tag ellenőrzött teljes SHA-ja:
`be29c81492b6151c8ea8d14c840e1311963b3a83`. Külön tiszta lokális checkout
rendelkezésre áll, de a mai CMake és packager pin változatlan:
`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` (9.0.1).

**GO a külön kompatibilitási kísérlethez; még nincs GO pin-merge-re vagy kiadásra.**
A CoreAudio-eszközkezelési és Windows ablak/fókusz javítások relevánsak lehetnek
a JUCE standalone/wrapper útján. A saját Source/Tests nem hívja a megváltozott
`addJob`, eltávolított `getMidiInputSelectorListBox` vagy pontosított
`isOperatingSystem64Bit` API-t. Ez keresési eredmény, nem teljes API/build QA.
OpenGL, VST3 hosting és új codec-funkció nem indok automatikus VDX7 hangjavulási
állításra. A saját FM-motor/SRC változatlan marad.

Következő külön ág: azonos VDX7-kódon tiszta régi/új build, pin és packager SHA
együttes frissítése, aktuális dependency útmutató, Windows/macOS universal és
sanitizer, állapot/GUI/csomag kontrollok. Privát v1.8 Classic output-összevetés
és új bináris valódi hostpróbája a terv szerint marad; hiányzó REAPER NOT RUN,
nem örökölt PASS. A jelen provenance PR review/CI kapuit előbb zárjuk le.

Források: [JUCE 9.0.3 kiadás](https://github.com/juce-framework/JUCE/releases/tag/9.0.3),
[változáslista](https://github.com/juce-framework/JUCE/blob/be29c81492b6151c8ea8d14c840e1311963b3a83/CHANGE_LIST.md),
[breaking changes](https://github.com/juce-framework/JUCE/blob/be29c81492b6151c8ea8d14c840e1311963b3a83/BREAKING_CHANGES.md).
Sem firmware, sem privát audio nincs a repositoryban/artifactban. A kizárólagos
original v1.8 támogatási döntés, release approval, tagek és assetek változatlanok.
