# LCD horizontal clearance

The LCD frame moves from x=382,width=555 to x=412,width=525.
The display moves from x=392,width=535 to x=422,width=505.
Both retain their previous right edges, y coordinates and heights (112/86).
UTILITY ends at x=384, leaving 28 reference units before the frame.

Previous arrow and bank/program row move 30 units right. The patch name region
shrinks by 30 units; the next arrow remains fixed. Navigation logic is unchanged.

Validation: local macOS arm64 VST3 build and strict ad-hoc signature verification
passed. Full processor regression executable with local ROM and screenshot output
passed, including layout/selector containment and UTILITY gap checks at widths
960, 1440 and 1600. Inspected 960px EDIT preview. This targeted layout round did
not rerun the entire CTest suite. Existing Xcode license warning remains unchanged.

No installed plugin replacement, tag, release or ROM payload. New header/logo,
size presets and final visual polish remain separate chapters.
