# GUI runtime assets and historical references

Reviewed 2026-09-28. The approved appearance is unchanged.

`Resources/GUI/manifest.json` and `VDX7-GUI-Design-Spec.md` describe the historical
raster design pack. Their 1440x1080 canvas, palette and `assets/2x` paths are not
the current production contract. They remain useful reference material, not a
promise that every proposed asset or a full 2x pack ships in the repository.

The current editor draws against a 1440x1110 logical reference canvas. Its five
selectable sizes are defined in `Source/VDX7GuiScale.h`: 600x463, 900x694,
1200x925, 1500x1156 and 1800x1388. Window size and display pixel density are
different variables; the five-size regression is not full desktop HiDPI proof.

## Runtime versus bundled/reference data

- PluginEditor: vector header wordmark, keyboard normal/pressed PNGs, meter
  rail/LED PNGs and the output value-field PNG are used at runtime.
- LookAndFeel: button, tab, knob80, fader and wheel PNG families are used.
- About: VDX7, GYR and signature SVG assets are used; retain all three.
- Chassis, panels, LCD frame, envelope grid and separators are drawn by code.
  Their five obsolete editor image-member loads have been removed; the source
  PNG/reference files remain. No blanket raster deletion was performed.
- CMake still embeds the PNGs under `Resources/GUI/1x` and SVGs under `Vector`.
  Bundled data is not the same as an image decoded per editor instance. This
  change reduces decoded runtime memory, not a claimed package-size reduction.

Runtime source and actual CMake inputs take precedence over historical asset
patterns. See [measurement and visual regression evidence](../validation/VALIDATION_20260928_NONHOST_HARDENING.md).

PITCH mouse drag release returns to centre; keyboard adjustment retains its
value by the owner's explicit 2026-09-28 decision. No input redesign is intended.
