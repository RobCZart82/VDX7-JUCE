# VDX7 Mk1. — Használati útmutató

[Vissza az áttekintéshez](../../README_HU.md) · [English guide](GUIDE_EN.md)

A [publikált 1.0.1 kiadás](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1)
Windows x64 EXE és macOS Universal PKG VST3 telepítőket, illetve kézi ZIP-eket tartalmaz.
A megfelelő forrás a [külön forráskiadásban](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source) érhető el.
Az Actions artifactok külön fejlesztői csomagok. AU és Standalone nincs mellékelve.

## 1. Platform és csomag

### Várakozó projekt védelme (1.0.1)

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
2. Használd a Windows EXE vagy macOS PKG telepítőt. A PKG a
   `/Library/Audio/Plug-Ins/VST3/` mappába telepít. Kézi telepítéshez bontsd ki
   a Manual Install ZIP-et, és a teljes `VDX7.vst3` bundle-t másold ide:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\` (vagy `%COMMONPROGRAMFILES%\VST3\`)
3. Ne hagyj másik VDX7-példányt egy másik plug-inmappában, mert a host a régi
   példányt is betöltheti.
4. Indítsd újra a hostot. REAPERben szükség esetén indíts újrakeresést a
   Preferences → Plug-ins → VST alatt, majd illeszd be a VDX7-et virtuális
   hangszerként.
5. Add meg saját, jogszerűen használható eredeti DX7 Mk I v1.8 firmware-edet a LOAD ROM gombbal vagy az alábbi automatikus keresési helyek egyikén.
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

**Az egyetlen hivatalosan támogatott firmware az eredeti Yamaha DX7 Mk I v1.8 (IG11469).**
**Minden más firmware-verzió, köztük a Special Edition / SER-7 és a módosított
firmware-képek nem támogatottak, és a VDX7 jövőbeli kiadásaiban sem lesznek támogatottak.**
Ez a végleges támogatási politika felváltja a korábban halasztott SER7-kompatibilitási
munkát; nem későbbi támogatási ígéret.
A sikeres ROM-betöltés önmagában nem jelent támogatott kompatibilitást;
a betöltő nem alkalmaz kizárólag a v1.8-at engedélyező firmware-listát.

A támogatott eredeti v1.8 firmware azonosításához a 16 384 bájtos firmware-kép
SHA-1 ellenőrzőösszegét hasonlítsd ehhez: `715dbb8e96a4df2a7f096b368334a7654860bb26`.
Ez egyezik a [MAME DX7 ROM-azonosításával](https://github.com/mamedev/mame/blob/master/src/mame/yamaha/ymdx7.cpp#L296-L300).
Az ellenőrzőösszeg a firmware tartalmát azonosítja, nem a letöltés eredetét
vagy a használati jogosultságot. Kombinált képnél ez az azonosító csak az első
16 384 bájtra vonatkozik, nem a teljes firmware- és bankfájlra.
A gyári hangszínbankok külön adatok.

Támogatott elrendezések a v1.8 firmware használatával:

- 16 384 bájtos DX7 Mk I v1.8 firmware, opcionálisan mellette `dx7_factory_voices_32KB.bin`.
- 49 152 bájtos kombinált `dx7.bin`: 16 KB v1.8 firmware és 32 KB gyári hangadat.

Az 1.0.1 mind a 256 gyári hangszín adatait ellenőrzi,
nem csak a fájlméretet. Hibás gyári adatot tartalmazó kombinált ROM esetén
a teljes betöltést elutasítja; a korábban betöltött hangszer-/projektállapot
megmarad. A külön, opcionális 32 KB-os fájlt hiba esetén figyelmeztetéssel
kihagyja, a 16 KB-os firmware betölthető marad. A program nem javítja vagy
korlátozza automatikusan a sérült hangadatokat; használj érvényes bankot.

Frissítési kockázat: az érvénytelen kombinált ROM-hoz kötött régi projekt
az 1.0.1-ben függőben maradhat. Másik vagy kézzel módosított ROM nem jelent
garantált, azonos-ROM-os helyreállítást. Őrizd meg az eredeti ROM, a plug-in
és a projekt mentését; projektmásolaton ellenőrizd a visszatöltést, mielőtt
felülírnád a munkapéldányt.

Automatikus keresési mappák:

| Rendszer | Helyek |
| --- | --- |
| macOS | `~/Library/Application Support/VDX7-JUCE/ROM/`, `~/Library/Application Support/discoDSP/Retromulator/ROM/` |
| Windows forrásból fordítva | `%USERPROFILE%\Documents\VDX7-JUCE\ROM\`, `%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\` |

Gyári hangadat esetén ROM1A–ROM4B érhető el: nyolc bank, bankonként 32 program. Enélkül kompatibilis hangszín-/bank-SysEx adat szükséges. A LOAD ROM kézi fájlválasztást is biztosít.

### Gyári bankmappa

Az 1.0.1 külön, a felhasználótól származó gyári SysEx bankokat
is kezel önálló firmware mellett; nem kell kombinált firmware/bank ROM.
Nyisd meg a **SETTINGS → Bank folder** mappát, és közvetlenül ide másold a `.syx`
fájlokat. Meglévő példányban a SETTINGS **Refresh banks** gombja frissíti a
listát. Új példány a firmware betöltésekor olvassa be a mappát. A fő GUI
elrendezése változatlan.

- macOS: `~/Library/Application Support/VDX7-JUCE/Factory Banks/`
- Windows: `%APPDATA%\VDX7-JUCE\Factory Banks\`

Egy–nyolc bank is használható. Csak a rendelkezésre álló ROM1A–ROM4B helyek
aktívak. A felismerés a teljes, ellenőrzött 4096 bájtos bankadat SHA-256
lenyomatával történik, nem fájlnév vagy hangszínnevek alapján. Azonos fájl
átnevezése nem változtatja meg a helyét. A referencialenyomat ismert dumpot
azonosít; nem igazolja a Yamaha szerzőségét, és nem ad továbbterjesztési jogot.
Hangadat nincs a szoftverhez mellékelve.

Teljes, 4104 bájtos DX7 VMEM banküzenet szükséges helyes szerkezettel,
ellenőrzőösszeggel, 7 bites és támogatott paraméterértékekkel. Ismeretlen vagy
módosított bank nem kap gyári helyet: a **LOAD SYX** CUSTOM bankként töltheti be.
Egyhangszínes fájl, összefűzött üzenet és almappa nem kerül beolvasásra.
Legfeljebb 128 SysEx fájlt vizsgál; limitjelzésnél távolítsd el a duplikátumokat
és felesleges fájlokat. Befejezetlen beolvasás nem cseréli le a meglévő
banklistát. Azonos duplikátum nem ad új bankhelyet. A forrásfájlokat
nem írja át.

A frissítés a mappa és az érvényes régi kombinált ROM/kísérőadat alapján
cseréli a banklistát, de nem cseréli le a megszólaló hangot és nem törli a
nem exportált módosítás jelzését. Bank eltávolításakor a hely inaktívvá válik,
kivéve ha a régi ROM még biztosítja. Ha az aktuális bank listabeli adata eltűnik
vagy megváltozik, a hang RAM-ban marad, CUSTOM jelöléssel. Függőben lévő,
megfelelő firmware-re váró projekt alatt a frissítés tiltott. A SETTINGS
bankmappagombjai nem alkalmazzák a párbeszédablak más, még be nem fejezett
beállításait.

A DAW-projekt az aktuális szerkeszthető bankot, a legutóbbi szerkesztett
hangszínt/nevet, a választott programot és a rendelkezésre álló banklista
saját adatmásolatát is elmenti. Újranyitáskor ez a mentett hang és banklista
áll vissza akkor is, ha a helyi mappa megváltozott vagy eltűnt. Visszatöltéskor
a mappa nem helyettesíti csendben másik hanggal a projektet. Firmware-t nem
ágyaz be; továbbra is kell az eredetivel egyező ROM. Banklistát nem tároló régi
projektnél a korábbi viselkedés marad. Minden példány saját pillanatképet tart;
a többi nyitott példányt külön frissítsd. A projekttárolás nem ad jogot Yamaha
adatok továbbterjesztésére.

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

### Csak az 1.0.1 utáni fejlesztési buildekben

Az UTILITY → Init Preset megerősítést kér, és csak az aktuális szerkeszthető
munkahangszínt cseréli le. A Mégse nem változtatja meg; ha a párbeszédablak
közben változik a hangszín vagy a kiválasztás, új megerősítés kell. A meglévő
szerkesztéseket előbb SAVE AS-szel őrizd meg. A gyári katalógus, a mentett
USER-fájlok, más munkahangszínek és a PERFORMANCE/SETTINGS nem törlődnek.
Az új hangszín exportálásig `Init Preset *` néven látszik; tárolt DX7-neve
`Init Prese` (10 karakter, csillag nélkül). Hangja, szerkesztései, dirty jelzője
és a kijelzés eredete a DAW-projekttel mentődik. Saját VDX7 kezdőhangszín,
nem Yamaha INIT dump, és nincs benne a már publikált 1.0.1 csomagokban.

Importált bankkönyvtár (új fejlesztési buildek, kompatibilis ROM betöltése után): UTILITY → Imported Banks →
Open folder létrehozza/megnyitja a külön bankmappát. Ide tedd a saját,
32-hangszínes `.syx` bankokat, majd válaszd a Refresh műveletet. Windows:
`%APPDATA%\VDX7-JUCE\Imported Banks`; macOS:
`~/Library/Application Support/VDX7-JUCE/Imported Banks`.
Ebben a lépésben explicit frissítés kell, nincs automatikus induláskori scan.
Az LCD Bank listájában Imported Banks csoport, rövidített, számozott nevek
láthatók; a sor és a kiválasztott bank tooltipje a teljes fájlnevet és
tartalmi azonosítót mutatja. Kiválasztáskor CUSTOM munkamásolat készül,
01-es programmal; nem mentett szerkesztéseknél megerősítés kell.
A gyári/USER/forrásfájlok és a beállítások nem íródnak felül. A Refresh csak
a listát módosítja, nem az aktuális hangot. Hibás fájlok és limitek jelentést
adnak; hiányos scan esetén a korábbi könyvtár megmarad. Közben változó listánál
nyisd meg újra a választót. A projekt a kiválasztott bankot és a szerkesztett
hangot eltűnt forrásfájl mellett is őrzi. Ezek nincsenek a publikált 1.0.1-ben;
a valódi host-elfogadás és az induláskori bekötés külön fejlesztési kapu.

A UTILITY menüben hangszínátnevezés (1–10 nyomtatható ASCII karakter) és operátormásolás/-beillesztés található. A SysEx-export a SAVE AS menüben van. A másolás mind a 21 operátormezőt tartalmazza. Vágólapja a pluginpéldányhoz tartozik, a projekt nem tárolja.

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

Egyes archivált bankokban 127-es operátor-burkológörbe sebesség/szint és
100-as finomhangolás szerepel, a szerkesztő normál 0–99-es tartományán túl.
Ezeket a konkrét legacy értékeket az import, a USER-tárolás, a projekt RAM-ja
és a SysEx-export megőrzi; a letöltött fájlokat nem kell átírnod. A szerkesztő
továbbra is 0–99-et használ: puszta megtekintés nem írja át a tárolt bájtot,
az adott paraméter kifejezett szerkesztése viszont a választott normál értékre
cseréli. A többi paraméterhatár, a 7 bites adatok, a méret, a fejléc és az
ellenőrzőösszeg ellenőrzése megmarad. A teljes VMEM-bank exportja a fenntartott
biteket is megőrzi; az egyhangszínes VCED-ben ezeknek nincs külön mezőjük,
de a támogatott legacy paraméterértékek ott is megmaradnak.

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

Az aktuális CMake-beállítás ROM-mentes CTesteket regisztrál. A
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

A v1.0.1 stabil kiadás letölthető a [GitHub Releases oldaláról](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1).
A kiadás macOS Universal `.pkg`-t és kézi ZIP-et, Windows x64 `.exe`-
telepítőt és kézi ZIP-et tartalmaz. Az ellenőrzőösszegek a leírásban, a teljes
megfelelő forrás a [külön forráskiadásban](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source) érhető el.
Kizárólag VST3-at ad; AU- és Standalone-csomag nincs benne. A kiadás
után talált hibák javítása egy későbbi karbantartó kiadásba kerülhet; ez nem
módosítja a publikált v1.0.0 taget vagy fájlokat. A szoftver garancia nélkül
érhető el. A firmware nem része a szoftverlicencnek. A leírásban szereplő
Yamaha-név kompatibilitást jelöl, nem támogatást vagy jóváhagyást; Yamaha-logó
nincs mellékelve.

## 13. Köszönet és következő lépések

Köszönet a [VDX7/chiaccona](https://github.com/chiaccona/VDX7), [Retromulator/dx7Lib](https://github.com/reales/retromulator) és [JUCE](https://github.com/juce-framework/JUCE) fejlesztőinek. Csak a hordozható DX7-mag épül be, nem a teljes Retromulator alkalmazás.

A publikált 1.0.1 dokumentált automatizált/csomagellenőrzései és körülhatárolt
tulajdonosi tesztjei sikeresek. Az [elfogadott kézi teszthalasztások](../validation/VALIDATION_20261005_PUBLICATION_101.md)
nem PASS eredmények. A jövőbeli karbantartás nem változtatja meg a kiadott fájlokat.
