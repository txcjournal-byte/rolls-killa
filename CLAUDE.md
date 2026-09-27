# ROLLS KILLA – Hi-Hat Roll Presets (instrukce pro Claude Code)

Tento soubor je zadání projektu. Drž se ho a postupuj po milestonech (viz konec).
Po každém milestonu projekt zkompiluj, spusť testy a stručně shrň, co je hotové.

## 1. Produkt

- **Název:** Rolls Killa, podtitul „Hi-Hat Roll Presets“.
- **Série:** Killa pluginy (808 Killa, Keys Killa). Vizuálně i názvoslovím musí zapadat do série.
- **Pro koho:** trap, rage, plugg, drill a underground producenti, kteří nechtějí ručně kreslit hi-hat rolly.
- **Hlavní myšlenka:** plugin je **knihovna hotových trapových hi-hat rollů (presetů)**. Uživatel klikne na obdélník **PRESETS**, otevře se tabulka presetů rozdělená do kategorií, vybere roll a ten hned hraje. Pak ho může **mírně upravit / „dohrát“** pár ovladači (rychlost rollů, hustota, velocity, pitch, swing, variace). Nahoře v pluginu běží **interaktivní okénko**, kde je vidět, jak roll právě hraje.
- Pattern hraje buď přímo v pluginu (vestavěný sampler), nebo se přetáhne jako MIDI do DAW.
- **Musí to znít jako trap.** Presety nejsou náhodný generátor – jsou postavené podle analýzy reálných hi-hat MIDI z producentových drumkitů (viz sekce 3).
- **Hlavní DAW pro testování:** FL Studio (Windows). Musí fungovat i v Ableton Live a dalších VST3 hostech.

## 2. Technologie

- **C++17, JUCE 8, CMake.** JUCE stáhni přes CMake `FetchContent`, verzi pevně zafixuj (tag).
- **Formáty:** VST3 a Standalone. AU jen jako volitelný cíl, když se bude buildit na macOS.
- **Build:** Windows + Visual Studio 2022/2026 (generátor „Visual Studio 17 2022“ nebo novější) a Ninja jako alternativa.
- Typ pluginu: **instrument (synth) s MIDI výstupem** (`IS_SYNTH TRUE`, `NEEDS_MIDI_OUTPUT TRUE`), aby šel v FL použít jako generátor na kanálu i jako MIDI zdroj.
- **Bez externích závislostí** kromě JUCE. Žádné online služby.
- Kód drž čistý: DSP, knihovna presetů a úpravy patternu oddělené od UI, všechno testovatelné bez GUI.

### Struktura projektu

```
RollsKilla/
  CMakeLists.txt
  Source/
    PluginProcessor.h/.cpp        // audio + MIDI, parametry (APVTS), host sync
    PluginEditor.h/.cpp           // UI
    engine/Pattern.h              // datová struktura patternu (noty, velocity, pitch, délka)
    engine/RollLibrary.h/.cpp     // načtení továrních presetů (JSON v BinaryData), kategorie, hledání
    engine/PatternShaper.h/.cpp   // úpravy presetu knoby (speed, density, velocity, pitch, swing)
    engine/VariationEngine.h/.cpp // KILL = deterministická variace aktuálního presetu podle seedu
    engine/HatSampler.h/.cpp      // vestavěný sampler hajtek (choke, pitch per nota)
    engine/MidiExport.h/.cpp      // export do .mid + drag & drop
    ui/PresetBrowser.h/.cpp       // tabulka presetů (kategorie × presety)
    ui/RollVisualizer.h/.cpp      // interaktivní okénko nahoře
    ui/…                          // knoby, tlačítka, look & feel
  Resources/Samples/              // vestavěné hi-haty (Killa Drum Kit Vol.1)
  Resources/Presets/Factory/*.json// tovární rolly (autorsky vytvořené, viz 3.4)
  tools/midi_analyzer/            // skript, který z MIDI složky spočítá statistiky (k ladění presetů, není součást pluginu)
  Tests/                          // Catch2 nebo JUCE UnitTest
```

## 3. Jak mají trapové rolly vypadat (naučeno z drumkitů)

### 3.1 Zdroj dat

