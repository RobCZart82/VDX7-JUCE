# VDX7-JUCE prototype – macOS VST3/AU wrapper

**Állapot:** első fordítható prototípus / fejlesztői build.  
**Cél:** a VDX7 Yamaha DX7 Mk I hardveremulációs mag használata önálló, egyszerű JUCE VST3/AU hangszerként.

Ez a projekt nem Dexed-alapú FM újraimplementáció. A hangmotor a VDX7-ből származó, Retromulatorban karbantartott `dx7Lib` portot használja: HD6303R CPU + DX7 EGS/OPS emuláció, az eredeti Yamaha firmware-rel.

## Mit tud az első prototípus?

- VST3 instrument macOS-re, plusz AU és Standalone build.
- Apple Silicon (`arm64`) és Universal (`arm64 + x86_64`) build script.
- Windows VST3 build script is mellékelve későbbi teszthez.
- Yamaha DX7 Mk I firmware betöltés külső fájlból.
- Elfogad:
  - **16 384 byte-os** firmware-only fájlt (`DX7-V1-8.OBJ` vagy `dx7.bin`), és opcionálisan mellette `dx7_factory_voices_32KB.bin` fájlt;
  - **49 152 byte-os** kombinált `dx7.bin`-t: 16 KB firmware + 32 KB factory voice adat.
- Automatikus ROM-keresés macOS-en és Windowson.
- MIDI Note On/Off, velocity, Program Change, Mod Wheel, Breath, Foot, Data Entry, Sustain, Portamento, Aftertouch, Pitch Bend.
- 8 factory bank × 32 program, ha a 32 KB factory voice adat rendelkezésre áll.
- Standard Yamaha DX7 **4104 byte-os, 32-voice `.syx` bank** betöltés.
- REAPER projekt-state mentés: RAM + bank/program + ROM útvonal.
- A DX7 core natív kb. **49.096 kHz** sebességen fut; a wrapper a host sample rate-re konvertálja.
- Minimalista, szándékosan egyszerű GUI: ROM, SYX, bank, program, patch-név.

## Fontos: Yamaha ROM nincs a forráscsomagban

A repository **nem tartalmaz Yamaha firmware-t vagy gyári ROM-adatot**. Használd a saját, jogszerűen rendelkezésedre álló DX7 ROM dumpodat.

A nálad korábban ellenőrzött DX7 Mk I v1.8 firmware jellemzői:

- méret: 16 384 byte
- CRC32: `6cbb0865`
- SHA-1: `715dbb8e96a4df2a7f096b368334a7654860bb26`

## ROM elhelyezése macOS-en

Ajánlott saját mappa:

```text
~/Library/Application Support/VDX7-JUCE/ROM/
```

Ide teheted például:

```text
~/Library/Application Support/VDX7-JUCE/ROM/dx7.bin
```

vagy:

```text
~/Library/Application Support/VDX7-JUCE/ROM/DX7-V1-8.OBJ
~/Library/Application Support/VDX7-JUCE/ROM/dx7_factory_voices_32KB.bin
```

A plugin a meglévő Retromulator mappát is automatikusan ellenőrzi:

```text
~/Library/Application Support/discoDSP/Retromulator/ROM/dx7.bin
~/Library/Application Support/discoDSP/Retromulator/ROM/DX7-V1-8.OBJ
```

Ha egyik helyen sem talál ROM-ot, a pluginban a **Load ROM...** gombbal manuálisan kiválaszthatod.

## ROM elhelyezése Windowson

Ajánlott saját mappa:

```text
%USERPROFILE%\Documents\VDX7-JUCE\ROM\
```

A Retromulator meglévő helyét is ellenőrzi:

```text
%USERPROFILE%\Documents\discoDSP\Retromulator\ROM\
```

## macOS ARM64 VST3 fordítása

### Követelmények

- macOS 11 vagy újabb ajánlott
- Xcode + Command Line Tools
- CMake
- internetkapcsolat az első buildnél, mert a CMake letölti:
  - JUCE 9.0.1
  - Retromulator `dx7Lib` forrást

Terminálból:

```bash
cd VDX7-JUCE-prototype
chmod +x scripts/build-macos-arm64.command
./scripts/build-macos-arm64.command
```

A script:

1. létrehozza az Xcode buildet;
2. lefordítja a `VDX7_VST3` targetet;
3. ad-hoc aláírja a helyi tesztbuildet;
4. bemásolja ide:

```text
~/Library/Audio/Plug-Ins/VST3/VDX7.vst3
```

Ezután REAPER:

```text
Preferences > Plug-ins > VST > Re-scan
```

Majd keress rá: **VDX7**.

## Universal macOS build

```bash
./scripts/build-macos-universal.command
```

Ez `arm64 + x86_64` VST3 bundle-t készít.

## GitHub Actions – Mac nélkül is lefordítható

A repository tartalmazza:

```text
.github/workflows/build-macos.yml
```

