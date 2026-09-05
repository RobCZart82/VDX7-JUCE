# Third-party components

## VDX7 / Retromulator dx7Lib

The project fetches the `source/dx7Lib` implementation from:
https://github.com/reales/retromulator

That code is adapted from VDX7:
https://github.com/chiaccona/VDX7

The relevant upstream files identify the VDX7 code as GPL version 3 or later.

`Source/VDX7Algorithms.h` records graph topology derived by symbolic tracing of
the fetched `dx7Lib/OPS.h` algorithm ROM. The graph regression test compares all
32 records against that core table; no firmware bytes or hardware artwork are
copied into the diagrams. The JUCE layout and interactive rendering are local
wrapper code.

## JUCE

The wrapper fetches JUCE 9.0.1 from:
https://github.com/juce-framework/JUCE

This release uses JUCE under AGPLv3. See NOTICE.md, LICENSE.txt and the
complete corresponding-source archive, which preserves JUCE's third-party
license files and exact source revision.

## Yamaha firmware

No Yamaha firmware, factory ROM, or SysEx bank is included in this source archive.