Analyzováno **545 hi-hat MIDI** z producentovy složky `Documents\drumkits` (22 kitů, mj. Detroit Heatwave, PSYKOTIC Deconstructed, !KITS! Collection, Playboi Drum Kit, Rio Leyva, Deviants, 1ONEAM, lilp0o, Killa Drum Kit Vol.1) a hi-haty z **Killa_Drum_Kit_Vol1**. Celkem ~3 900 taktů a ~2 900 rollů.

### 3.2 Čísla, podle kterých se presety dělají

- **Tempo:** medián 140 BPM, rage/underground kity ~152 BPM, Killa kit 146–160 BPM. Presety se ukládají v dobách (beats), takže sedí na jakékoli tempo.
- **Základní grid:** převážně 1/8 a 1/16. Kity programované v „half-time“ (1/8 při 140 = 1/16 při 70) se převádějí na 1/16.
- **Rozestupy not (všechny hajtky):** 1/8 33 %, triolová 1/24 (0.167 doby) 13 %, 1/16 13 %, triolová osmina (0.333) 6,5 %, 1/32 6 %, 1/48 (0.083) 6 %, 1/96 (0.042) 6 %, 1/64 6 %.
- **Rychlost uvnitř rollu:** trioly 1/24 (0.167 doby) 43 %, 1/32 22 %, 1/48 12 %, 1/64 12 %, 1/96 10 %.
  → trioly jsou v trapu nejčastější, 1/64 a 1/96 jsou „ozdoba“ hlavně na konci fráze.
- **Délka rollu:** nejčastěji **2/3 doby** (27 %), pak 1/2 doby (10 %), ~5/8, 3/8, 1/4 doby. Dlouhé rolly přes ~1 dobu (6–9 %) jsou fill na konci fráze.
- **Kde roll začíná:** na době 58 %, na osmině („a“) 36 %, jinde ~6 % (hlavně 5. šestnáctina – posunutý „skip“).
- **Hustota:** průměrně **0,73 rollu na takt** (medián 0,5). Rage má rollů méně a spoléhá na tvrdou rovnou 1/8–1/16.
- **Velocity v rollu:** **80 % rovná**, 13,5 % ramp nahoru, 6,5 % ramp dolů. (Výchozí je tedy rovná, ramp je volba/variace, ne pravidlo.)
- **Pitch v rollu:** 55 % bez změny. Jinak nejčastěji +3, +4, +5, +7, +12 půltónů, nahoru častěji než dolů.
- **Zvuk hajtek (Killa Drum Kit Vol.1, 14 closed + 8 open, 44,1 kHz/24 bit):** closed 75–185 ms, dozvuk (−30 dB) 55–125 ms, jas (spektrální těžiště) 7,7–10 kHz. Open hats 0,26–0,56 s.
  → 1/64 roll při 150 BPM má krok **25 ms**, takže se noty překrývají: sampler **musí mít choke** (nová nota utne předchozí s krátkým fade 1–3 ms), jinak roll zní rozmazaně.

### 3.3 Kategorie (12 × 8 = 96 presetů)

Názvy jsou schválně přehnané a šílené: thrash metal, latina, okultní/satanistická estetika, vomit, aura, brain-rot slang a šílenství. **Zvuk ale musí zůstat trap.** Každá kategorie má pevný hudební profil odvozený z čísel v sekci 3.2. Úplným opakem je kategorie **PRIMITIVUS**: rolly úplně jednoduché, pattern mění jen minimálně, ale brutálně, a síla je v jednoduchosti.

