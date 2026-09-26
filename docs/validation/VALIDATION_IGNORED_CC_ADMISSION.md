# Ignored CC admission — 2026-09-24

Audit A1 reproduced in actual processBlock with genuine engine mutex contention.
300 CC0 messages followed by Note72 produced peak=0 and failed the deferred
Note On assertion before the fix (exit 1). No injected queue/counter state.

Separate syntactic validity from statically inert adapter events: CC0, CC100,
CC101 and CC32 values8..127 are now rejected before bounded deferred storage.
The same classifier rejects direct engine input before reserveMidi, preventing
irrelevant input from invoking overflow recovery. Other CCs, supported banks,
channel filtering and SysEx validation keep their previous routing.

Actual processor tests now cover all four flood types with deferred audio onset,
release/silence, empty ownership and unchanged persistent settings. Existing
sample-exact partition and expiration tests remain. A ROM-free test exhausts
128 CCs x128 values, checks syntax remains valid, and distinguishes admission
from selected-channel rejection.

Rebuilt affected targets. Targeted CTest 4/4 PASS in53.81s (deferred unit,
v1.8 profile, deferred processor group7.80s, corrected MONO group44.96s).
ROM-free checks7/7 PASS in0.91s. Not a full-suite, sanitizer or DAW claim.
Direct full-serial-FIFO and full sub-CPU-queue rejection need dedicated runtime
controls; early-return placement is source-verified, not that additional test.
Native MONO Note0 remains separately open. No firmware or installed plugin
modified. Next audit work: controlled project-install/epoch interleavings,
bypass lifecycle and failed/flushed controller delivery, each separately tested.
