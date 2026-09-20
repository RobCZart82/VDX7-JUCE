# Hardware-inspired GUI foundation

First code-native styling pass based on the approved concept: warm brown-charcoal
enclosure/panels, yellow-green LCD, off-white labels, restrained turquoise active
states, matte knobs, ON/OFF-labelled performance switches and green/yellow/red
meter segments. The old striped VDX7 artwork is no longer drawn; a simple VDX7
typographic heading establishes an independent interim identity.

GYR-Logo2.png is embedded unchanged in ABOUT with existing source/license notices.
The user-provided hardware photos and concept mockups are references only and
are not included as texture assets. No AI-generated bitmap is needed for this
code-native visual pass.

Bindings, host IDs, coordinates, LCD navigation, bank/save workflow and DSP remain
unchanged. The meter retains its existing 12-segment -60..0 dB mapping (8 green,
2 yellow, 2 red); this is a level display, not a newly calibrated analog VU model.
Buttons retain labels and focus feedback; switches show both position and ON/OFF.

This is NOT the finished skin. Performance play controls remain functional
dropdowns, not the concept's decorative knobs. Remaining work includes grouped
performance layout, fader/wheel refinement, lavender/salmon function accents,
final wordmark, subtle material texture, all modal styling and physical HiDPI/
Windows/host interaction acceptance. Legacy unused background assets remain in
the pack until a separately reviewed cleanup; no source artwork is deleted.

GUI tests cover 960/1440/1600 layouts and assert ABOUT contains a decoded GYR
image. Local screenshots are inspected separately. No ROM, installed plugin
replacement, tag or release publication is part of this development chapter.

Final local macOS arm64 full CTest: 10/10 passed in 57.80 seconds. VST3 build
and strict ad-hoc signature verification passed. Existing Xcode licence warning
from the helper remains; no system licence was accepted or settings changed.

## PERFORMANCE grouping follow-up

Three top cards now group PLAY MODE, PITCH BEND (Range/Step) and PORTAMENTO
(Mode/Glissando/Time), instead of one undifferentiated six-field grid. The top
row retains the existing width reservation for OUTPUT; all six controls remain
functional selectors with unchanged names, callbacks, persistence and semantics.
Shorter labels avoid redundant group-name prefixes at small sizes. Four controller
cards below are unchanged. Layout regression asserts play/portamento membership
alongside existing bounds and binding checks at 960/1440/1600 pixels.

Local follow-up CTest: 10/10 passed in 57.85 seconds. Additional screenshot run
passed; the 960-pixel PERFORMANCE image was visually inspected. VST3 build and
strict ad-hoc signature verification passed. Fresh PR CI remains required.

Still not a final skin or a host-acceptance claim. Knob-versus-selector refinements,
fader/wheel styling, typography, material treatment and Windows/HiDPI acceptance
remain follow-up work.
