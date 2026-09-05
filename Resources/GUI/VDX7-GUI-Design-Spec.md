# VDX7 Mk I GUI Design Spec v1.2

## 1. Vizuális karakter

A felület egy sötét, professzionális, késő-1980-as éveket idéző digitális hangszer és egy modern plug-in közös nyelvét használja. Nem retro másolat: a történeti utalást a blokkos hardver-logika, az LCD, a keskeny feliratok és a fizikai vezérlők adják; a kortárs minőséget az egységes térköz, a tiszta állapotjelzés és a visszafogott anyagkezelés biztosítja.

Fő szabályok:

- az alapsík közel fekete, enyhén kékeszöld antracit;
- a fémes részletek finomak, karc és erős zaj nélkül;
- a cián kizárólag identitás, fókusz, kiválasztás és érték-visszajelzés;
- a zöld LCD külön anyag és külön információs réteg;
- a piros/sárga/zöld kizárólag a kimeneti LED VU méréshez tartozik;
- minden díszítésnek funkcionális oka legyen; a glow rövid és kontrollált;
- a Yamaha név és embléma nem használható.

## 2. Referencia és koordinátarendszer

Eredeti referencia: `1447 × 1087 px`. A javasolt implementációs vászon `1440 × 1080` logikai pixel. Az alábbi értékek a referencia-képből leolvasott, ±2 px toleranciájú irányadó zónák.

| Zóna | Referencia téglalap, x/y/w/h | Funkció |
|---|---:|---|
| Külső chassis | 24 / 97 / 1400 / 925 | teljes plug-in test |
| Header | 38 / 109 / 1372 / 101 | branding, preset, globális menü |
| Voice + LCD | 50 / 216 / 909 / 178 | voice módok, patch kijelző, algoritmus rajz |
| Algorithm | 968 / 216 / 206 / 178 | algoritmus és feedback |
| Output | 1187 / 216 / 211 / 350 | master fader és két VU |
| Global | 50 / 404 / 474 / 162 | volume, transpose, tune, poly/mono, portamento |
| LFO | 532 / 404 / 340 / 162 | speed, delay, PMD, AMD |
| Pitch EG | 880 / 404 / 294 / 162 | R1–R4 |
| Operator tab-sor | 51 / 575 / 1344 / 47 | OP1–OP6 és alnézet-tabok |
| Operator editor | 51 / 628 / 1346 / 196 | paraméter-knobok, envelope, level |
| Wheels | 51 / 833 / 154 / 137 | pitch és modulation |
| Keyboard | 215 / 836 / 1180 / 133 | képernyő-billentyűzet |
| Footer | 38 / 976 / 1372 / 33 | verzió, termékleírás, CPU/voices |

Arányrendszer:

- külső margó: 24–38 px;
- fő szekcióköz: 8–12 px;
- panel belső padding: 14–18 px;
- cím és elválasztó köz: 8 px;
- kontrollcsoportok ritmusa: 70–84 px;
- sarkok: külső 14–18 px, panelek 6–8 px, gombok 3–5 px.

## 3. Színrendszer

| Token | HEX | Használat |
|---|---|---|
| `surface-950` | `#05090B` | külső fekete mélység |
| `surface-900` | `#071014` | panelbelső, value mező |
| `surface-850` | `#0B181B` | header/footer és másodlagos sík |
| `surface-800` | `#0F1C1E` | fő felület |
| `steel-500` | `#52707A` | keret, elválasztó, skála |
| `text-primary` | `#E6EEF0` | fő felirat |
| `text-secondary` | `#91A5AC` | segéd- és státuszszöveg |
| `accent-cyan` | `#00E7E7` | active/focus/value |
| `accent-cyan-soft` | `#00AEB7` | visszafogott hover/perem |
| `lcd-mint` | `#40D0A0` | LCD alap |
| `lcd-ink` | `#06352E` | LCD karakter/grafika |
| `vu-green` | `#10EF22` | normál jelszint |
| `vu-yellow` | `#FFB000` | magas jelszint |
| `vu-red` | `#F3211B` | peak/clipping tartomány |
| `key-ivory` | `#D7D3CB` | fehér billentyű |

