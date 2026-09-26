# State installation timeline — 2026-09-24

Audit B1 first schedule reproduced deterministically: publish a restore request,
run the real callback under engine-lock contention with a stale program change
and Note62, then call the real locked restoration routine. Before the fix the
fixture exits1: `pre-install MIDI replayed into restored project`.

This fixture uses friend access to stage announcement/installation separately;
it is not a timed race through public setStateInformation/APVTS or a DAW test.

The installation routine now publishes a second epoch after completing RAM,
edits and snapshot installation under the engine lock. The callback observes
epochs both before collecting input and immediately after acquiring that lock.
If an install straddles those observations, the collected old block is discarded.
Audio-owned queues are never cleared by the state thread. Early announcement
remains for invalidating pre-request input during preparation.

Regression checks stale program/held-note rejection and subsequent Note72 audio
and adapter release. Existing deferred CC floods, partition/expiry and corrected
MONO persistence/lifecycle/pitch controls remain in the targeted run.

Still open: deterministic scheduling specifically between the callback's two
observations, concurrent public state calls, full GUI/DAW and sanitizer tests.
The first proven schedule must not be presented as exhaustive race coverage.

Rebuilt host regression executable: targeted CTest profile, deferred processor
and corrected MONO groups **3/3 PASS**, exit0. Native Note0 acceptance was not
included and remains open. No full-suite or installed-plugin validation.

## Public restore follow-up

Added two fixtures through public setStateInformation, including its actual
parsing, parameter-state and saved-ROM restoration path: native POLY and opt-in
corrected MONO, at 48kHz/64 samples. First, holding the engine mutex on the test
thread makes a real asynchronous callback defer a different program change,
sustain-on and Note60 (POLY) or Note0 (corrected MONO). The callback must return
within one second, without ordinary C++ allocation, and input must be deferred.
Then the mutex is released and the public project restore is invoked.

After rendering, the saved program must remain selected, deferred input and
adapter/firmware note/pedal ownership must be empty, and audio silent. Fresh
Note60/Note0 followed by Note72 must sound and release. Corrected MONO's final
active count must be zero. No production code changes in this follow-up.

Rebuilt targeted CTest: **2/2 PASS**, exit0, 10.71s including profile; deferred
group10.04s. Existing staged boundary, bypass and partition checks also run in
this group. This is not a new full-suite run or native MONO Note0 closure.
The public restore starts after the contended callback completes: these tests
do not establish correctness of concurrent public restores, an installation
inside a running callback's pre/post-lock interval, or all DAW schedules.

## Deterministic public installation inside an active callback

The host-reset test executable now recompiles PluginProcessor.cpp with its own
private VDX7_TEST_STATE_BOUNDARY definition. Only that build calls a scheduling
gate after host/keyboard collection and before trying the engine mutex. The
VDX7 shared-code/plugin target has no such definition or gate call. Class layout
and normal production behaviour are unchanged. The test waits deliberately;
its paused callback duration is not a realtime latency result.

Two new 48kHz/64 fixtures (native POLY and corrected MONO) pause the real callback
with old host program/sustain/note input and one collected virtual keyboard key.
An acquire/release handshake proves the collection point has been reached and
the keyboard count is one. The other thread completes public setStateInformation
before allowing the callback to acquire the engine lock. Old events must not
change the restored program, retain firmware/adapter ownership or produce sound.
Fresh Note72 must subsequently sound and release in both modes.

Negative control: temporarily omitted the post-lock discardStaleCollectedInput
call, rebuilt and ran the group. It failed at the new overlap assertion:
`overlapping public restore replayed collected host/keyboard input` (CTest1/2,
exit8). Restored the original call immediately; no mutant code is committed.
Final rebuild and targeted profile/deferred/wheel groups: **3/3 PASS**, exit0,
21.29s. Deferred10.85s; wheel9.92s. No full-suite/DAW run in this round.

This closes the specific pre/post-lock scheduling gap in test coverage, not
all concurrency questions. Multiple simultaneous public restore calls, broader
host schedules and sanitizer coverage remain separate work. The native MONO
Note0 issue remains open; private ROM and installed plugin are unchanged.