Ha a teljes projektet feltöltöd egy GitHub repositoryba:

1. nyisd meg az **Actions** lapot;
2. válaszd a **Build macOS VST3** workflow-t;
3. `Run workflow`;
4. a build végén töltsd le a **VDX7-macOS-universal-VST3** artifactot.

Ez a workflow macOS runneren készíti el az Universal VST3-at.

## Windows build

Visual Studio 2022 + CMake mellett:

```bat
scripts\build-windows.bat
```

A VST3 várható helye:

```text
build-windows\VDX7_artefacts\Release\VST3\VDX7.vst3
```

## `.syx` bank betöltése

A pluginban kattints:

```text
Load .SYX...
```

Az első prototípus 32-voice Yamaha DX7 bulk dumpot vár:

- 4104 byte
- `F0 43 nn 09 20 00 ... checksum F7`
- az `nn` MIDI device/channel nibble lehet 0–15.

A voice adat közvetlenül a DX7 belső 32-programos voice RAM-jába kerül, majd az aktuális program újratöltődik.

## Factory bankok

Ha a 48 KB-os kombinált `dx7.bin`-t használod, vagy a 16 KB firmware mellett megtalálható a `dx7_factory_voices_32KB.bin`, a GUI bankválasztója nyolc bankot kínál:

```text
ROM1A
ROM1B
ROM2A
ROM2B
ROM3A
ROM3B
ROM4A
ROM4B
```

## REAPER használat

1. Insert virtual instrument on new track.
2. Válaszd `VDX7` VST3-at.
3. Ha a ROM nincs automatikusan felismerve: **Load ROM...**.
4. Válassz factory bankot/programot vagy tölts be `.syx` bankot.
5. MIDI billentyűzettel vagy Piano Rollból játszd.

A plugin mono DX7 hangot küld mindkét stereo kimenetre, ugyanúgy, ahogy a Retromulator DX7 adaptere.

## Jelenlegi prototípus-korlátok

Ez még **nem végleges release**.

- Ebben a ChatGPT futtatókörnyezetben nincs macOS SDK/Xcode, ezért a tényleges Mach-O `VDX7.vst3` bundle-t itt nem tudtam helyben lefordítani és REAPERben betölteni.
- A saját DX7 engine wrapper C++ kódja külön szintaktikai ellenőrzésen átment.
- A CMake konfiguráció helyben elindul, de a sandbox hálózata nem tudta elérni a GitHubot a JUCE FetchContent letöltéséhez; Macen/GitHub Actions alatt ezt a build rendszer végzi.
- A host sample-rate konverzió az első prototípusban egyszerű lineáris interpoláció. Működési tesztre alkalmas, de később érdemes minőségi sinc/polyphase resamplerre cserélni, különösen 44.1 kHz host sample rate esetén.
- Nincs teljes DX7 front-panel editor. A firmware és a szintézis core működik, a GUI csak ROM/bank/program/SysEx kezelést céloz.
- MIDI Out/SysEx dump küldés nincs bekötve a DAW felé az első verzióban.
- Automatizálható DX7 paraméter-editor még nincs.

## Miért a Retromulator `dx7Lib` port?

Az eredeti VDX7 alkalmazás Linux/X11/JACK környezetre készült. A Retromulator repositoryban található `dx7Lib` ugyanennek a VDX7-emulációnak egy hordozhatóbb, plugin-integrációra már adaptált változata. A projekt csak ezt a DX7 core-részt fordítja; nem építi be a Retromulator teljes alkalmazását/UI-ját.

Upstream:

- VDX7: https://github.com/chiaccona/VDX7
- Retromulator / dx7Lib: https://github.com/reales/retromulator
- JUCE: https://github.com/juce-framework/JUCE

## Licenc

A VDX7 és a Retromulator DX7-emulációs része GPLv3-or-later eredetű. A wrapper forrása ennek megfelelően nyílt forrású prototípus. A JUCE 9 külön licencfeltételekkel (AGPLv3 vagy kereskedelmi JUCE licenc) érhető el; nyilvános bináris terjesztés előtt ezeket a feltételeket külön ellenőrizni kell.

A Yamaha firmware **nincs mellékelve** és nem része a projekt licencének.

## Első teszt, amit érdemes elvégezni

A sikeres macOS build után:

1. REAPER 48 kHz / 128 vagy 256 sample buffer;
2. töltsd be a DX7 v1.8 ROM-ot;
3. factory `ROM1A` → `E.PIANO 1` vagy ugyanazt a `.syx` bankot töltsd be;
4. játssz C3–C5 tartományban;
5. ellenőrizd Note Off, sustain, mod wheel, pitch bend működését;
6. mentsd el a REAPER projektet, zárd be, nyisd újra és ellenőrizd a patch-state visszaállását.

Ha az első macOS compiler/build logban hiba jelenik meg, a log alapján a következő iterációban célzottan javítható a wrapper.
