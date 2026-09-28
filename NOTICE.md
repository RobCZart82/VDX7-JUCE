# Licensing and attribution / Licenc és köszönet

VDX7-JUCE 1.0.0 development sources use the free-software combined-work route
under GNU AGPL version 3, without warranty. The project owner authorised the
AGPLv3 publication route. Wrapper contributions and original GUI resources
are offered under AGPL-3.0-only; see LICENSE.txt. Development artifacts are not
accepted stable releases; this update does not change upstream license terms.

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

For each development candidate, prepare matching complete source as
VDX7-1.0.0-dev-<full-source-SHA>-corresponding-source.zip. Its embedded
SOURCE_MANIFEST.json identifies the exact source and dependency revisions and
hashes the payload files; SHA256SUMS.txt hashes the ZIP. Publication must not
proceed without the matching source package alongside the binary.
The source package includes the wrapper, GUI resources, build scripts, exact JUCE sources
and the portable dx7Lib source subset used by the build. See
docs/guides/SOURCE_DEPENDENCIES.md for pinned revisions and offline build instructions.

Source: https://github.com/RobCZart82/VDX7-JUCE

No Yamaha firmware, factory voice ROM, Yamaha logo or reference photographs
are included. Yamaha/DX7 names describe compatibility, not endorsement.
External firmware is supplied separately by the user and is not licensed
by this project.

Magyar összefoglaló: a kiadás AGPLv3 szerinti szabad szoftver, garancia nélkül.
A felhasznált komponensek eredeti licence és szerzői közlései megmaradnak.
A DX7-mag önállóan GPLv3-or-later, a JUCE ebben a kiadásban AGPLv3.
A teljes fordításhoz szükséges forrás külön, a bináris mellett tölthető le.
Yamaha firmware és gyári ROM-adat nem része a csomagnak.
