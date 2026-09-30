# VDX7 Mk1. — Használati útmutató

[Vissza az áttekintéshez](../../README_HU.md) · [English guide](GUIDE_EN.md)

A [publikált 1.0.0 kiadás](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.0)
Windows x64 és macOS Universal VST3 telepítőket/kézi ZIP-eket tartalmaz. A main
már az 1.0.1-et készíti elő; az „1.0.1 fejlesztés” jelölésű részek még kiadatlan
javításokat írnak le. Az Actions artifactok továbbra is fejlesztői csomagok.

## 1. Platform és csomag

### Várakozó projekt védelme (1.0.1 fejlesztés)

Ha a visszatöltött projekt a megfelelő ROM-ra vár, előbb azt töltsd be.
Addig az import/export, USER-mentés, átnevezés, operator másolás/beillesztés,
bank/program választás és performance-módosítás elutasításra kerül akkor is,
ha másik ROM már be van töltve. A live SysEx/program/bank és tartós MIDI
beállítások is blokkoltak; a hangjegyek, felengedés és átmeneti expression
vezérlés megmarad. A host voice/operator szerkesztéseit és a projekt
mentését/újranyitását a védelem megőrzi. A ROM-helyreállítási figyelmeztetés
elsőbbséget kap az általános export-emlékeztetővel szemben.
Ez a javítás a már kiadott 1.0.0 binárisokban még nincs benne.

A CI **macOS Universal (arm64 + x86_64) VST3** és **Windows x64 VST3** célokat
fordít. A kiadásra tervezett formátum a VST3; az AU és Standalone fordítási cél,
nem ígért letöltési csomag. A tulajdonos macOS REAPER- és Windows 10 x64 REAPER-
használatról számolt be, de ezek nem igazolják a végleges RC minden platform-,
mintavételi- és bufferkombinációját. A fizikai Intel Mac elfogadása továbbra
sincs dokumentálva.

**Aláírás:** a Windows VST3 nincs kiadói tanúsítvánnyal aláírva. A macOS
Universal VST3 technikai ad-hoc aláírást kap, de nincs Developer ID-aláírás és
notarizáció. Emiatt figyelmeztetés vagy betöltési akadály jelentkezhet. Ne
kapcsold ki a rendszer egészére vonatkozó biztonsági védelmet. A macOS 11
buildcél, nem minden rendszer/host kombináció tesztelésének ígérete.

## 2. Telepítés és első megszólaltatás

1. Zárd be a DAW-t, és készíts mentést a korábbi VDX7 plug-inről, projektekről
   és módosított USER-bankról.
