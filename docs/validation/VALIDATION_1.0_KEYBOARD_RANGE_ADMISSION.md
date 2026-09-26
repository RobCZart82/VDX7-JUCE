# Programmatic keyboard pitch admission

## Finding

The visible piano keyboard spans notes 36-96, but `keyboardState()` is a public
processor API and its listener can receive the full MIDI range. Before this
change, every virtual-keyboard Note On/Off entered a bounded 256-event queue;
product-range filtering happened later in `processBlock`. A flood of unsupported
notes could therefore consume the queue and cause a later supported key event
to be dropped during overflow recovery. This was an identified capacity risk,
not a reported crash or confirmed DAW issue.

## Change

Use the shared Note 12-120 predicate in the keyboard listener before setting UI
ownership or publishing to the queue. The queue exposes a supported-note
admission method as defense in depth; unsupported pitches are inert and do not
set its overflow flag.

## Regression coverage

- ROM-free queue test: 160 out-of-range Note On/Off pairs do not occupy the
  queue or cause overflow; the following supported Note 60 is accepted.
- ROM-backed processor test: floods the public keyboard-state API with
  unsupported low/high pitches in both settings modes, then verifies a supported
  Note 60 reaches the engine and releases.

Local execution: **NOT RUN** — CMake is unavailable in this environment.
The ROM-free queue regression is registered in normal CTest/CI; the processor
route regression remains dependent on a local compatible ROM.
