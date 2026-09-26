# LCD navigation and dividers — development validation

Scope: integrated LCD previous/next graphics and section divider layout.

- Previous/next controls now draw dark vector chevrons and flat outlines over
  the LCD background, matching the BANK/program selector ink. Hover/press use
  dark translucent feedback; keyboard focus thickens the outline, disabled
  graphics are muted. No hardware gradient or bright external-button skin.
- Arrow hit areas are inset inside the LCD; navigation callbacks and wrapping
  remain unchanged. Accessible names identify previous/next patch.
- Native full-width section rules replace partial/fading bitmap rules. The
  operator rotary subsection gets a separator between its two rows; the lower
  row moves down 12 reference units to provide clearance. Envelope is unaffected.
- GUI regression tests at 960/1440/1600 widths check LCD style markers,
  containment and selector/arrow hit-area separation, and existing 01/32 wrap.

Local macOS arm64: VST3 and processor test targets built; full CTest 10/10 passed
(56.07 seconds). Strict ad-hoc signature verification passed. No installed plugin
was replaced. Xcode helper still reports the pre-existing license warning;
the license was not changed or accepted by the agent.

Remaining: real-host hover/focus/pressed acceptance, Windows/HiDPI visual checks,
header/logo integration, About redesign and selectable 75/100/125% UI sizes.
This chapter does not claim final GUI polish or complete the header alignment.
No tag/release, firmware or factory ROM payload is included.
