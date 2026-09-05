HISTORICAL GUIDE: v0.1.0 only. For v0.6.6 use README.md / README_HU.md.
REGI UTMUTATO: csak v0.1.0. A v0.6.6-hoz: README.md / README_HU.md.

VDX7-JUCE v0.1.0 Pre-Beta 1
Yamaha DX7 Mk I hardware-emulation VST3
============================================================

MAGYAR
============================================================

Ez egy korai PRE-BETA tesztverzió.

A VDX7-JUCE NEM tartalmaz Yamaha firmware/ROM fájlt.
A felhasználónak saját, jogszerűen beszerzett Yamaha DX7 Mk I firmware-t kell használnia.

Tesztelve:
- macOS Apple Silicon / Mac mini M1 + Cockos REAPER
- Windows x64 + Cockos REAPER

1. macOS VST3 TELEPÍTÉS
-----------------------
Felhasználói VST3 mappa:
    ~/Library/Audio/Plug-Ins/VST3/

Teljes útvonal:
    ~/Library/Audio/Plug-Ins/VST3/VDX7.vst3

Alternatív rendszer-szintű mappa:
    /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Ajánlott ROM hely:
    ~/Library/Application Support/VDX7-JUCE/ROM/dx7.bin

Kompatibilitási útvonal:
    ~/Library/Application Support/discoDSP/Retromulator/ROM/dx7.bin

REAPER:
    Preferences > Plug-ins > VST > Re-scan

macOS Gatekeeper / quarantine
-----------------------------
A GitHub Actions macOS build ad-hoc aláírt, de nem Apple Developer ID
aláírt és nem notarizált.

Először próbáld meg Terminal-parancs nélkül.

Felhasználói VST3 mappánál:
    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/VDX7.vst3"

Rendszer VST3 mappánál:
    sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Aláírás ellenőrzése:
    codesign --verify --deep --strict --verbose=2 /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Csak valódi signature hiba esetén:
    sudo codesign --force --deep --sign - /Library/Audio/Plug-Ins/VST3/VDX7.vst3

2. WINDOWS x64 VST3 TELEPÍTÉS
-----------------------------
Másold a teljes VDX7.vst3 mappát ide:
    C:\Program Files\Common Files\VST3\VDX7.vst3

REAPER:
    Options > Preferences > Plug-ins > VST > Re-scan

ROM hely Windowson
------------------
A plugin a Windows által ténylegesen jelentett Documents / Dokumentumok
Known Foldert használja.

Ajánlott:
    <Windows Documents>\VDX7-JUCE\ROM\dx7.bin

Hagyományos Documents mappa:
    C:\Users\YourName\Documents\VDX7-JUCE\ROM\dx7.bin

OneDrive Documents Backup esetén például:
    C:\Users\YourName\OneDrive ...\Dokumentumok\VDX7-JUCE\ROM\dx7.bin

Kompatibilitási útvonal:
    <Windows Documents>\discoDSP\Retromulator\ROM\dx7.bin

Ha az automatikus keresés nem talál ROM-ot:
    Load ROM...

3. TÁMOGATOTT ROM-ELRENDEZÉSEK
------------------------------
A) 48 KB kombinált dx7.bin:
    49 152 byte = 16 KB firmware + 32 KB factory voice adat

B) 16 KB firmware:
    16 384 byte

16 KB firmware mellett a plugin automatikusan keresi:
    dx7_factory_voices_32KB.bin

Méret:
    32 768 byte

DX7 Mk I v1.8 / IG11469 referencia:
    Size:   16 384 bytes
    CRC32:  6cbb0865
    SHA-1:  715dbb8e96a4df2a7f096b368334a7654860bb26

A Yamaha firmware/ROM NINCS a release-ben.

4. .SYX BANKOK
--------------
Standard Yamaha DX7 32-voice bulk SysEx bank támogatott.
Elvárt méret:
    4 104 byte

Használd:
    Load .SYX...

5. PRE-BETA 1 VALIDÁLT ALAPFUNKCIÓK
-----------------------------------
- macOS Universal VST3 (arm64 + x86_64)
- Windows x64 VST3
- REAPER plugin betöltés
- ROM auto-detect / manuális ROM load
- 8 factory bank
- MIDI Note On/Off
- polyphony / akkordok
- preset/program váltás
- CC64 Sustain / Hold Pedal
- REAPER project mentés/újranyitás preset-visszaállítással
- offline render