Glow-szabály: 1× nézetben legfeljebb 4–8 px vizuális kiterjedés, 15–35% fedettség. A LED és az active tab lehet erősebb; normál állapot nem világít.

## 4. Tipográfia és feliratok

Javasolt UI-betű: `Barlow Condensed SemiBold` vagy metrikailag hasonló, keskeny groteszk. LCD és számérték: `IBM Plex Mono Medium` vagy `Roboto Mono Medium`. A feliratokat JUCE renderelje, ne a PNG.

- szekciócím: 20–24 px, uppercase, `text-primary`, enyhe pozitív tracking;
- kontrollcímke: 13–16 px, uppercase, középre zárt;
- gomb/tab: 13–15 px, uppercase; active állapotban sötét felirat a cián alapon;
- élő érték: 18–24 px, mono, `accent-cyan`;
- footer: 11–15 px, másodlagos szín;
- LCD fő sor: 22–28 px mono; másodlagos sor: 14–18 px.

Rögzített feliratkészlet:

- header: `VDX7`, `Mk I.`, `HARDWARE EMULATION`, `Original firmware required`, `LOAD`, `SAVE`, `SETTINGS`, `ABOUT`;
- voice: `VOICE`, `EDIT`, `PERFORMANCE`, `UTILITY`;
- algorithm: `ALGORITHM`, `FEEDBACK`;
- global: `GLOBAL`, `VOLUME`, `TRANSPOSE`, `TUNE`, `POLY`, `MONO`, `PORTAMENTO`, `TIME`;
- LFO: `LFO`, `SPEED`, `DELAY`, `PMD`, `AMD`;
- pitch: `PITCH EG`, `R1`, `R2`, `R3`, `R4`;
- output: `OUTPUT`, `LEFT`, `RIGHT`;
- operator választó: `OP1`…`OP6`;
- operator nézet: `ENVELOPE`, `KEY SCALING`, `MODULATION`, `OUTPUT`;
- operator paraméterek: `OPERATOR 1`…`OPERATOR 6`, `OUTPUT LEVEL`, `COARSE`, `FINE`, `DETUNE`, `KEY SCALE`, `VEL SENS`, `AMP MOD SENS`, `LEVEL`, `L1`…`L4`;
- performance: `PITCH`, `MOD`, `+2`, `0`, `-2`;
- footer: `VDX7 Mk I`, verzió, `6-OPERATOR FM SYNTHESIZER`, CPU és voice számláló.

## 5. Szekciók és komponensek

### Header

Balra a csíkozott cián `VDX7` szójel, mellette világos `Mk I.`. A termékleírás kisebb, két soros tömb. Középen a preset-léptetés: bal nyíl, széles sötét preset mező, jobb nyíl. Jobbra négy azonos menügomb. A header nem kap folyamatos cián fényt; az aktív preset és a hover használhat vékony kiemelést.

### Voice/LCD

A `VOICE` panel felső címsort és vékony vízszintes elválasztót használ. A három módgomb közül egyszerre egy active. Az LCD zöld üvegét a `display/lcd-frame.png` adja; minden karakter, kurzor és algoritmus-ábra dinamikus. Az LCD-n legfeljebb két hangsúlyszint legyen: fő presetnév és kisebb paramétersor.

### Algorithm

Az algoritmusszám léptetőgombok között jelenik meg. A hat operator-node a PNG node-elemekből vagy azonos geometriával rajzolható; az összekötővonalak 1–2 px cián vonalak. A feedback knob a 64 px családot használja.

### Global, LFO, Pitch EG

Azonos magasságú, közös baseline-ra igazított panelek. A knobok alatt külön value-field található. A LFO sorrendje rögzített: `SPEED`, `DELAY`, `PMD`, `AMD`; mind a négy tekerőnek teljesnek és azonos méretűnek kell lennie. A `POLY/MONO/PORTAMENTO` blokk függőleges, az active mód cián.

