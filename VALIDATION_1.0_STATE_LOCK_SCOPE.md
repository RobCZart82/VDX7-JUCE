# Reduce state serialization lock scope

Base: main a4fb2d9c810e3839758e3b8b635e411f351a21c5 (PR33).

Save still captures RAM, voice values, selection and dirty flags together under
engineMutex_. Base64 encoding and ValueTree property construction now operate
on those captured values outside the lock. Missing-ROM saves detach a copy of
pending state under the lock, then create XML/binary output after releasing it.
Restore prepares its detached input copy before taking the engine lock.

This removes known serialization work from the critical section; it does not
claim a measured dropout reduction or eliminate the remaining capture/restore,
parameter publication, ROM loading or settings transaction locks. Pending-state
edit capture and deep copying remain protected to preserve coherent state.
The state schema and all parameter IDs remain unchanged. Existing processor and
stability regressions cover save/restore, missing-ROM preservation and reentrant
state capture. Real GUI/audio overlap and host continuity remain acceptance work.

No GUI layout, installed plugin, firmware upload, tag or release changes.

Local arm64 VST3/all-test build passed; full CTest 10/10 passed in 70.27s.
Strict ad-hoc verification passed. Final source differs from tested source only
in indentation/comments/documentation. Existing Xcode license warning remains;
no automatic license acceptance. Windows/Universal CI remains a separate gate.