| # | Kategorie | Hudební profil | Tempo | Rolly | Velocity | Pitch |
|---|---|---|---|---|---|---|
| 1 | **PRIMITIVUS** | primitivní a brutální: rovná 1/8 nebo 1/16, max. 1 krátký roll na 2 takty | 130–160 | jen 1/32 nebo trioly, 1/4–1/2 doby | tvrdá 110–127, rovná | 0 |
| 2 | **LIBER TRAP** | klasický trap (ATL), „kodex“ | 130–150 | trioly 1/24 + 1/32, fill 1/64 na konci 4. taktu | rovná ~95, občas ramp | +12 na fillu |
| 3 | **TRINITAS** | všechno ve trojkách, triolové rolly a triolové skipy | 135–155 | trioly 1/24 a 1/48, 2/3 doby | rovná | 0 až +5 |
| 4 | **VOMITORIUM** | rage: tvrdá rovná 1/16, krátké zvratky 1/32 | 150–165 | málo, krátké 1/32 | ~115, skoro bez rozdílů | +2 až +5 |
| 5 | **BLAST RITUAL** | thrash/blast beat: nejrychlejší a nejhustší | 140–165 | 1/64 a 1/96 bursty, dlouhé filly přes 1 dobu | ramp nahoru | +12 |
| 6 | **AURA FARM** | plugg/pluggnb, vzdušné a měkké | 140–160 | málo, spíš triolové | měkká 70–95 | melodické +3/+5/+7 |
| 7 | **DELIRIUM** | underground melodic, glitter rolly | 140–155 | hodně rychlých not (až 45 %), 1/64–1/96 | rozsah 50–80 | melodické přes 12+ půltónů |
| 8 | **NECRODRILL** | drill (UK/NY), triolové skipy | 140–145 | 3 údery přes 1/2 doby, posunuté starty | akcenty | minimum |
| 9 | **MOTOR MORTIS** | Detroit, bouncy | 130–145 | 1/32 bursty, ~19 % rychlých not | široký rozsah (~40) | ~4 výšky na pattern |
| 10 | **SPASMUS** | jerk: záškuby a pauzy | 140–155 | 1/32 + časté díry | střední | minimum |
| 11 | **REQUIEM** | dark/slow half-time, memphis | 60–80 | 1/32 a trioly, dlouhé filly | rovná | +12 / −12 |
| 12 | **MANIA** | chaos: změna rychlosti uprostřed rollu, pitch pády | 140–170 | mix 1/24 → 1/64 → 1/96 v jednom rollu | ramp nahoru i dolů | ±12, pády dolů |

### 3.4 Tovární presety

- **Počet:** 12 kategorií × 8 presetů = **96 rollů** ve verzi 1.0. Každý preset má 1, 2 nebo 4 takty (fill vždy na konci fráze).
- **Formát:** JSON (`name`, `category`, `bpmHint`, `bars`, `tags`, `notes: [{beat, len, vel, pitch}]`), uložený v `BinaryData`.
- **Presety vytváříme autorsky** podle čísel ze sekcí 3.2–3.3. **Nekopírovat 1:1 MIDI z cizích kitů** do pluginu (licence kitů povoluje použití v hudbě, ne redistribuci v produktu). Analyzátor v `tools/` slouží jen k tomu, aby presety statisticky odpovídaly reálnému trapu.
- **Validace presetů (test):** rolly začínají na době/osmině (výjimka „skip“ u NECRODRILL, MOTOR MORTIS, SPASMUS, MANIA), rychlosti rollů jen z povolené sady (1/24, 1/32, 1/48, 1/64, 1/96), velocity 1–127, žádné překrývající se noty stejné výšky, délka = počet taktů. PRIMITIVUS navíc: max. 1 roll na 2 takty.
- **Názvy presetů** jsou originální, **bez jmen reálných umělců, kapel, alb a značek**:

| Kategorie | Presety |
|---|---|
| PRIMITIVUS | Club Hit · Stone Age · Caveman Tick · One Blow · Raw Bone · Blunt Force · Neanderthal · Skull Tap |
| LIBER TRAP | Codex Trappus · Bando Evangelium · Psalm 808 · Gospel of Drip · Ave Trappa · Liber Choppa · Opus Stash · Trap Sacrament |
| TRINITAS | Tres Diaboli · Holy Triplet · Trinity Knife · Tri-Hex · Third Eye Roll · Triple Six · Trine Tick · Pater Triplex |
| VOMITORIUM | Bile Rage · Puke Pit · Acid Reflux · Mosh Vomit · Gutspill · Stomach Pump · Toxic Spew · Retch Riot |
| BLAST RITUAL | Blastfemy · Thrash Chapel · Speed Kills · Shredder Mass · Riff Carnage · Headbang Hex · Mach Satanas · Pit Liturgy |
| AURA FARM | +1000 Aura · Aura Debt · Halo Drip · Glow Up Ghost · Mog Mode · Aura Leak · Main Character · Cloud Sigil |
| DELIRIUM | Fever Dream · Brain Rot · Lucid Snake · Glitter Psychosis · Pill Ladder · Melting Clock · Echo Asylum · Mind Siphon |
| NECRODRILL | Grave Slide · Corpse Skip · Necro Block · Tombstone Step · Cold Casket · Crypt Runner · Bone Saw · Morgue Opps |
| MOTOR MORTIS | Rust Bounce · Engine Exorcism · Chrome Carcass · Assembly Hex · Burnout Ritual · Scrapyard Knock · V8 Voodoo · Diesel Demon |
| SPASMUS | Twitch · Nerve Jerk · Seizure Step · Glitch Limb · Stop Drop Rot · Tic Tac Hex · Convulsio · Short Circuit |
| REQUIEM | Lacrimosa Hat · Dies Irae Drip · Syrup Mass · Funeral Crawl · Memento Mori · Black Candle · Slow Exhumation · Codeine Crypt |
| MANIA | Insanus · Straitjacket · Rabies · Padded Room · Ego Death · 13th Floor · Psycho Loop · Unhinged |

