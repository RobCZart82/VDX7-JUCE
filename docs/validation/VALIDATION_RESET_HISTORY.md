# DRAFT: reset history exceeds deferred MIDI age limit

Historical baseline evidence below. The subsequent running-status mitigation
and passing expanded matrix are in `docs/validation/VALIDATION_RESET_RUNNING_STATUS.md`; this
original failure record is retained, not a statement that the new tree fails.

2026-09-23. Baseline: `659bc0b498f8e48171175b97f1634ba4c6c23909`
(merged #45). This branch adds a failing acceptance regression, not a fix.
Do not merge as a completed reset milestone. ROM-free CI does not run this test.

## Production-path fixture

macOS arm64 Release, Apple clang 21, private local combined ROM, 48000 Hz / 64.
Using public processor MIDI input, visit all 128 pitches with either one or
sixteen overlapping Note Ons, then matching Note Offs. Render four callbacks per
event. Assert that no engine MIDI overload occurred, all notes are released, and
every lifetime release-budget entry reached exactly the requested repeat count.
No private state is modified to manufacture the history.

Request public reset and immediately submit Note On 72 in the next callback.
Observe ten seconds of audio timeline, separately recording reset completion
and first audible block. Require that the note survives and eventually releases
with a single Note Off; persistent settings must remain unchanged. Release
observation allows the measured reset delay plus one second, because the existing
deferred timeline shifts subsequent Note Off by the same delay as Note On.

## Results

| History | Reset completes by block end | First audible block start | Outcome |
| --- | --- | --- | --- |
| 128 pitches x1 | 132 ms | 180 ms | PASS, including release and settings |
| 128 pitches x16 | 2092 ms | none in 10 seconds | FAIL: fresh note lost |

These are audio-timeline observations, not callback wall-clock durations, nor a
claim that every user session builds this worst-case history. The longest case
is 6144 bytes of conservative release traffic (128 x16 x3).

The engine finishes, but the normal two-second deferred-lag guard has already
cleared the immediate post-reset note. This reaches the bounded-loss policy
documented in #44; it is not evidence that its shorter existing tests were false.
It prevents declaring the stronger expanded-history acceptance complete.

## Causal control (NOT SHIPPED)

Temporarily changing only the processor's deferred maximum lag from two to three
seconds made the extended test pass (8.42 s wall-clock suite duration). Reset
completion remained 2092 ms; first audible block was 1605 (2140 ms block start).
The fresh note also released after allowing its shifted Note Off timeline.
The original one-second release observation was too short for that deferred
timeline and was corrected; this is not a second proven lost-Note-Off bug.

The three-second change was reverted. Simply accepting more than two seconds of
musical latency is not a final solution and weakens unrelated contention policy.
No production code, installed plugin, ROM, tag or release is changed in this PR.

## Next implementation gate

Design a reset-specific bounded cleanup/delivery policy which avoids silently
losing fresh input at the normal lag boundary, without unlimited buffering,
unbounded callback work, or blind replay of stale MIDI. Review conservative
release accounting and firmware completion before reducing cleanup traffic.
Keep this regression, add sample-rate/block variants, measure callback cost,
and retain explicit overflow behavior and note-release ordering. Actual VST3
host reset and state/ROM overlap acceptance remain separate open checks.

Reproduce with local ROM tests enabled: build `vdx7_host_reset_tests`, then run
`ctest --test-dir <build> -R ^vdx7_host_reset$ -V`. Expected on this branch: FAIL
at `expanded-history reset lost fresh note`; earlier cases pass. Never upload ROM.
