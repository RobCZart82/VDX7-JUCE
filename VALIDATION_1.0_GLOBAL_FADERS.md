# GLOBAL faders and keyboard hover validation

## Scope

- The eight GLOBAL pitch-envelope fader bounds increase from 62 to 74 reference
  units in height. Their captions and value fields were repositioned within the
  existing GLOBAL panel; the panel, graph, LFO controls and approved outer layout
  remain unchanged.
- White-key hover feedback now uses the same inset, rounded visible key body as
  the key fill. It no longer colours the transparent top or bottom image margins.

## Automated validation

- `vdx7_processor_tests` passed with the locally supplied firmware on macOS.
  It exercised the existing state, editor, rendering and screenshot checks.
- The new white-key image regression renders normal and hovered states at 1x and
  2x. It verifies unchanged pixels above and below the visible key body, plus a
  visible changed pixel in the key centre.
- Existing editor geometry checks at 960, 1440 and 1600 reference widths now
  verify that all eight named pitch-envelope faders are at least 73 scaled pixels
  high and leave room for their value fields.
- The 1440-pixel EDIT screenshot was visually inspected after the run.

No installed plug-in, firmware payload, tag or GitHub release was changed.