- V každé kategorii jdou presety **od nejjednoduššího po nejdivočejší** (1 = nejklidnější, 8 = nejvíc crazy).
- Uživatel si může uložit **vlastní presety** (kategorie „User“) a označit preset hvězdičkou (**Favorites**).

## 4. Parametry (APVTS, všechny automatizovatelné)

Úpravy jsou **jemné** – preset musí pořád znít jako ten preset. Aplikují se nedestruktivně nad originálem presetu (`PatternShaper`).

- **Preset** (index v knihovně + Next/Prev)
- **Roll Speed**: posun rychlosti rollů o krok (Slower / Original / Faster: 1/24 ↔ 1/32 ↔ 1/48 ↔ 1/64)
- **Density** 0–200 % (100 % = originál; pod 100 ubírá rolly, nad 100 přidává krátké rolly na osminy podle profilu kategorie)
- **Velocity** (Original / Flat / Ramp Up / Ramp Down) + **Groove** 0–100 % (humanizace)
- **Pitch Ramp** −12 až +12 půltónů (0 = originál)
- **Swing** 0–60 %
- **Variation** 0–100 % (jak moc KILL mění preset)
- **Bars** 1 / 2 / 4 / 8 (kratší preset se opakuje, fill zůstane na konci fráze)
- **Seed** (skrytý, mění ho tlačítko KILL)
- **Sampler:** výběr hajtky (14 closed Killa hats + vlastní WAV), Tune, Decay, Choke on/off, Volume

## 5. Funkce

**MVP (verze 1.0):**

1. **Obdélník PRESETS** (lišta s názvem aktuálního presetu a šipkami ◀ ▶). Po kliknutí se otevře **Preset Browser**: vlevo 12 kategorií (každá s vlastní ikonou/sigilem), vpravo tabulka presetů (název, BPM hint, počet taktů, tagy, hvězdička). Hover = náhled v mini gridu, klik = načíst a hrát, šipky/klávesy = listování, pole pro hledání.
2. **Interaktivní okénko (Roll Visualizer)** nahoře: živé zobrazení patternu synchronně s hostem – přehrávací hlava, noty jako sloupce podle velocity, rolly jako rozsvícené shluky, pitch jako barevný přechod. Klik do okénka = ztlumí/obnoví notu, tažení nahoru/dolů = velocity noty, dvojklik na roll = změní jeho rychlost. Když host stojí, jde spustit **Preview** (plugin hraje sám v BPM hintu presetu).
3. **KILL**: deterministická variace aktuálního presetu (nový seed) v mezích profilu kategorie, síla podle knobu Variation. **Lock bars** zamkne takty, které KILL nemění.
4. Pattern hraje synchronně s hostem (PPQ pozice, tempo, loop) přes **vestavěný sampler** s choke.
5. Pattern se zároveň posílá na **MIDI výstup** (pro FL: Channel settings → MIDI out).
6. **Drag & drop MIDI** z oblasti „DRAG MIDI“ (`juce::DragAndDropContainer::performExternalDragDropOfFiles`, soubor dočasně v temp složce).
7. **Export MIDI** do souboru.
8. **Vestavěný sampler:** 14 closed hats z Killa Drum Kit Vol.1 (`Killa_Hat_01–14.wav`, přidám do `Resources/Samples/`; mezitím placeholder generovaný kódem). Uživatel může přetáhnout vlastní WAV.
9. **Undo/Redo** posledních 20 změn.
10. **Ukládání stavu** v projektu DAW (`getStateInformation`): preset, úpravy, seed, zámky, sampler.

