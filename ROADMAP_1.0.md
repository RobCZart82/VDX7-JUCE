# VDX7 1.0.0 release gate

Target: stable 1.0.0, not another public pre-beta. Work-in-progress builds are
not final releases. Do not publish or replace existing release assets until
the acceptance checklist is complete. Keep plugin IDs and existing parameter
indices compatible with saved projects.

## Stabilization

- [x] Deferred multi-block MIDI timeline, restart cleanup and consistent program
  normalization implemented; see `VALIDATION_1.0_MIDI_LIFECYCLE.md` for policy
  and local tests. Actual host transport acceptance remains open.

- [x] Ordered program/bank/voice/operator edit queue with local save/audio
  regressions; see `VALIDATION_1.0_EDIT_ORDER.md` for scope and overload policy.

- [x] Missing-ROM deferred project restore, including re-save/restart (local automated tests).
- [x] Invalid-ROM rejection preserves RAM and pending UI edits; firmware-only reload tested.
- [x] CC64/65 thresholds and CC11 expression regression tests.
- [x] Preserve MIDI note-offs during engine transactions; bounded overflow recovery tested.
- [x] Validated live bank SysEx with coherent GUI/host state (local tests).
- [x] Remove 512-sample lookahead; measure onset and host-block invariance.
- [ ] Physical MIDI timing, dense chords and automation acceptance in hosts.
- [x] Move direct voice-parameter notifications outside the audio callback and
  engine lock; editorless/reentrant state tests (see `VALIDATION_1.0_HOST_PUBLICATION.md`).
- [ ] Complete remaining audio-thread allocation/locking audit and contention stress test.
- [x] Guard firmware serial/controller saturation; validate recovery with a
  60-second simulated load and callback ordinary-C++ heap probe
  (`VALIDATION_1.0_MIDI_OVERLOAD.md`). Direct-C/aligned heap profiling and live
  host soak acceptance remain open; this is not a hard-realtime guarantee.
- [x] Remove keyboard-state locking/listeners from audio; bounded UI MIDI handoff
  and 12-configuration local stress grid (see `VALIDATION_1.0_REALTIME_BASELINE.md`).
  Full allocation/overload audit and host acceptance are still open.
- [x] Full 0–127 note range and pitch/release regressions (local ROM).
- [x] Configure PR ROM-free CI and opt-in local ROM integration CTest gate.
- [x] Run the development CI configuration on GitHub: macOS and Windows passed
  on `5164fabd36c8fdd745e272fc1f493c0c352c1ced`. Every subsequent change needs
  fresh checks; this does not constitute RC or host acceptance.

## Features and quality

User decision (2026-09-20): milestones through 4 may be developed, with PR/CI
gates between chapters. Milestone 4 uses firmware-faithful behavior as its base;
modern extensions require a separate decision. Subsequent user approval extends
development to the agreed GUI/function integration and Save As workflow below.
Release publication remains outside this authorization. Each chapter still needs
fresh PR checks and user merge before the next chapter.

- [ ] PERFORMANCE/SETTINGS scope implemented and tested: pitch range, controller
      assignments, MIDI input channel and tuning, preserving state compatibility.
- [ ] Decide mono/portamento scope from firmware capabilities and host tests.
- [ ] Measure resampling/aliasing; quality implementation accepted against references.
  Milestone 3B adds band-limited SRC, selected spectral limits, exact reported
  latency and CPU diagnostics (`VALIDATION_1.0_BANDLIMITED_SRC.md`). Physical
  reference/host listening and latency-compensation acceptance remain open.
- [ ] Current documentation matches all shipping features and limitations.

## Agreed GUI direction — 2026-09-20 (not implemented)

The three GUI visual concepts remain layout references, not a finished skin.
The approved hardware-inspired material/colour direction is specified below.
The following functional/layout requirements were explicitly agreed with the
user. Record them now; implementation follows the stabilization work.

- [ ] Make the LCD the single bank/patch navigation centre: bank selector,
  direct program selector, current program number/name and modified-state `*`.
- [x] LCD left/right arrows provide the same previous/next preset behavior as
  the existing header quick switch. Remove that header switch and its duplicate
  preset display. Keep the LCD and navigation available in EDIT, PERFORMANCE
  and UTILITY views; view switching must not hide them.
- [x] Place an explicitly numbered algorithm dropdown (1–32) beside the
  algorithm diagram. Selection updates the diagram immediately. Remove the
  duplicate GLOBAL ALGO encoder, while retaining the existing underlying host
  parameter ID/index and saved-project/automation compatibility.
- [ ] Output level-meter LEDs run predominantly GREEN from the bottom upward,
  then YELLOW near the top and RED at the very top. No blue/cyan lower LEDs.
  Exact level thresholds and clipping indication remain implementation details
  to specify and validate; the concepts do not define a calibrated meter scale.

Magyar összefoglaló: állandó LCD-s bank-/hangszínkezelés bal–jobb léptetéssel;
a felső gyorsváltó megszűnik. Az algoritmusábra mellett 1–32-es lenyíló lista
váltja a GLOBAL ALGO tekerőt. A szintmérő alulról nagyrészt zöld, majd sárga,
legfelül piros; kék alsó LED-ek nélkül. A hardverfotók alapján elfogadott
anyag- és színvilág alább szerepel. Az első funkcionális GUI-bekötési kör
szintén alább található; a végleges
grafikai átdolgozás és a PERFORMANCE-oldal még külön feladat.

### Final visual direction — hardware references approved 2026-09-20

User-supplied `concept-1.jpg` and `concept-4.jpg` are visual references only;
do not copy the photos, their branding or cropped surface textures into the
plug-in or redistribute them as project assets. The direction is an original
VDX7 interface with a restrained vintage hardware feel, not a replica front panel.

