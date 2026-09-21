# Edit-queue transaction boundary

Base: main a3f3e61b095ac978ffecb8e22e1520d4678d71a2 (PR28).

Confirmed deterministically: pause push immediately after reservation; call
discard; accept a new edit; publish the old edit. Before the fix, pop returns
the old command, failing with "pre-transaction edit escaped discard boundary".
The test seam is a private template accessed by a friend test helper; normal
push supplies an inlined no-op, with no stored callback or allocation.

Discard now snapshots the reservation counter. All earlier reservations are
invalidated even when unpublished. The consumer reclaims them only once their
producer publishes, never overwriting a slot still owned by a producer. Later
reservations survive, in order. Both discard and pop have bounded loops; neither
waits for publication. A saturated queue remains closed until the obsolete
prefix drains, then recovers. Ordinary saturation still requires an explicit
transaction, preserving the existing fail-closed policy after rejected switches.

The cutoff is consumer-owned under the existing engine mutex. Producers do not
take that mutex. Pending/dirty indications can conservatively include obsolete
published edits until reclaimed; such commands never reach the engine. This does
not solve a permanently stalled producer or broader engine-lock audio contention.

ROM-free regression covers all four command types, ready successors behind a
paused producer, post-boundary program/edit order, repeated transactions with
multiple paused producers, delayed saturation recovery, ring reuse and the
existing four concurrent producers. The deterministic interleaving is at queue
level; no claim of reproducing a user-visible DAW failure is made.

No GUI layout, firmware behavior, parameter IDs, state schema, installed plugin,
version promotion, tag or release changes. User firmware stays local.

Validation: local macOS arm64 VST3 and all-test targets built successfully;
CTest 10/10 passed in 58.21 seconds, including processor/state and stress tests.
Manual ad-hoc signing and strict verification passed. The existing Xcode-license
warning during helper execution remains; no license was accepted automatically.
Windows/Universal CI and real-host acceptance are separate checks.