**Verze 1.1+ (zatím neimplementovat, jen připravit architekturu):**

- **Open hat vrstva** (8 open hats z Killa kitu) a **perc vrstva**.
- **Follow 808:** čtení MIDI vstupu (808 linky) – rolly se vyhýbají 808 slidům, nebo je zvýrazní.
- **Sync s ostatními Killa pluginy.**
- Rozšiřující balíčky presetů (další kategorie po 8).

## 6. UI

- Velikost ~ 900×560 px, škálovatelné (100 / 125 / 150 %).
- **Trapový vzhled série Killa:** tmavé pozadí (#0E0E10), akcent neon červená (#FF2E3E), bílé texty, výrazné bold/condensed písmo v logu, jemný glow na aktivních prvcích, lehká textura (grain/chrome), žádné kýčovité efekty.
- **Horní část:** logo „ROLLS KILLA“ + **Roll Visualizer** přes celou šířku (hlavní „wow“ prvek – plynulá animace 60 fps, glow na hrajících notách).
- **Pod ním:** obdélník **PRESETS** (◀ kategorie / název presetu ▶), undo/redo, zámky taktů.
- **Spodní část:** knoby Roll Speed, Density, Velocity/Groove, Pitch, Swing, Variation; sekce sampleru; obří tlačítko **KILL**; oblast **DRAG MIDI**.
- **Preset Browser** se otevírá jako překryvný panel přes plugin (ne nové okno), zavírá se klikem mimo nebo Esc.
- Žádné cizí loga ani trademarky (hlavně ne logo nebo design Rolls-Royce), žádná jména reálných umělců.

## 7. Kvalita a testy

- **Unit testy knihovny:** všechny tovární presety se načtou a projdou validací (3.4); celkem přesně 96 presetů, každá z 12 kategorií má 8.
- **Statistický test:** souhrn továrních presetů odpovídá číslům ze sekce 3.2 v toleranci (např. podíl triolových rollů 30–55 %, 0,4–1,2 rollu/takt, rovná velocity ≥ 60 %).
- **Unit testy PatternShaper/Variation:** determinismus podle seedu, výchozí hodnoty = přesně originál presetu, velocity 1–127, žádné překrývající se noty stejné výšky, zamčené takty se nemění.
- **Test MIDI exportu:** soubor jde znovu načíst a má správné tempo, délku a počet not.
- **Audio thread:** žádné alokace ani zámky v `processBlock`, pattern se předává lock-free (atomický swap ukazatele / double buffer).
- **Host sync:** správně při loopu, změně tempa a skoku pozice. Otestovat ve FL Studiu (i při změně tempa uprostřed) a ve Standalone.
- **pluginval** (úroveň strictness 5+) musí projít.

## 8. Milestony

1. **M1:** CMake + JUCE projekt, prázdný VST3/Standalone se kompiluje a načte ve FL Studiu.
2. **M2:** `Pattern`, `RollLibrary`, formát presetů + validátor, `tools/midi_analyzer` a prvních 24 továrních presetů (2 na kategorii) + unit testy.
3. **M3:** Sampler s choke, přehrávání synchronní s hostem, MIDI výstup.
4. **M4:** UI – Roll Visualizer, obdélník PRESETS + Preset Browser, knoby, KILL, lock bars.
5. **M5:** `PatternShaper` + `VariationEngine`, drag & drop a export MIDI, undo/redo, ukládání stavu, user presety a favorites.
6. **M6:** doplnit knihovnu na 96 presetů (12 × 8), poslechové ladění, pluginval, výkon, finální Release build a krátký README s instalací (kam zkopírovat .vst3: `C:\Program Files\Common Files\VST3`).

## 9. Pravidla pro práci

- Před většími změnami napiš krátký plán a pokračuj.
- Nepřidávej knihovny ani služby mimo JUCE bez zeptání.
- Commituj po každém milestonu s jasnou zprávou (pokud je projekt v gitu).
- Když něco nejde otestovat automaticky (např. FL Studio, poslech presetů), napiš mi přesný postup, co mám vyzkoušet.
