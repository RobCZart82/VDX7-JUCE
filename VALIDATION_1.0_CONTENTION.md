# Engine contention measurement foundation

Base: main 3e2fe46dd9090216c44377879884531c1f441dd7 (PR31).

Added instance-lifetime atomic counters for positive-length audio callbacks
which fail the engine try-lock while ROM is loaded: blocks and samples. No
logging, allocation, persistence or GUI reset on the callback. These counters
measure deliberate silence from engine contention, not every possible dropout,
OS underrun, or normal silence. Exposed to the regression friend, not a new
host parameter or user-interface control. Successful blocks do not increment.

Deterministic two-thread probe holds the engine mutex while three 256-sample
callbacks finish. Exactly 3 blocks/768 samples are counted and confirmed silent
(16 ms at 48 kHz). Callback completion timeout is a deadlock guard, not a realtime
deadline. This forced schedule does NOT measure normal GUI dropout frequency.

One local diagnostic run measured POLY->MONO at 2.48479 ms and MONO->POLY at
0.739375 ms. 1000 uncontended PERFORMANCE getter triplets took 0.044166 ms.
These are wall times from this run, not maximums or promises; no timing threshold
is asserted. Simulated firmware audio duration must not be confused with them.
At a small buffer even a millisecond-scale transaction warrants investigation.

Next: use the counters during real GUI/audio overlap; replace routine display
reads with suitable snapshots and evaluate bounded settings transactions.
The three PERFORMANCE getters still take engineMutex_, and mode changes still
perform firmware work under that mutex. This chapter does not fix those paths
or declare audio continuity accepted. Intentional voice reset on mode change
must be distinguished from missing audio blocks.

GUI layout, firmware routing, parameter IDs, state schema and installed plugin
are unchanged. No release/tag or firmware upload.

Local arm64 VST3/all-test build passed; full CTest 10/10 passed in 69.24s.
Strict ad-hoc signature verification passed. The existing Xcode license warning
remains, with no automatic license acceptance. Windows/Universal CI is separate.
