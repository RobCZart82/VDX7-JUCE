# Pitch/modulation delivery — 2026-09-24

New registered local-ROM test `vdx7_wheel_delivery` reproduces audit B3 through
the real processor control application and callback rendering. Friend access
stages recovery before submission or immediately after submission (before any
render); it is a deterministic recovery test, not a naturally saturated host.
Fixture directly changes the raw wheel parameter values, not the GUI callback.

Before fix: normal pitch/mod input127/95 PASS; recovering-at-submission64/0 FAIL;
queued-then-flushed64/0 FAIL. Reads actual firmware input RAM after processing,
not processor display caches. CTest1/2 (profile passes), exit8.

Engine now owns the latest accepted pitch/mod intent independently of bounded
message storage. GUI cache updates mean intent accepted, not firmware delivery.
Normal FIFO delivery remains; recovery marks intent pending for retry after
recovery/reset and queued sub-CPU messages drain. No firmware RAM or pitch
rewriting. Fresh accepted physical MIDI changes the same intent, so recovery
must not replay an older unchanged GUI value over it. ROM load clears intent.

An initial implementation replaced normal FIFO wheel traffic with pending-only
delivery. Existing portamento overload test caught the changed overload
behaviour (`portamento enters recovery`). Corrected before final validation:
ordinary reservation/enqueue preserved, detached intent is fallback only.

Each of the three schedules also sends newer physical pitch/mod32/19, checks
firmware inputs, triggers another recovery, and requires32/19 to remain. Normal
callbacks keep finite-audio and ordinary C++ allocation checks. This measures
input delivery, not an independent audible pitch/modulation oracle.

Remaining: full-suite/DAW, sample-offset wheel ordering under mixed saturation,
live GUI callbacks and host lifecycle combinations. Native MONO Note0 remains
open independently. Private ROM and installed plugin unchanged.

Final rebuilt targeted run: **5/6 PASS**, exit8. Wheel delivery, portamento,
deferred-input and corrected MONO groups plus profile all pass. The sole failure
is processor GUI's desktop/display prerequisite; it is not counted as passed.
The final header comment only documents API semantics and changes no behaviour.
