# PERFORMANCE display without engine locking

Base: main 85b0c90f33a17e97bd7913d3414fc238d9ba651f (PR32).

The panel previously took engineMutex_ three times on refresh to read controller,
play and bend settings. It now reads one atomic 64-bit frame. Encoding uses
58 bits: four 7-bit ranges + twelve assignment flags, three play flags + 7-bit
portamento time, two 4-bit bend settings. A compile-time assertion requires
lock-free uint64 atomics. Fields are coherent without reader loops or GUI locks.

Publish under existing engine ownership after each successful audio block,
engine snapshot updates (load/restore/import), and successful PERFORMANCE UI
setters. Thus firmware-delayed MIDI changes become visible after rendering,
and stopped-host UI edits do not wait for an audio callback. The display starts
at zero with no ROM. Exact locked getters remain for non-display use; no state
format or parameter IDs change.

Regression compares every display group against engine-backed getters, tests
range endpoints and assignment combinations, bend endpoints, portamento time,
state restore, and firmware-driven MIDI mono/poly updates. Ten thousand reads
must finish while another thread holds engineMutex_; timeout is a deadlock guard,
not a realtime performance target. Audio allocation probes remain enabled.

Scope: removes engine locking from routine PERFORMANCE display refresh only.
Setters, voice publication and ROM/state transactions still use engineMutex_.
No claim that all audio dropouts are fixed or host continuity is accepted.
GUI positions/artwork, installed plugin, firmware routing, tags/releases unchanged.

Local arm64 VST3/all-test build and strict ad-hoc signature verification passed.
Full CTest 10/10 passed in 69.76s, including the new display checks. Existing
Xcode license warning remains; no license accepted automatically. Windows and
Universal CI remain separate checks. Firmware remains local, never committed.
