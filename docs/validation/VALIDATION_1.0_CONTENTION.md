# Engine contention measurement foundation

Base: main 895ac5dd669d (PR42).

Added instance-lifetime atomic counters for positive-length audio callbacks
which fail the engine try-lock while ROM is loaded: blocks and samples. No
logging, allocation, persistence or GUI reset on the callback. These counters
measure deliberate silence from engine contention, not every possible dropout,
OS underrun, or normal silence. Exposed to the regression friend, not a new
host parameter or user-interface control. Successful blocks do not increment.
The processor also records the longest contiguous run of contended audio
samples. This distinguishes a few scattered callbacks from one longer silent
section; it is an instance-lifetime high-water mark, not a host underrun meter.

Deterministic two-thread probe holds the engine mutex while three 256-sample
callbacks finish. Exactly 3 blocks/768 samples are counted and confirmed silent
(16 ms at 48 kHz), and the longest contiguous run is exactly 768 samples.
Callback completion timeout is a deadlock guard, not a realtime deadline. This
forced schedule does NOT measure normal GUI dropout frequency.

The direct POLY/MONO transaction now records the last and peak time spent after
the caller has acquired `engineMutex_`, in microseconds. It intentionally
excludes time spent waiting for the mutex, host scheduling and display
notification; the stress test separately prints the complete caller wall time.
The diagnostic clamps a sub-microsecond measurement to one microsecond so a
completed transaction is observable on all supported clocks. There is no timing
threshold: these are local measurements, not a maximum or promise. Simulated
firmware audio duration must not be confused with either measurement. At a small
buffer even a millisecond-scale transaction warrants investigation.

Routine PERFORMANCE display reads now use one coherent packed snapshot. The
legacy-shaped controller, play and pitch-bend accessors decode that same
snapshot instead of acquiring `engineMutex_`; a held-lock regression confirms
all four read paths complete while the engine mutex remains owned elsewhere.

Frequent non-disruptive UI writes are now coalesced in atomics and applied by
the next engine-owning audio/state path: controller ranges/assignments,
pitch-bend range/step, master tuning, portamento mode, glissando and
portamento time. Portamento time is a bounded three-byte serial command; unlike
POLY/MONO it neither drains the serial path nor resets voices, so committing it
at the next engine-owning path preserves its firmware order without making the
UI wait for `engineMutex_`. The editor receives the requested packed display
value immediately; a project save commits it before RAM capture. A held-lock
test proves these setters finish without acquiring `engineMutex_`; a concurrent
1,000-write/audio-callback test confirms zero contention counters and the
final firmware values, including the last portamento-time value. A forced
serial-overflow regression additionally proves that the latest requested time
is retained and retried after firmware recovery. The callback still performs no
ordinary C++ allocation in this path.

POLY/MONO deliberately remains a direct firmware transaction. It drains serial
work and ends active notes, so moving it blindly to the audio callback would
trade a controlled transaction for a long callback. It remains in the real
GUI/host overlap acceptance scope.
This chapter does not declare audio continuity accepted: host underruns,
automation behavior, mode-change reset audibility and physical DAW timing still
need separate checks.

GUI layout, firmware routing, parameter IDs, state schema and installed plugin
are unchanged. No release/tag or firmware upload.

Local validation for this slice: the stress and processor test binaries build,
the ROM-backed stress executable passes, and the nine non-GUI CTest entries
pass (60.87 s). In the current headless shell the existing SAVE AS dialog test
cannot obtain a desktop display, so its broader processor CTest remains a
host/desktop check rather than a failure attributed to this change. The
existing Xcode license warning remains, with no automatic license acceptance.
Windows/Universal CI is separate.
