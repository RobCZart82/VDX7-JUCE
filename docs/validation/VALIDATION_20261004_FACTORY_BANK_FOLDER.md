# Factory bank folder and project sound recall

The owner approved content-based recognition of user-supplied ROM1A–ROM4B
SysEx banks after checking last-edited voice recall. This development starts
from main d12bc98, after the legacy-bank fix in PR 123. No Yamaha firmware,
factory voice payload, generated binary or private test file is committed.

## Data and selection rules

The scanner first validates one complete 4104-byte VMEM message, including
checksum and all packed voices. It compares the complete unchanged 4096-byte
payload with eight reference SHA-256 fingerprints. Filenames, MIDI channel
and individual voice names are not identity evidence. These fingerprints
identify reference dumps, not Yamaha provenance or redistribution rights.
Synthetic tests inject separate reference hashes; production never accepts
test hashes or user-configured identities.

The folder is non-recursive, with at most 128 SysEx files read and at most
4104 bytes read per file. Invalid, oversized, truncated, single-voice,
checksum-damaged and unknown files do not enable a factory slot. Identical
duplicates share one slot. Unknown valid banks can still use LOAD SYX as
CUSTOM. Input files are never rewritten. Only present slots are selectable;
unavailable MIDI bank requests are rejected before deferred input storage
and again at the engine boundary.

A non-directory path or scan-limit overflow is an incomplete scan: it does not
replace a live catalog, and first-load folder overlay is skipped. Individual
invalid/unknown files are reported and ignored within an otherwise complete scan.

SETTINGS adds Bank folder and Refresh banks without moving main GUI controls.
New instances scan on firmware load. Explicit refresh replaces the catalog
using the folder over the original valid ROM/companion banks, without replacing
working RAM. Prior edits are committed before catalog changes. Dirty flags
remain. If the selected catalog entry disappears/changes, its current working
sound stays intact with a CUSTOM marker. Other open instances refresh explicitly.

## Project preservation

The existing project-save path applies outstanding host/operator/voice edits
before capturing RAM, even without another audio block. The new state stores
the available catalog and eight-bit presence mask in addition to that RAM.
No firmware is embedded. Before first firmware load, no unknown empty catalog
is frozen. Missing-ROM projects retain their existing pending snapshot.

A complete validated project catalog takes precedence over the current local
folder. The final edited RAM takes precedence over the factory entry during
recall. Partial presence survives firmware reboot, project RAM restore and
MONO correction changes. Existing ROM identity checks retain their original
combined/companion semantics; the new folder does not change ROM identity.
Old projects without catalog fields retain the old fallback behavior.

Malformed catalog fields, wrong sizes, unsupported occupied voice values and
nonzero absent slots are rejected before loaded or pending state is replaced.
Catalogs from valid legacy combined images need not match the new folder's
reference fingerprints. This preserves legacy compatibility without using
arbitrary files as new factory identities.

Final self-review identified that JUCE's MemoryBlock decoder allocates from an
untrusted decimal prefix before checking decoded size. Both catalog and existing
project RAM decoding now require the exact fixed-size prefix and encoded length
before allocation, and canonical encoding before accepting bytes. Negative/huge
prefixes, truncated text and noncanonical payloads are rejected transactionally.
The existing writer's project format is unchanged.

## Test evidence and limits

Before implementing the folder, the existing processor regression passed with
the private v1.8 ROM and desktop access. It checks every operator/global voice
field, save before another render and persistent RAM/host-parameter recall.
The first sandbox attempt reached the GUI test and failed for lack of desktop
access; the authorized desktop rerun passed. This is not a product failure.

New public synthetic tests cover exact/renamed/misnamed identities, partial and
complete catalogs, duplicates, channel-independent payloads, checksum/size/
semantic rejection, unchanged source bytes, non-recursion, scan limits, XML
round trips, malformed-state non-mutation and partial-library MIDI filtering.
ROM-free processor tests also cover missing-ROM catalog preservation and
malformed-state rejection. No Yamaha payload is required for public CI.

The optional local Settings preview passed child-bound checks; visual review
confirmed both bank buttons and all previous settings fit without clipping.
The temporary test dialog was cancelled without applying settings or touching
installed plug-ins. The preview is not a Windows visual/host acceptance test.

The private local processor probe recognised all eight owner-supplied files.
With firmware alone and a renamed ROM2A file it enabled only ROM2A, tested
automatic first-load selection, latest edits/name without another render,
refresh without sound/dirty-flag loss, changed/absent folder recall, missing-ROM
resave/reopen/recovery, MONO correction reboot and CUSTOM patch recall. A fresh
valid legacy combined-image load also used the overlaid reference bank in working
RAM, rather than the superseded legacy bytes. Release and ASan/UBSan variants passed.
Private data remained outside the repository and was not installed into the
owner's plug-in directories. Isolated temporary fixtures were removed.

Local required ROM-free ASan/UBSan components: 9/9 PASS. Leak detection is
disabled on this macOS configuration. Python suite: 77/77 PASS. Final normal
CTest regression before the final allocation hardening: 45/45 PASS in 430.21 seconds (14 ROM-free and 31 private-ROM
tests, including the ROM fixture). Complete registration/labels/timeouts/fixture
checker passed. The tests were rebuilt from the then-current product source before this
run; earlier intermediate test passes are not substituted. Development compile
checks caught and corrected JUCE API mismatches. A stale intermediate run was
interrupted after a failed rebuild and is not counted as final evidence.

After allocation hardening, factory-bank and pre-ROM state tests passed again;
the private bank-folder processor probe passed in normal and ASan/UBSan builds,
and the required ASan/UBSan component set passed 9/9. A complete rebuilt normal
regression is rerun; its final result/head is a required gate recorded in PR 124,
not inferred from the earlier full-suite result.

GitHub Windows/macOS/sanitizer PR results and post-merge main results remain
separate gates at source-commit time; record their exact run/head identities in
the pull request before merging. Main d12bc98 platform runs 37217270272 and
37217270279 passed before this new development round.

The owner has not yet tested this new feature in real Windows/macOS REAPER.
No new installer/source packaging, final acceptance tuple, tag, release or
asset publication is asserted. The previous 1.0.1 candidate does not contain
this feature. After green PR/review and post-merge main checks, rebuild and
independently verify exact final packages/source, then give the owner the new
packages for last-edited patch recall and bank-folder host acceptance.
