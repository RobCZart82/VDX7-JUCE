# Vector header integration

Embedded own SVG wordmark replaces plain VDX7 text and detached Mk I. label.
Mk 1. is part of the paths, shares the logo bottom and has a 15-unit gap from
the 7. The current SVG applies restrained metallic horizontal scanlines to the
same original paths, retaining vector sharpness without a raster/glitch asset.
CMake embeds SVG alongside existing PNG resources.

The approved three thin decorative lines are rendered across the top header
in the same `0xff575248` tone as the section dividers, at reference y=22, 34
and 46. A ROM-free pixel regression checks the line pixels against a divider
pixel at 1080, 1440 and 1800 editor widths (75%, 100% and 125%).

Two separate text lines remain vector-rendered glyphs, not raster logo content.
Logo bottom and lower text ink bottom use the header buttons' visible bottom
(component bottom minus 1.5px). Separator spans the full inner width below them.
OUTPUT panel top moves from 125 to 135 to avoid the separator; its controls and
bottom remain fixed. The approved operator/keyboard arrangement is unchanged.

Local VST3 build and strict ad-hoc signature verification passed. Processor
regression/screenshot run covers 960/1440/1600 widths with new five-button count,
common-bottom and branding-area-clearance assertions. Screenshots are inspected
for SVG rendering and ink alignment. No new audio parameters or DSP changes.

The previous Xcode helper license warning remains; no license was accepted.
No installed-plugin replacement, tag/release or firmware payload. About redesign,
fine material texture, percentage sizing and physical HiDPI/host acceptance remain.
