# Stopped prepare mode — local checkpoint 2026-10-11

Stacked on #179 head `173e7a831ce128864635c6b8213d2177a7e9c658`.
Branch: `feature/sound-mode-stopped-prepare`. Not merged acceptance.

Host-stopped prepare installs current ready project intent using the existing
engine lock and quiescent engine API, after MIDI lifecycle cleanup. Pending
projects are excluded by the owner gate. No Clean reader/writer/UI enabled.

PASS: targeted Release processor rebuild and private original v1.8 snapshot
suite. Added pending intent exclusion and ready Clean cold install, with
unchanged project revision. Test-only Clean owner injection is not supported
Clean project admission. Initial comparison incorrectly expected lifecycle
cleanup not to advance/consume an existing pending native scan install; the
final predicate checks that pending Clean intent cannot relabel Classic.

Remaining: final-test sanitizer rebuild/run, complete CI checks, platform/review,
and independent main verification after #179. Reload/reset/release and public
Clean project acceptance remain open. No release or installed plugin changed.

## Verified follow-up

Updated to main `9c0174cb08fcd051f273b79872e74a7bb8b0552f` after #179;
its separate Windows/macOS checks passed. Fresh Release ci_checks build and
targeted ASan/UBSan build completed. Final private original v1.8 snapshot
suite PASS under ASan/UBSan (`detect_leaks=0:halt_on_error=1`,
`halt_on_error=1:print_stacktrace=1`). This is not leak/TSAN acceptance.
The earlier remaining-check list is the preceding checkpoint, not current
sanitizer status. Final-head public CI/review and main checks remain required.

Final local Release private snapshot suite, 22/22 ROM-free CTest and 94/94
Python tests PASS. Whitespace check PASS. No real host or new package tested.
