# Licensing and attribution / Licenc és köszönet

VDX7-JUCE 1.2.0 stable, development and release-candidate sources use the free-software
combined-work route under GNU AGPL version 3, without warranty. The project
owner authorised the AGPLv3 publication route. Wrapper contributions and
original GUI resources are offered under AGPL-3.0-only; see LICENSE.txt.
Development artifacts are not accepted stable releases; this update does not
change upstream license terms.

Current 1.2.0 source packages use their exact build label: stable `1.2.0`,
development `1.2.0-dev` or candidate `1.2.0-rcN`. Stable v1.2.0 publication
was explicitly authorized and completed. This does not authorize unrelated
development/candidate artifacts or make them accepted stable releases.

Existing upstream components retain their own notices and licenses:

- VDX7 DX7 core: Copyright (C) 2023 chiaccona@gmail.com, GPL-3.0-or-later;
  adapted for Retromulator integration. GPL text: LICENSES/GPL-3.0.txt.
  The GPLv3 section 13 / AGPLv3 section 13 combination provisions apply;
  this does not relicense the upstream core in isolation.
- JUCE framework: Raw Material Software Limited and contributors, AGPLv3
  for this release. Its LICENSE.md lists bundled third-party components
  under their respective licenses; those files and notices are retained in
  the corresponding-source archive.
- VST3 SDK and other JUCE dependencies retain the licenses stated in their
  included source files. No commercial JUCE license is claimed.

Published stable v1.2.0 includes matching complete source and checksum under
`Source/` in both public Manual ZIPs. Source/tag:
`7e2271352d0f05fdad39bae26fdec764064fa1e1`; publication 2026-10-14.
Development/RC builds are not yet approved or authorized for stable publication
merely by being built.

The published 1.0.1 release used a separate v1.0.1-source GitHub release linked
from the main release notes; its four binary user downloads remain separate.
For each current 1.2.0 development candidate, prepare matching complete source
as `VDX7-<version-label>-<full-source-SHA>-corresponding-source.zip`, using the
exact `1.2.0-dev` or `1.2.0-rcN` label used by that build. Its embedded
`SOURCE_MANIFEST.json` identifies the exact source and dependency revisions and
hashes the payload files; `SHA256SUMS.txt` hashes the ZIP. A future stable
publication must not proceed without durable public access to matching source
and separate stable-release approval.
The source package includes the wrapper, GUI resources, build scripts, exact JUCE sources
and the portable dx7Lib source subset used by the build. See
docs/guides/SOURCE_DEPENDENCIES.md for pinned revisions and offline build instructions.

Source: https://github.com/RobCZart82/VDX7-JUCE

No Yamaha firmware, factory voice ROM, Yamaha logo or reference photographs
are included. Yamaha/DX7 names describe compatibility, not endorsement.
External firmware is supplied separately by the user and is not licensed
by this project.

Magyar összefoglaló: a jelenlegi 1.2.0 fejlesztési és kiadásjelölt-források
AGPLv3 szerinti szabad szoftverek, garancia nélkül. A címke `1.2.0-dev` vagy
`1.2.0-rcN`; ezek nem automatikusan elfogadott stabil kiadások. A stabil 1.2.0
megjelent tulajdonosi elfogadással és engedéllyel 2026-10-14-én; teljes
megfelelő forrása és checksumja mindkét Manual ZIP `Source/` mappájában elérhető.
A felhasznált komponensek eredeti licence és szerzői közlései megmaradnak.
A DX7-mag önállóan GPLv3-or-later, a JUCE ebben a kiadásban AGPLv3.
A már publikált 1.0.1 külön forrásarchívumot és négy külön bináris letöltést
használ. Egy jövőbeli stabil kiadáshoz a megfelelő teljes forrásarchívumot is
nyilvánosan elérhetővé kell tenni.
Yamaha firmware és gyári ROM-adat nem része a csomagnak.
