# MIDI capacity and pending-off overflow

Base: main edb66e2cdbfcac23969dd65d9d737e709e844dc3 (PR30).

Eight local-firmware cases: POLY/MONO x 32 distinct/repeated notes x normal
all-notes-off/serial overflow. Each note is rendered before the next, allowing
real firmware allocation rather than just filling a wrapper queue. Host channels
cycle 1..16 under the existing single-part policy. A sustained carrier patch
prevents natural envelope decay from hiding a stuck voice.

After sustain is engaged, all offs are queued without rendering. A burst of
3000 pitch-wheel messages then flushes the serial queue via overflow recovery.
The old code failed the fresh-note audibility check in MONO/repeated/overflow:
silence alone did not establish a healthy recovered instrument.

Root cause: active-note counters already decremented for offs not yet consumed
by firmware. Flushing those offs loses the release multiplicity. A separate
per-pitch high-water release budget now retains maximum tracked multiplicity
since ROM load, capped at 16. Overflow/lifecycle cleanup uses this budget, not
the already-decremented active count. Normal note dispatch remains unchanged.
Memory is fixed (128 bytes); no new allocation or lock. Worst-case cleanup is
bounded at 128*16*3 serial bytes, fitting the existing 8192-byte RX buffer.
The budget is deliberately conservative and may issue extra harmless offs;
it is not firmware voice allocation state and is not persisted in project data.

Assertions cover finite and audible held audio, exactly one induced overload,
silence and cleared recovery/ownership afterwards, then an audible fresh note
and its successful release. Existing stress coverage still exercises twelve
rate/buffer configurations, allocation probes, two-instance isolation and
partition invariance. These offline checks are not realtime deadline guarantees
or real-host/platform acceptance. Engine-mutex contention remains separate work.

No GUI, parameter IDs, state format, installed plugin, tag or release changes.
User firmware remains local; no firmware or factory ROM is committed.

Final local arm64 VST3/all-tests build passed. Full CTest: 10/10 passed in
71.01 seconds; expanded stress runner passed in 11.84 seconds. Manual ad-hoc
signing and strict verification passed. Existing Xcode license warning remains;
no license accepted automatically. Windows/Universal CI remains a separate gate.
