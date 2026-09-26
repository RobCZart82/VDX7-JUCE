# VDX7 Mk1.

**Hatoperátoros FM szintetizátor · Hardveremuláció**

Nyílt forrású hangszer a VDX7 DX7 Mk I hardveremulációs magjára,
annak hordozható Retromulator dx7Lib adaptációjára és JUCE-ra építve.
Eredeti firmware, hardveres ihletésű felület és közvetlen hangszínszerkesztés.

[English](README.md)

> **1.0.0-dev — fejlesztői előzetes.** A GUI kinézete jóváhagyott;
> a teljes kiadási ellenőrzés még folyamatban van. Saját, jogszerűen
> rendelkezésre álló kompatibilis ROM szükséges. Yamaha firmware és gyári hangadat nincs mellékelve.

![VDX7 Mk1. EDIT — operátorok, burkológörbék és algoritmusábra](docs/screenshots/vdx7-edit.png)

![VDX7 Mk1. PERFORMANCE — játékmód, pitch bend, portamento és vezérlő-hozzárendelések](docs/screenshots/vdx7-performance.png)

*A projekt tulajdonosától kapott valódi 1.0.0-dev képernyőképek.
A kinézetet a helyi VST3 REAPERben és a Standalone alkalmazásban történő
kipróbálása után jóváhagyta. A képeken megmaradt a fejlesztői verziójelzés;
ez nem a stabil kiadás minősítése.*

## Letöltés

Fejlesztői VST3-változatok a [GitHub Actions](https://github.com/RobCZart82/VDX7-JUCE/actions)
oldalról tölthetők le. Válaszd ki a kívánt ág és commit sikeres futását, majd az artifactot:

- **Windows x64:** `VDX7-Windows-x64-VST3`
- **macOS Universal (Apple Silicon + Intel):** `VDX7-macOS-universal-VST3`

Az artifact letöltéséhez GitHub-bejelentkezés szükséges lehet. A beolvasztott
változathoz a legfrissebb sikeres **main** futást válaszd; a pull request
buildje még be nem olvasztott módosításokat is tartalmazhat.
Ezek időszakosan elérhető tesztcsomagok, nem publikált 1.0.0 kiadások.

A [publikált kiadások](https://github.com/RobCZart82/VDX7-JUCE/releases) külön
érhetők el. Standalone és macOS AU forrásból fordítható;
a public workflow-k jelenleg VST3-at terjesztenek.

## Funkciók

- Hat FM operátor, 32 algoritmus és kattintható operátorkapcsolási ábra.
- Négyszakaszos operátorburkolók, ratio/fixed frekvencia, detune és keyboard scaling.
- Globális pitch envelope, feedback, oszcillátorszinkron és LFO.
- PERFORMANCE oldal POLY/MONO móddal, pitch-bend tartománnyal/lépéssel és portamentóval.
- Modulation-, láb-, légzésvezérlő- és aftertouch-hozzárendelések.
- DX7 hangszín-/bank-SysEx import/export és tartós, 32 helyes USER-bank.
- 148 automatizálható hostparaméter és DAW-projektállapot visszaállítása.
- EDIT/PERFORMANCE nézet, képernyő-billentyűzet, pitch/mod kerék,
  kivezérlésmérők és audio-callback terheléskijelző.
- Arányos GUI-méretek: 50%, 75%, 100%, 125% és 150%.

## Rendszerigény

A pluginhoz megfelelő VST3-host szükséges. Fordítási célok: **Windows x64**
és **macOS Universal**. A macOS 11 build-célverzió, nem minden rendszer/host
ellenőrzésének ígérete. A fizikai Intel Mac és Windows hostelfogadás még kiadási
feltétel. A helyben fordított Standalone DAW nélkül fut.

**A megszólaláshoz kompatibilis DX7 Mk I ROM szükséges.**
A formátumokat és az opcionális külső gyári hangadatot a
[firmware-útmutató](GUIDE_HU.md#3-firmware-és-bankok) ismerteti.
A támogatott MIDI-hangtartomány **12–120**, Native és Correct MONO módban egyaránt.

Előre fordított csomaghoz nem kell fordító vagy CMake.
A fejlesztői csomagok nem notarizáltak/Developer ID aláírtak;
az operációs rendszer figyelmeztetései előtt olvasd el az útmutatót.

## Telepítés

1. Zárd be a hostot; mentsd a meglévő plugint, projekteket és módosított bankokat.
2. Bontsd ki az artifactot és az esetleges belső ZIP-et. A teljes
   `VDX7.vst3` csomagot másold a VST3-mappába:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\`
3. Kerestesd újra a plugineket, majd töltsd be a VDX7-et hangszerként.
4. A **LOAD ROM** gombbal add meg saját, jogszerűen használható firmware-edet.
5. Válassz programot vagy tölts be kompatibilis SysExet, engedélyezd a MIDI-monitorozást, és játssz.

Kerüld a kettős plugintelepítést. Ne kapcsold ki a rendszer egészének biztonsági védelmét.
Részletek: [telepítés és első megszólaltatás](GUIDE_HU.md#2-telepítés-és-első-megszólaltatás).

## Dokumentáció

- [Részletes magyar útmutató](GUIDE_HU.md)
- [Detailed English guide](GUIDE_EN.md)
- [1.0 kiadási ellenőrzőlista](ROADMAP_1.0.md)
- [Forrásfüggőségek](SOURCE_DEPENDENCIES.md)
- [Licenc- és komponensközlések](NOTICE.md)

Élő MIDI Out/SysEx-küldés nincs. A görbék alakot szemléltetnek, nem kalibrált időzítést.
A GUI-jóváhagyás és a zöld CI nem helyettesíti a firmware-, offline render-,
automatizálási és platformteszteket. Lásd az [ismert korlátokat](GUIDE_HU.md#9-ismert-korlátok).

## Fordítás forrásból

A [fordítási útmutató](GUIDE_HU.md#11-fordítás-forrásból) tartalmazza a C++20,
CMake 3.22+, platformeszközök, rögzített függőségek és tesztparancsok részleteit.
A public CI ROM-mentes regressziókat futtat; a firmware-tesztekhez jogszerű helyi
ROM szükséges, amely nem kerülhet commitba vagy csomagba.

Hibát a [GitHub Issues](https://github.com/RobCZart82/VDX7-JUCE/issues) oldalon jelezz,
build/commit, rendszer, architektúra, hostverzió, mintavétel/puffer és reprodukciós lépések megadásával.

## Névjegy

![VDX7 Mk1. About — fejlesztői aláírás és komponensközlések](docs/screenshots/vdx7-about.png)

*A projekt tulajdonosától kapott valódi 1.0.0-dev About képernyőkép.*

Fejlesztő: **RobCZart82**. Köszönet:
[chiaccona / VDX7](https://github.com/chiaccona/VDX7),
[Retromulator / dx7Lib](https://github.com/reales/retromulator)
és [JUCE](https://github.com/juce-framework/JUCE).
A VDX7-JUCE nem Dexed-alapú újraimplementáció.

## Licenc

A wrapper és az eredeti GUI-erőforrások licence [GNU AGPL-3.0-only](LICENSE.txt).
A DX7-mag GPL-3.0-or-later, a JUCE AGPLv3 feltételekkel szerepel;
lásd a [közléseket](NOTICE.md). A szoftver garancia nélkül érhető el.

A Yamaha firmware nem része a csomagnak. A Yamaha-név kompatibilitást jelöl;
a projekt nem hivatalos Yamaha-termék, Yamaha-logót nem tartalmaz.
