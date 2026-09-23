# Reset release batch: running-status encoding

Follow-up on the unmerged #47 branch. No main merge or installed-plugin change.

## Mechanism

Host reset sends a contiguous batch of Note Offs on one receive channel. Use one
explicit MIDI Note Off status followed by the note/zero-velocity pairs (running
status), instead of repeating the identical status for every release. Preserve
every pitch and repetition, including the conservative lifetime budget. The
maximum batch becomes 4097 rather than 6144 bytes. Reserve the whole batch before
writing, and keep the existing sustain/portamento cleanup. No budget retirement,
firmware RAM edits, increased callback sample budget, or changed two-second
deferred age limit. Ordinary allNotesOff and overload recovery keep their prior
full-status encoding; only beginHostReset opts into running status.

## Evidence and limits

Before: the production-path 128-pitch x16 history case at 48 kHz/64 took 2092 ms
of audio timeline to finish reset and lost fresh input at the age limit (#46).
After: the same existing assertions passed; completion was by 1510.67 ms and
first audible block 1169 (1558.67 ms), including subsequent shifted Note Off and
persistent-state comparison. This is mitigation of note loss, NOT a claim of
acceptable final musical response time or a universal latency bound.

Additional tests exercise ordinary versus running-status Note Off batches with
one and sixteen repeated notes in the real local firmware. They require audio
to stop WITHOUT a host reset, output mute gate or EGS reconstruction, so reset
silencing cannot mask a firmware parser that ignored the compressed releases.

Expanded-history acceptance is extended to 44.1/48/96 kHz and 64/128/256/512
samples. Assertions for no setup overload, exact lifetime history, fresh-note
survival, release and unchanged persistent settings are retained. The original
no-ROM, nonzero-L4, repeated/zero-block reset, contention and reactivation tests
are retained. All integration targets build locally on macOS arm64 Release.

Full local-ROM suite: **12/12 PASS in 145.49 s**, including the previously
failing history case. The expanded reset executable passed in 68.02 s; all four
independent firmware serial-release cases passed. Across the twelve maximum
history cases reset completion was 1510.67–1520.91 ms (block-end timeline).
At 48 kHz/64 the one-repeat history completed by 96 ms (previously 132 ms).

Local arm64 VST3 compiled. The build helper emitted Xcode-license warnings and
its resulting bundle initially failed strict signature verification. The build
artifact only was re-signed ad hoc with the system codesign tool; subsequent
`codesign --verify --deep --strict` passed. This is not notarization, a clean
release-environment certification, or validation of the installed VST3.

## Still open

Real VST3-host reset, acceptable worst-case responsiveness, reset overlapping
state/ROM replacement, nonzero-L4 unmute/overflow interaction, Windows ROM runtime,
sanitizers and callback-cost profiling. CI compiles the reset tests but its
ROM-free green result is not a firmware-runtime test. No release acceptance is
claimed by this change. Historical FAIL records remain in prior validation files.