- [ ] Warm brown-charcoal enclosure with a fine-grained matte finish instead of
  the current blue-metal appearance. Use subtle depth and natural shading;
  avoid artificial wear, heavy chrome, excessive neon or bloom.
- [ ] Yellow-green LCD with dark, high-contrast characters, a slightly recessed
  black surround and a readable character-display feel. Preserve the modern
  bank/program selectors, patch name/number, modified indicator and arrow buttons;
  do not reproduce the photographed display's text or impose its limited layout.
- [ ] Restrained turquoise, lavender and salmon button surfaces assigned
  consistently by function group. States must also be recognisable through
  labels, position or shape, not colour alone.
- [ ] Off-white, readable labels and simple panel-divider lines. Test typography
  and control states at supported small sizes as well as Retina/HiDPI scales.
- [ ] Black ribbed wheels and slider caps, natural ivory-white keys, subtle
  highlights and clear press/hover/focus states. No invented aged/grimy texture.
- [ ] Original VDX7 wordmark, icons and newly drawn interface assets. Rework the
  current striped wordmark toward a more independent identity; do not reuse
  Yamaha logos, original product lettering or trace the original front panel.
- [ ] Preserve the approved EDIT/PERFORMANCE structure and functional LCD,
  SAVE AS..., algorithm, output and keyboard controls across views. The colour
  redesign must not remove, duplicate or disconnect implemented functionality.
- [ ] Level-meter LEDs remain predominantly green from the bottom upward,
  then yellow and red at the top; no cyan/blue lower segments. Meter colours
  remain distinct from the turquoise used for controls.

Magyar irányelv: meleg barnás-antracit, finoman szemcsés matt ház; sárgászöld
LCD fekete kerettel; visszafogott türkiz, levendula és lazac gombszínek;
törtfehér feliratok, fekete bordázott kezelőszervek, elefántcsontszínű billentyűk.
Saját VDX7-arculat és újrarajzolt elemek, eredeti márkajelzés és fotókivágások
nélkül. Ez elfogadott tervezési irány, nem már elkészült GUI vagy jogi minősítés.

## GUI/function integration chapters — approved 2026-09-20

The PERFORMANCE concept based on visual concept 3 is the agreed layout direction,
not a specification of implemented functionality or controller default values.
Implement real bindings, persistence and round-trip tests alongside each control.
The existing skin remains in use during the first functional integration chapter.

1. **Navigation/export wiring (merged PR #11):** move preset arrows to the LCD,
   replace the algorithm encoder with its numbered dropdown using the unchanged
   host parameter, expose existing single-voice and 32-voice SysEx export through
   a persistent SAVE AS... header button. This is file export, NOT yet a USER
   library. Existing UTILITY export remains available. Factory ROM is never written.
2. **Firmware-backed PERFORMANCE:** establish parameter ranges and behavior from
   the core/firmware, then connect the approved play mode, pitch bend, portamento
   and four controller-assignment panels. No decorative active controls. Keep LCD,
   algorithm, output, Save As and keyboard available across views. Test both GUI
   and MIDI paths, project restore and compatibility with the 148 existing IDs/indices.
   SETTINGS MIDI channel/tuning are included in this functional work.
   First slice: four controller range/assignment panels connected to firmware
   battery RAM, persistent project recall and EDIT/PERFORMANCE switching.
   No new host automation IDs; these are message-thread global controls.
   Play mode, pitch bend/portamento and SETTINGS remain subsequent slices.
3. **USER preset storage / Save As:** default destination is a USER bank + slot,
   with name entry and explicit occupied-slot overwrite confirmation. Preserve
   factory originals; make saved USER banks selectable from the LCD. Also support
   single-patch export and whole-bank export. Separate voice payload from global
   PERFORMANCE settings; ordinary voice SysEx must not silently claim to contain
   controller/global state. DAW project save remains independent. Verify exact
   export/import, cancellation, failed writes, changes while dialogs are open,
   missing files, bank switching and session restart before marking complete.
4. **Visual integration and acceptance:** combine concept 3's approved layout
   with the hardware-inspired material/colour direction above for
   EDIT/PERFORMANCE/UTILITY, green-yellow-red meters, resize/readability
   checks and real host interaction checks. Generated mockup is reference only,
   not a screenshot of the shipping plugin. No release until all release gates pass.

- [x] First-stage SAVE AS... button exposes real voice/bank SysEx file export.
- [ ] USER bank library, destination slot and non-destructive Save As workflow.
- [ ] PERFORMANCE panel layout with firmware-backed bindings and project recall.
- [x] First PERFORMANCE slice: four controller ranges and 12 assignment switches,
  project/missing-ROM recall, real MIDI/audio regression and persistent LCD/header.
- [ ] Final hardware-inspired visual treatment, original VDX7 identity and host usability acceptance.

## Publication gate

- [ ] M1 REAPER acceptance: multiple instances, automation, transport stop,
      missing ROM, state restore, SysEx, offline render, 44.1/48/96 kHz,
      64/128/256/512 sample buffers.
- [ ] Windows REAPER acceptance and runtime prerequisites verified.
- [ ] Physical Intel Mac acceptance, or explicitly do not claim Intel support.
- [ ] All distributed plugin formats have host validation.
- [ ] Version 1.0.0 in one source of truth; unique exact source commit/tag.
- [ ] Verified signatures, corresponding sources, notices, checksum manifest.
- [ ] No Yamaha firmware/factory ROM in source or binary release archives.
- [ ] Final release notes and package installation instructions approved.
- [ ] Publish stable release only after the preceding gates; never clobber old assets.

The installed plugin and the original local development checkout are preserved.
Local full-range development commit 80ebf54 and reviewed main 483daf7 have been
merged into the isolated codex/1.0-stabilization branch.