### Output és LED VU

A master fader középen, két oldalán 12–14 LED szegmensből épített oszlop. Alul a zöld tartomány, feljebb két sárga szegmens, legfelül egy piros peak. Az `LEFT` és `RIGHT` felirat az oszlopok fölött van. A `0.0 dB` value-field a panel alján középre zárt.

### Operator editor

Az OP-tabok a teljes szélesség bal részét foglalják, a nézet-tabok jobbra csoportosulnak. Az active operator és active nézet ugyanazt a cián állapotot használja. A fő paraméter-knobok 64 px-esek; az `OUTPUT LEVEL` hangsúlyosabb lehet. Az envelope rács külön PNG, a görbe és a pontok dinamikusak. A négy level fader kis méretű változat; a skála és címkék külön rajzolandók.

### Keyboard és wheels

A billentyűzet a fehér és fekete key assetek ismétléséből épül. Note-on állapotban a megfelelő `pressed` PNG jelenik meg. A pitch és mod wheel ugyanazt a base assetet használhatja, de eltérő markerrel:

- `PITCH`: cián marker, bipoláris mozgás, nyugalmi állapot középen; elengedéskor visszatér középre;
- `MOD`: ugyanaz a cián marker (`#00E7E7`), unipoláris mozgás; minimum alul, maximum felül; nem tér vissza automatikusan;
- a markerek külön overlayek, és a ridgelt roller felületén belül mozognak;
- görgetés vagy drag közben a marker Y-pozíciója folyamatosan kövesse az értéket, ne csak az interakció végén frissüljön.

A pitch skálája `+2 / 0 / -2`, a mod wheel címkéje egyszerű `MOD`.

### Footer

Alacsony, sötét sáv, finom felső elválasztóval. Balra terméknév és verzió, középen ritkított `6-OPERATOR FM SYNTHESIZER`, jobbra CPU és VOICES. A runtime adatok mono betűvel, visszafogott fényerővel jelenjenek meg.

## 6. Interakciós állapotok

| Állapot | Megjelenés | Mozgás |
|---|---|---|
| `normal` | sötét, neutrális, alacsony kontraszt | nincs |
| `hover` | +8–12% fényesség, vékony cián perem | nincs |
| `pressed` | sötétebb belső sík, csökkentett árnyék | 1–2 px lefelé/befelé |
| `active` | cián felület/perem, erősebb kontraszt | nincs |

Knobnál az `active` fókuszt jelent, nem a paraméter értékét. Az érték a külön pointer/arc overlayből olvasható. LED-nél az állapotok nem hoverek, hanem `off/green/yellow/red` energiaszintek.

## 7. JUCE megvalósítási szabályok

- A `LookAndFeel_V4` leszármazott saját `drawRotarySlider`, `drawButtonBackground`, `drawTabButton`, `drawLinearSlider` és `drawLabel` implementációt kapjon.
- PNG-ket `juce::ImageCache` vagy BinaryData használatával töltsd be; ne dekódold újra minden paint során.
- HiDPI esetén a 2× assetet töltsd, de logikai méretben rajzold.
- A knob szögtartománya `-135°…+135°`; az arc ugyanebből a normalizált értékből számolódjon.
- A panel `9-slice` insets: 18 px 1×, 36 px 2×. Az LCD insets: 26 px 1×, 52 px 2×.
- A tabok és gombok szövege optikailag középre, pressed állapotban +1 px Y eltolással kerüljön.
- A modulation wheel marker pozíciója: `y = bottom - normalizedValue * travel`. A marker útját a wheel belső ridgelt tartományára kell clampelni; javasolt 8 px felső és alsó belső margó.
- Minimum kontraszt: fő szöveg legalább 4.5:1; másodlagos szöveg legalább 3:1 a saját hátterén.
- A referencia 1447×1087 képarányát arányosan lehet 1440×1080 implementációs vászonra igazítani; önálló panelek újratördelhetők, de a zónák sorrendje nem változhat.