Hibajelentés:
    https://github.com/RobCZart82/VDX7-JUCE/issues

Yamaha firmware-t vagy ROM-fájlt NE tölts fel GitHubra.

============================================================
ENGLISH
============================================================

This is an early PRE-BETA test build.

VDX7-JUCE does NOT include Yamaha firmware/ROM files.
Users must provide their own legally obtained Yamaha DX7 Mk I firmware.

Tested with:
- macOS Apple Silicon / Mac mini M1 + Cockos REAPER
- Windows x64 + Cockos REAPER

1. macOS VST3 INSTALLATION
--------------------------
Per-user VST3 folder:
    ~/Library/Audio/Plug-Ins/VST3/

Full path:
    ~/Library/Audio/Plug-Ins/VST3/VDX7.vst3

Alternative system-wide location:
    /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Recommended ROM location:
    ~/Library/Application Support/VDX7-JUCE/ROM/dx7.bin

Compatibility location:
    ~/Library/Application Support/discoDSP/Retromulator/ROM/dx7.bin

REAPER:
    Preferences > Plug-ins > VST > Re-scan

macOS Gatekeeper / quarantine
-----------------------------
The GitHub Actions macOS build is ad-hoc signed, but it is not Apple
Developer ID signed and is not notarized.

Try normal installation first.

Per-user VST3:
    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/VDX7.vst3"

System VST3:
    sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Verify signature:
    codesign --verify --deep --strict --verbose=2 /Library/Audio/Plug-Ins/VST3/VDX7.vst3

Only if signature verification fails:
    sudo codesign --force --deep --sign - /Library/Audio/Plug-Ins/VST3/VDX7.vst3

2. WINDOWS x64 VST3 INSTALLATION
--------------------------------
Copy the complete VDX7.vst3 directory to:
    C:\Program Files\Common Files\VST3\VDX7.vst3

REAPER:
    Options > Preferences > Plug-ins > VST > Re-scan

Windows ROM location
--------------------
The plugin uses the actual Windows Documents Known Folder.

Recommended:
    <Windows Documents>\VDX7-JUCE\ROM\dx7.bin

Typical path without OneDrive:
    C:\Users\YourName\Documents\VDX7-JUCE\ROM\dx7.bin

With OneDrive Documents Backup:
    C:\Users\YourName\OneDrive ...\Documents\VDX7-JUCE\ROM\dx7.bin

Compatibility location:
    <Windows Documents>\discoDSP\Retromulator\ROM\dx7.bin

If automatic detection fails:
    Load ROM...

3. SUPPORTED ROM LAYOUTS
------------------------
A) 48 KB combined dx7.bin:
    49,152 bytes = 16 KB firmware + 32 KB factory voice data

B) 16 KB firmware:
    16,384 bytes

With 16 KB firmware the plugin automatically looks for:
    dx7_factory_voices_32KB.bin

Size:
    32,768 bytes

DX7 Mk I v1.8 / IG11469 reference:
    Size:   16,384 bytes
    CRC32:  6cbb0865
    SHA-1:  715dbb8e96a4df2a7f096b368334a7654860bb26

Yamaha firmware/ROM is NOT included.

4. .SYX BANKS
-------------
Standard Yamaha DX7 32-voice bulk SysEx banks are supported.
Expected size:
    4,104 bytes

Use:
    Load .SYX...

5. PRE-BETA 1 VALIDATED BASICS
------------------------------
- macOS Universal VST3 (arm64 + x86_64)
- Windows x64 VST3
- REAPER plugin loading
- ROM auto-detection / manual ROM loading
- 8 factory banks
- MIDI Note On/Off
- polyphony / chords
- preset/program switching
- CC64 Sustain / Hold Pedal
- REAPER project save/reopen preset restoration
- offline rendering

Bug reports:
    https://github.com/RobCZart82/VDX7-JUCE/issues

Do NOT upload Yamaha firmware or ROM files to GitHub.

============================================================
LICENSE / THIRD-PARTY
============================================================
VDX7-JUCE is an open-source wrapper project.
Release archives include LICENSE.txt and THIRD_PARTY.txt.
Yamaha DX7 firmware/ROM data is NOT distributed with VDX7-JUCE.
