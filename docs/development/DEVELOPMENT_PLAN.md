# VDX7 egységes fejlesztési terv / Current development plan

Frissítve / Updated: 2026-10-14. Egyetlen irányadó aktív terv / Sole active plan.

**Publikálás TELJESÜLT:** a v1.2.0 megjelent; részletek alább.

## Aktuális kiadási állapot / Current release status

**Publikálás TELJESÜLT:** [v1.2.0 stable Latest](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.2.0),
2026-10-14. Published source: `7e2271352d0f05fdad39bae26fdec764064fa1e1`;
package run: 38548177165. Four unchanged VST3 downloads; matching source inside
both Manual ZIPs. Documentation changes do not rebuild or replace releases.

Végleges csomagellenőrzés PASS, Windows/macOS teszt OWNER-REPORTED PASS,
kifejezett publikálási engedély. Nem közölt részletes műszeres teszt nem PASS.
Final packages verified; explicit owner-reported acceptance and authorization.
Unreported exhaustive measurements are not claimed PASS.

- [Final acceptance / Végleges elfogadás](../validation/FINAL_120_OWNER_ACCEPTANCE_20261014.md)
- [Download policy / Letöltések](../release/PUBLIC_DOWNLOADS.md)
- [English guide](../guides/GUIDE_EN.md) / [Magyar kézikönyv](../guides/GUIDE_HU.md)

## Karbantartás / Maintenance

1. **Dokumentációs audit mainben (#200).** Bilingual current navigation and
   UTF-8 consistency tests integrated. A dokumentációs javítás beolvadt.
2. **Csomagolási fail-fast javítás mainben (#201).** Single configure and
   executable failure regression integrated, not a new package-production run.
   A kiadott binárisokat nem módosítja.
3. **Tervrendezés: review/CI még szükséges.** This consolidation preserves
   history; its own final-head review/CI must pass before merge. E rendezés
   még nem beolvasztott változás, saját review/CI után zárható le.

No new feature/version target is assigned. Historical open ideas remain
preserved, not automatically completed or restarted; compare them with current
code/evidence before any new scope decision. Új funkció/verzió külön döntés;
a régi nyitott ötleteket nem töröljük és nem jelöljük automatikusan késznek.

## Állandó korlátok / Standing boundaries

- Original Yamaha DX7 Mk I v1.8 only; firmware not distributed.
  Kizárólag original v1.8 támogatott, ROM nem terjeszthető.
- Fresh main/head, relevant regression and CI, review, serial SHA-bound merge,
  then separate green main checks. No admin merge or force push.
- Preserve others' work, 148 parameter IDs/order, project compatibility and RT
  boundaries. No automatic plugin installation or DAW-project modification.
- Future publication requires explicit authority; no release/tag/asset replacement.

## Történet / History

[Complete preserved ledger / Teljes megőrzött napló](../archive/DEVELOPMENT_PLAN_HISTORY_20261014.md).
Old dates, SHA-bound PASS/NOT RUN and D/T/M identifiers remain searchable there,
not a competing roadmap. A történeti állítások nem írják felül a kiadást vagy
ezt az aktív tervet; újraaktiválásuk friss ellenőrzést igényel.
