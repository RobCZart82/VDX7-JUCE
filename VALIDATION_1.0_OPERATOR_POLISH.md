# Operator layout and vector control rendering

## Scope

- Operator panel expands downward 40 reference units. Keyboard and footer move
  down 40, outer 1440x1110 reference size remains unchanged. PERFORMANCE panels
  follow the expanded area without leaving the old background boundary behind.
- Envelope faders: height 116 -> 169. Graph: height 164 -> 239, bottom y819,
  matching the envelope value-row bottom. Graph repaint region grows with it.
- Upper operator knobs: 52 -> 58; lower knobs: 38 -> 44; rows spaced around a
  full rotary-subsection divider. No controls are added or removed.
- Vector bevels/knurled knob rims and scaled markers; vector slot/tick fader
  tracks and mechanical caps replace stretched track/cap bitmaps. Panel and
  envelope backgrounds use subtle gradients. Audio/parameter mappings untouched.

## Validation

Local macOS arm64 VST3 build and strict ad-hoc signature verification succeeded.
CTest: 10/10 passed (56.76 seconds). An additional final processor screenshot
run checks 960/1440/1600 sizes after the PERFORMANCE background adjustment.
Eight named operator envelope sliders have explicit height/bottom-bound tests;
existing binding, recall, preset navigation and PERFORMANCE tests remain enabled.
EDIT 1440 and PERFORMANCE 960 were visually inspected during iteration.

The pre-existing Xcode helper license warning remains; no license was accepted.
No installed plugin was replaced, no tag/release created, no ROM redistributed.

## Remaining

This is an incremental final-artwork implementation chapter, not final skin
acceptance. Header/logos/About, fine material texture, percentage sizing,
outer-margin fitting, physical Retina/Windows/DAW interaction and performance
acceptance remain. No claim that screenshots alone validate all interaction states.