2. Töltsd le a kívánt VST3 ZIP-et, majd csomagold ki. Ha a ZIP további ZIP-et
   tartalmaz, azt is bontsd ki. A teljes `VDX7.vst3` bundle-t másold a megfelelő
   mappába:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\` (vagy `%COMMONPROGRAMFILES%\VST3\`)
3. Ne hagyj másik VDX7-példányt egy másik plug-inmappában, mert a host a régi
   példányt is betöltheti.
4. Indítsd újra a hostot. REAPERben szükség esetén indíts újrakeresést a
   Preferences → Plug-ins → VST alatt, majd illeszd be a VDX7-et virtuális
   hangszerként.
5. Add meg saját, kompatibilis firmware-edet a LOAD ROM gombbal vagy az alábbi automatikus keresési helyek egyikén.
6. Válassz bankot/programot az LCD-n, vagy importálj megfelelő `.syx` fájlt a
   LOAD SYX gombbal. Engedélyezd a MIDI-monitorozást, majd játssz hangokat.

Ha az operációs rendszer aláírási, kiadói azonosítási vagy notarizációs
figyelmeztetést mutat, ellenőrizd, hogy a csomagot a projekt hivatalos GitHub-
oldaláról szerezted-e be. A figyelmeztetés megszüntetésére ne kapcsold ki a
rendszer védelmét; kövesd az operációs rendszer dokumentált alkalmazás-
jóváhagyási folyamatát.

Ha nincs hang, ellenőrizd a firmware állapotát, a MIDI útvonalát, a sáv monitorozását és az OUTPUT hangerőt. Kerüld a párhuzamos VDX7-példányokat a felhasználói és rendszerszintű pluginmappákban.

## 3. Firmware és bankok

**Yamaha firmware és gyári hangadat nincs mellékelve.** Csak olyan fájlokat használj, amelyek használatára jogosult vagy; a projektmentés nem csomagolja be a firmware-t.

Elfogadott ROM-elrendezések:

- 16 384 bájtos DX7 Mk I firmware, opcionálisan mellette `dx7_factory_voices_32KB.bin`.
- 49 152 bájtos kombinált `dx7.bin`: 16 KB firmware és 32 KB gyári hangadat.

Automatikus keresési mappák:

| Rendszer | Helyek |
| --- | --- |
| macOS | `~/Library/Application Support/VDX7-JUCE/ROM/`, `~/Library/Application Support/discoDSP/Retromulator/ROM/` |
| Windows forrásból fordítva | `%USERPROFILE%\Documents\VDX7-JUCE\ROM\`, `%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\` |

Gyári hangadat esetén ROM1A–ROM4B érhető el: nyolc bank, bankonként 32 program. Enélkül kompatibilis hangszín-/bank-SysEx adat szükséges. A LOAD ROM kézi fájlválasztást is biztosít.

## 4. Hangszínszerkesztés

- Hat operátortab: kimeneti szint, durva/finom hangolás, detune, rate scaling, velocity- és amplitúdómoduláció-érzékenység.
- OSC MODE vízszintes kapcsoló: RATIO vagy FIXED, alatta névleges arány/Hz. A kijelzés nem tartalmazza a detune és moduláció hatását.
- Operátor-envelope: operátoronként négy Rate és négy Level.
- Keyboard scaling: töréspont, bal/jobb mélység és négy görbetípus.
- GLOBAL: négyszakaszos pitch envelope, feedback, oszcillátor-szinkron, transzponálás és LFO-vezérlők. Az 1–32 algoritmus az ábra melletti listából választható.
- LFO: sebesség, késleltetés, pitch/amplitude modulációmélység, szinkron, hat hullámforma és pitch-moduláció-érzékenység.
- A grafikonok a burkológörbe alakját szemléltetik; nem kalibrált idő-/félhangdiagramok.

## 5. Algoritmusábra és játékvezérlők

Mind a 32 algoritmus kapcsolása látható. Operátorcsomópontra kattintva kiválasztod annak szerkesztőjét; a tabok és az ábra kijelölése együtt mozog. A kijelölés nem némít operátort és nem módosít hangot. Az OUT a kimeneti operátorokat jelöli; az F0–F7 a feedback értéke, nem élő jelszint.

A képernyő-billentyűzet, a pitch kerék, a helyzetét megtartó modulation kerék és a hangerőfader működik. A pitch kerék egérhúzás után középre tér vissza; billentyűzetes állításkor szándékosan megtartja a beállított értéket (tulajdonos által jóváhagyott működés). A kerék bordázata az értékkel együtt mozog. A két kivezérlésmérő a kimeneti szintet mutatja; a mag mono jele mindkét csatornára kerül.

A footer CPU-százaléka simított audio-callback terhelésbecslés, nem a teljes számítógép CPU-használata, és nem feltétlenül egyezik a REAPER mérőjével.

## 6. Utility és SysEx

A UTILITY menüben hangszínátnevezés (1–10 nyomtatható ASCII karakter), egyhangszínes export, bankexport és operátormásolás/-beillesztés található. A másolás mind a 21 operátormezőt tartalmazza. Vágólapja a pluginpéldányhoz tartozik, a projekt nem tárolja.

A SAVE AS... alapművelete egy rögzített hangszínmásolat mentése a tartós USER-bank
32 helyének egyikére, felülírási jóváhagyással és ütközésvédelemmel. Az LCD
USER (load copy) választása a USER-bankot a szerkeszthető bankba másolja.
Több elnevezett USER-bank még nincs. Ugyanitt Export Patch (.syx) és Export Bank (.syx)
is választható. A factory ROM változatlan marad. A USER-fájlról is készíts mentést.
A hangszín-SysEx nem tárol teljes projektet vagy globális PERFORMANCE-beállításokat.
A presetléptető nyilak az LCD mellé kerültek, a fejlécből a kettőzött presetkijelzés
eltűnt. A léptetés az aktuális bankon belül körbefordul. Az algoritmus az ábra
melletti 1–32-es listából választható, a korábbi automatizálható paraméterrel.

A LOAD SYX egy teljes DX7-hangszínt (163 bájtos VCED) vagy bankot (4104 bájtos VMEM) fogad. Egy hangszín a kijelölt helyet, egy bank a szerkeszthető bankot cseréli le. Összefűzött dumpok és más hangszertípusok formátumai nem támogatottak. Importkor szerkezet- és checksum-ellenőrzés történik. Az export device/channel nibble értéke 0; az import 0–15 értéket fogad.

## 7. Mentés és automatizálás

148 host-paraméter érhető el: 145 hangparaméter és Master Volume, Pitch, Mod. A korábbi paraméterazonosítók és sorrendjük megmaradtak.

A projekt szerkeszthető RAM-ot, bank-/programállapotot, ROM-útvonalat és vezérlőállapotot tárol. Projektköltöztetés után is legyen elérhető a külső ROM. A programváltás bezárt szerkesztőablaknál is szinkronizálja a paramétereket.

A név melletti csillag nem exportált módosítást jelez. A DAW-projekt mentése és a SysEx-export külön művelet: a projektmentés nem törli az exportjelzést. Az egyhangszínes export egy hangot, a bankexport minden helyet nyugtáz.

Kézi bank-/ROM-/SYX-csere előtt figyelmeztetés jelenik meg a nem exportált módosításokra. MIDI-vezérelt bankváltás nem nyit párbeszédablakot, és lecserélheti a bankot: előtte exportáld a fontos módosításokat. A jelzés nem visszavonási előzmény.

## 8. Felület és méretezés

A végleges 1.0 GUI kinézetét a tulajdonos helyi macOS REAPER VST3-, Standalone-
és Retina-vizuális ellenőrzés után jóváhagyta. A Settings ablakban rögzített
méretek közül választhatsz: 50%, 75%, 100%, 125% és 150%; az ablak sarkát húzva
nem méretezhető szabadon. A Settings a főhangolást, a MIDI-csatornaszűrést és az
advanced MONO-kompatibilitási módot is tartalmazza. Ez a vizuális jóváhagyás nem
teljes platform- és kiadási elfogadás.
Lásd a [képeket](../../README_HU.md) és a [kiadási listát](../release/ROADMAP_1.0.md).

## 9. Ismert korlátok

- Támogatott MIDI-hangterjedelem: Note 12–120 (C0–C9 a REAPER alapértelmezett
  oktávelnevezésével). A tartományon kívüli Note On/Off események mindkét
  SETTINGS-kompatibilitási módban szűrtek; a plugin nem transzponálja őket
  más hangra.

- A PERFORMANCE oldalon már működik a modulációs kerék, lábvezérlő (CC4),
  légzésvezérlő (CC2) és csatorna-aftertouch tartománya (0–99), valamint
  pitch/amplitude/EG-bias hozzárendelése. Ezek globális, DAW-projektben mentett
  beállítások, nem kerülnek a hangszín-/bank-SysEx fájlba, és nem új automatizálható
  hostparaméterek. Kitartott vezérlőértéknél is érvényesülnek a hangfeldolgozás során.
  A meglévő 148 paraméter változatlan marad.
- A PERFORMANCE firmware-alapú POLY/MONO, pitch-bend tartomány/lépés és portamento
  vezérlőket is tartalmaz. A módváltás elengedi a szóló hangokat. A SETTINGS főhangolást
  (-256…+255 firmware-egység, nem cent) és OMNI/1–16 bemeneti csatornaszűrést kínál.
  Ezek a projektben tárolódnak, nem a hangszín-SysExben. A csatornaszűrés wrapperfunkció,
  nem multitimbrális vagy MPE működés.
- Élő MIDI Out/SysEx-küldés nincs; SysEx-fájlimport/-export van.
- A mintavétel-átalakítás Blackman-ablakos sinc szűrést használ, a hostnak jelzett
  késleltetéssel. A tulajdonos szerint az RC1 hangja macOS-en és Windowson is
  kiváló, DX7-hű; a teljes mintavételi/buffer- és transport/render-mátrix nem
  futott le, a tulajdonos döntése alapján halasztva van.
- A hangparaméterek módosítása újratölti az aktív programot. Sűrű automatizálás és tartott hang alatti szerkesztés további hosttesztet igényel.
- A hardveres és más szoftverekkel való SysEx-együttműködés nincs átfogóan ellenőrizve.
- Nem ígér teljes DX7-funkcióazonosságot, kalibrált envelope-időzítést vagy általános hostkompatibilitást.
- A fontos munkáról készíts biztonsági mentést. A fejlesztői csomag nem jelent produkciós stabilitási garanciát.

## 10. Ellenőrzés és hibajelentés

A public CI ROM-mentes regressziókat futtat. A pontos RC1 forrás helyi ROM-os
tesztje 36/36 CTesttel sikeres volt (egy asztali párbeszédablakot nyitó teszt
kimaradt), a tulajdonos pedig célzott macOS/Windows REAPER-elfogadásról számolt
be. A szélesebb host/audio/GUI mátrix nem futott le, tulajdonosi döntés alapján
halasztva van; lásd a [kiadási jegyzéket](../release/RELEASE_CHECKLIST_1.0_RC.md).

Hibát a [GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues) oldalon jelezz
build/commit, operációs rendszer, architektúra, hostverzió, mintavétel/puffer,
lépések és elvárt/tényleges eredmény megadásával. Jogvédett ROM-ot ne tölts fel.

## 11. Fordítás forrásból

Szükséges: C++20 fordító, CMake 3.22+, macOS-en Xcode/Command Line Tools. A függőségek verziója rögzített; a teljes forrás ZIP tartalmazza a JUCE-ot és dx7Lib-et offline fordításhoz. A sima Git checkout letölti ezeket. Lásd: [forrásfüggőségek](SOURCE_DEPENDENCIES.md).

Fordítás a telepített plugin felülírása nélkül:

```sh
cmake -S . -B build-local -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-local --config Release --target VDX7_VST3
```

Kimenet: `build-local/VDX7_artefacts/Release/VST3/VDX7.vst3`.

A kényelmi `scripts/build-macos-arm64.command` script fordít, ad-hoc aláír és szigorúan ellenőrzi a csomagot. Nem telepít és nem cseréli le a telepített VST3-at; a telepítés külön, kézi lépés. Universal- és Windows-segédek: `scripts/build-macos-universal.command`, `scripts/build-windows.bat`. A macOS GitHub Actions workflow Universal artifactot fordít; a sikeres build nem hostellenőrzés.

Tesztek:

```sh
cmake --build build-local --config Release --target vdx7_all_tests
ctest --test-dir build-local -C Release --output-on-failure
```

Az aktuális CMake-beállítás tíz ROM-mentes CTestet regisztrál. A
`vdx7_ci_checks` és `vdx7_all_tests` cél ezen felül a firmware-függő integrációs
futtatókat és az elkülönített MONO-kísérletet is lefordítja, de ROM nélkül nem
futtatja őket. A teljes helyi tesztsorhoz konfigurálj
`-DVDX7_ENABLE_ROM_TESTS=ON -DVDX7_TEST_ROM_FILE=/abszolut/utvonal/dx7.bin`
opciókkal, majd fordítsd újra a `vdx7_all_tests` célt, és indítsd a CTestet.
A ROM-ot ne töltsd fel. A processor teszt nem nyit audioeszközt, és opcionálisan
létező abszolút mappát fogad a PNG-előnézetekhez.

A teljes opt-in tesztkör `VDX7_TEST_ROM_FILE` fájlja 49 152 bájtos, v1.8
firmware-t és gyári hangadatot tartalmazó kombinált kép legyen. A közvetlen
motortesztek nem olvasnak kísérőfájlt, az azonosságtesztek gyári hangadatot is
igényelnek. A közös profilteszt ezt minden függő ROM-teszt előtt ellenőrzi.
A plugin továbbra is támogatja a 16 384 bájtos firmware-t opcionális
`dx7_factory_voices_32KB.bin` mellett; ez nem jelenti ugyanennek a formának
a támogatását a teljes tesztkörben. A privát fixture nem kerül nyilvános CI-be.

## 12. Licenc és kiadási állapot

Ez a projekt [GNU AGPLv3](../../LICENSE.txt) szerint érhető el. A wrapper és az eredeti GUI-erőforrások AGPL-3.0-only licencűek; a DX7-mag megőrzi GPL-3.0-or-later licencét és eredeti közléseit. A JUCE-ot AGPLv3 alatt használjuk. Az egyesített mű és a komponensek közlései: [NOTICE.md](../../NOTICE.md).

Az 1.0.0 kiadási terv a bináris mellett teljes corresponding-source csomagot,
rögzített JUCE- és dx7Lib-forrást, build scripteket és licencközléseket irányoz
elő; ez a csomag még nem tekinthető publikált letöltésnek. A szoftver garancia
nélkül érhető el. A firmware nem része a szoftverlicencnek. A leírásban szereplő
Yamaha-név kompatibilitást jelöl, nem támogatást vagy jóváhagyást; Yamaha-logó
nincs mellékelve.

## 13. Köszönet és következő lépések

Köszönet a [VDX7/chiaccona](https://github.com/chiaccona/VDX7), [Retromulator/dx7Lib](https://github.com/reales/retromulator) és [JUCE](https://github.com/juce-framework/JUCE) fejlesztőinek. Csak a hordozható DX7-mag épül be, nem a teljes Retromulator alkalmazás.

Következő prioritások: teljes helyi ROM-os regressziós kör; célzott REAPER
hangtartomány- és transportteszt; valódi host-, konkurencia- és platformelfogadás;
hátralévő platformközi GUI-ellenőrzések; majd kiadási csomagolás. Az 1.0.0 publikálására
nincs jóváhagyás. Részletek: ROADMAP_1.0.md.
