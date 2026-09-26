# Wheel and wordmark polish validation

## Scope

- The VDX7 Mk I. wordmark remains an embedded SVG with its approved header
  placement. Its original paths now use a restrained metallic horizontal
  scanline pattern; it is not a copied hardware logo or a raster texture.
- The PITCH and MOD wheels retain their existing parameter bindings and mouse
  semantics. Their mechanical housing now has a recessed cavity, layered metal
  bezel and rubber cylinder. The cylinder rib phase and cyan indicator are both
  derived from the normalised slider value.
- PITCH continues to return to centre on mouse release. MOD remains at its last
  value. Slider repainting follows normal drag and mouse-wheel value changes.

## Automated and visual validation

- The existing wheel regression renders values 0.30 and 0.31 plus a repeated
  0.30 frame. It verifies deterministic drawing, a stationary outer frame and
  more than 20 changed rib pixels away from the cyan marker.
- A local firmware-backed `vdx7_processor_tests` run passed, including existing
  960/1440/1600 editor screenshots and a nine-position wheel-frame sheet.
- The 1440 EDIT screenshot and wheel sheet were visually inspected on macOS.
- Local VST3 build passed. The ordinary build helper still reports the pre-existing
  Xcode licence warning; no Xcode licence was accepted or system setting changed.

No firmware, binary payload, installed plug-in, tag or GitHub release is part of
this visual development round. Real host interaction and physical HiDPI/Windows
acceptance remain release-gate work.
