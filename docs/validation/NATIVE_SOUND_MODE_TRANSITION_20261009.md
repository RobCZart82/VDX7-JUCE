# Classic és Clean natív motorátmenet ellenőrzése

2026-10-09. Baseline: `edb0de367cc6b909af5f7e6f493fa8e0d005c23a` (#175).
Ág: `feature/engine-sound-mode-transition`, [PR #176](https://github.com/RobCZart82/VDX7-JUCE/pull/176).
Helyileg ellenőrzött kód: `3d26db6b45df2987c85e5fe42a9eae32fa801b7f`.
Az utána következő PR-linkes dokumentációs commit nem módosít futtatott kódot.
Ez a valódi motor native/SRC
mintasorrendjének részlépése. A processor Clean betöltése és SETTINGS kapcsolója
még nem aktív; a következő release csak a teljes integráció és elfogadás után kész.

## Minta és mód rendezése

Az EGS hat operátort és tizenhat voice-ot léptet 96 órajel alatt. Az adapter
e teljes kör elején készíti el a kívánt módhoz tartozó következő gain-lépést;
a mód a nulla gainű kör **első** OPS-órája előtt változik. A részleges korábbi
kör az eredeti módjával és gainjével fejeződik be. A gain az elkészült mintára
kerül, nem a későbbi buffer-fogyasztáskor.

A 256 le- és 256 felmintás natív mute-ramp latest-request elvű, nincs queue.
Azonos request nem indítja újra, visszavonás nem alkalmaz régi kívánt módot.
Instrukcióhatári overshoot nem vész el, és egy instrukció CPU-léptetése nem
válik több firmware-instrukcióvá. Steady Classic ugyanazt a korábbi clockot
hívja; nincs új gain- vagy filter-művelet ezen az ágon.

SRC reset, prepare és CPU boot nem reseteli a core EGS körpozícióját.
Az adapter is megtartja a fázist, kívánt/aktív módot és rámpát. Elutasított ROM
nem változtatja e mintasorrendet. A live scalar request nem ír RAM-ot,
programot, bankot vagy latency-t, és nem hajt végre plusz bootot.

## Reprodukció és automatizált eredmények

macOS ARM64, JUCE 9.0.3, Release; célzott RelWithDebInfo ASan/UBSan.

- `vdx7_native_sound_mode_tests`: PASS. A 17 óránál érkező request megtartja
  a félkör régi módját; a 256. új teljes kör nulla gainnel vált. Minden gain
  és minta ellenőrzött, a többletórák és mintaszám megmaradnak.
- A `--mid-scan-negative-control` szándékosan félkörben kapcsol: exit 1 FAIL
  a félkör-megőrzési ellenőrzésnél. Ez teszt-only hibás caller, nem production hiba.
- Saját EGS regiszter-stimulus: PASS. A Classic adapter bitazonos a régi közvetlen
  EGS-clockkal. A Clean eltér, véges; azonos absolute-clock kérések kimenete
  1/4/28/37/196/511-es clock darabolásban azonos.
- Actual engine adapter lifecycle: PASS. Reset/prepare/no-ROM MIDI lifecycle
  őrzi a fázist és intentet; régi native buffer fogyasztása nem lépteti a rampet.
- ROM-free CTest: 22/22 PASS. Python: 94/94 PASS, nincs SKIP.
  Tényleges CTest inventory és checker self-test: PASS. Az új target a
  Windows/macOS ci_checks és ROM-free sanitizer workflow része.
- macOS VST3/AU/Standalone és ci_checks dev-build: PASS; nincs telepítés.
- Célzott ASan/UBSan új adapter, privát firmware-es futással is: PASS.
  `detect_leaks=0:halt_on_error=1`; `halt_on_error=1:print_stacktrace=1`.
- Meglévő actual processor snapshot/admission/owner/stale-ROM matrix privát
  v1.8-cal: PASS. Privát state-transition és routing-publication regressziók: PASS.

Az első firmware-próba boot-default hangszínnel nem adott megfelelő nem nulla
stimulus-bizonyítékot, és a szigorú mérési feltételnél FAIL-t adott. A végső
fixture explicit, saját carrier-paramétereket használ; a nem nulla és eltérő
Clean-kimenet feltétele megmaradt. Nem gyári patch-adatot építünk be.

## Privát eredeti 1.8 mérések

Firmware: 16 384 bájt, SHA256
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Firmware-bájtok és factory bankok nem kerülnek Gitbe, CI-be vagy release-be.

Az opcionális teszt a `vdx7_native_sound_mode_tests` egy helyi firmware-fájlos
argumentumával fut. Saját carrier, tartott hang, gyors request-visszavonások,
sustain/Note Off/tail: 44,1/48/96 kHz-en PASS. Az azonos host-offset idővonal
1/64/257/511 mintás darabolása minden esetben pontosan azonos kimenetet ad.
Peer Classic példányok azonosak, a másik példány módváltása nem érinti őket.

| Host rate | Classic peak | Clean peak | Eltérő settled minták |
|---|---|---|---|
| 44 100 Hz | 0,00966722 | 0,00969887 | 4112 |
| 48 000 Hz | 0,00966785 | 0,00969178 | 4112 |
| 96 000 Hz | 0,00966688 | 0,00964658 | 3982 |

Release mérés, nem teljes bank/headroom vagy hangminőség-értékelés. A Clean
eltérés nem automatikusan javulás. A sanitizer optimalizálása utolsó számjegyben
eltérő metrikát adhat; a két build közötti bitazonosság nem e mérés állítása.

Külön Classic összehasonlítás: baseline Engine.h/cpp a fenti main SHA-ról,
azonos dependency-k, fordító és `-O2`, a `VDX7ClassicRenderReference.cpp`
harness változatlanul mindkét motorral. Három rate, 0/15/31-es algoritmus,
0/7 feedback, saját paraméterek és azonos Note On/Off: 18 eset, összesen
589 824 véges float minta **byte-ra azonos**. A kimenet SHA256 mindkét oldalon:
`2d7a7f88ea784f6ac1345e02901f2980bb38889e5e5a200c3330b56c4ecb41a0`.
A `vdx7_classic_render_reference` csak explicit helyi build-target; raw float
streamet ad stdout-ra, nem CTest vagy firmware-free CI-teszt.

## További integráció és kiadási kapuk

A processor még nem dispatchol módot az engine-re; Clean project admission,
writer és SETTINGS változatlanul gated. Cold/kompatibilis project install első
mintától unity-gain indulása, a processor-owned audio request és az összes
reset/reload/mono/restore út összekapcsolása következik. A jelen adapter a már
futó natív scan/ramp megőrzését igazolja, nem e teljes processor-életciklust.

Clean alatt az upstream Classic analóg szűrőhistory megmarad, nem lép;
visszatérésének spektrális és hallásos elfogadása még szükséges. A mute-dip és
256+256 hossz nem végleges UX vagy kattanásmentességi elfogadás. Teljes gyári
bank/combined-ROM mátrix, CPU/allokációs mérés, TSAN/LeakSanitizer, REAPER,
hallásos teszt és új installer/source csomag ebben a körben NOT RUN.

Final-head platform/sanitizer CI, érdemi review és merge utáni külön main kör
még kapu. Következő munkák az [egységes tervben](../development/DEVELOPMENT_PLAN.md).
Publikált 1.0.1, tagek, release assetek, GUI-képek és telepített plugin változatlan.
