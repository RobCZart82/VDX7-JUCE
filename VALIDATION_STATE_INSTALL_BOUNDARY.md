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
