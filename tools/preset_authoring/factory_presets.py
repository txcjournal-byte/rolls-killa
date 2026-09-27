"""Rolls Killa factory presets - authored by hand (see build_presets.py for the pattern language).

v2 - rebuilt after studying ~300 hi-hat MIDI files from the producer's drum kits
(tools/midi_analyzer + grid views). Nothing is copied; the presets follow what those files do:

- the backbone is a straight 1/8 (8 hits per bar); 1/16 only as single "skip" pickups
- about 10-12 hits per bar in total, ~1 roll per bar
- rolls are SHORT (3-5 notes) and LAND on the next beat or "and" - the landing note is part
  of the roll (T4 = triplet from the "and" into the beat, D5 = 1/32 from the beat into the "and")
- fast rolls (1/48, 1/96) often fall in pitch
- holes: whole beats of silence are a normal part of the groove (rage, fills)
- 2-bar phrases: bar 1 ~ bar 3, bars 2 and 4 carry the variation / fill

Order inside a category = from the simplest (1) to the craziest (8).
"""

from build_presets import P, R

# ---- 1/16 grid strings (x hit, X accent, o soft, . rest) -------------------------------
E = "x.x.x.x.x.x.x.x."        # straight 1/8
EA = "X.x.X.x.X.x.X.x."       # 1/8, accented beats
EO = "x.o.x.o.x.o.x.o."       # 1/8, soft "ands" (plugg)
K2 = "x.x.x.xxx.x.x.x."       # 1/16 skip into beat 3
K4 = "x.x.x.x.x.x.x.xx"       # 1/16 skip into the next bar
K24 = "x.x.x.xxx.x.x.xx"      # both skips
G3 = "x.x.x.x.....x.x."       # beat 3 silent
G4 = "x.x.x.x.x.x....."       # beat 4 silent
G34 = "x.x.x.x........."      # second half silent
S = "xxxxxxxxxxxxxxxx"        # straight 1/16 (rage / half-time)
SG4 = "xxxxxxxxxxxx...."      # 1/16, beat 4 silent
T = "xxxxxxxxxxxx"            # 1/8 triplets (12 grid)
TK = "x.xx.xx.xx.x"           # triplet skip grid

# ---- roll shorthands: bar, position (beats inside the bar) ------------------------------
# landing forms: the last note falls on the next beat / "and"


def T4(b, p, **k):   # 1/24 x4: from the "and" into the beat (the classic trap triplet)
    return R(b, p, 24, 4, **k)


def T7(b, p, **k):   # 1/24 x7: one whole beat of triplets, lands on the beat
    return R(b, p, 24, 7, **k)


def T3(b, p, **k):   # 1/24 x3: only at the end of a bar (the bar line is the landing)
    return R(b, p, 24, 3, **k)


def D5(b, p, **k):   # 1/32 x5: half a beat, lands
    return R(b, p, 32, 5, **k)


def D4(b, p, **k):   # 1/32 x4: end of a bar
    return R(b, p, 32, 4, **k)


def D3(b, p, **k):   # 1/32 x3: short stab (next 1/8 stays)
    return R(b, p, 32, 3, **k)


def D9(b, p, **k):   # 1/32 x9: one beat, lands
    return R(b, p, 32, 9, **k)


def F7(b, p, **k):   # 1/48 x7: half a beat, lands
    return R(b, p, 48, 7, **k)


def F6(b, p, **k):   # 1/48 x6: end of a bar
    return R(b, p, 48, 6, **k)


def H9(b, p, **k):   # 1/64 x9: half a beat, lands
    return R(b, p, 64, 9, **k)


def H8(b, p, **k):   # 1/64 x8: end of a bar
    return R(b, p, 64, 8, **k)


def Z13(b, p, **k):  # 1/96 x13: half a beat, lands
    return R(b, p, 96, 13, **k)


def Z12(b, p, **k):  # 1/96 x12: end of a bar
    return R(b, p, 96, 12, **k)


DOWN = ("ramp", 0, -5)        # fast rolls falling in pitch (very common in the kits)
DOWN12 = ("ramp", 0, -12)
UP12 = ("ramp", 0, 12)

LIBRARY = {
    # ------------------------------------------------------------------ 1
    # primitive and brutal: straight 1/8, max one short roll per two bars, hard flat velocity
    "PRIMITIVUS": [
        P("Club Hit", 140, 2, ["1/8", "raw"], [E, E], [
            D4(2, 3.5),
        ], vel=118),
        P("Stone Age", 150, 2, ["1/8", "raw"], [E, E], [
            T3(2, 3.5),
        ], vel=116),
        P("Caveman Tick", 145, 2, ["1/8", "accent"], [EA, EA], [
            T4(2, 2.5),
        ], vel=112, acc=12),
        P("One Blow", 135, 4, ["1/8", "hole"], [E, E, E, G4], [
            D4(2, 3.5),
            D5(4, 2.5),
        ], vel=120),
        P("Raw Bone", 150, 4, ["1/8", "skip"], [E, K2, E, E], [
            T4(1, 1.5),
            D4(4, 3.5),
        ], vel=114),
        P("Blunt Force", 155, 2, ["hard", "1/32"], [EA, "X.x.X.x.X.x....."], [
            D5(2, 2.0),
        ], vel=116, acc=11),
        P("Neanderthal", 140, 4, ["switch", "1/16"], [E, E, E, S], [
            T3(2, 3.5),
            D4(4, 3.5),
        ], vel=122),
        P("Skull Tap", 160, 4, ["crazy", "skip"], [K24, K24, K24, G3], [
            D5(1, 2.5),
            D5(4, 1.5),
        ], vel=115, acc=12),
    ],
    # ------------------------------------------------------------------ 2
    # classic ATL trap: triplets + 1/32, a 1/64 fill at the end of bar 4 (+12)
    "LIBER TRAP": [
        P("Codex Trappus", 140, 2, ["triplet", "classic"], [E, E], [
            T4(1, 1.5),
            D4(2, 3.5),
        ], vel=95),
        P("Bando Evangelium", 140, 4, ["triplet", "1/64", "fill"], [E, E, E, E], [
            T4(1, 1.5),
            D5(2, 2.5),
            T4(3, 1.5),
            H8(4, 3.5, pitch=UP12),
        ], vel=95),
        P("Psalm 808", 140, 4, ["triplet", "fill"], [E, K2, E, G4], [
            T3(1, 3.5),
            T4(2, 1.5),
            T3(3, 3.5),
            H9(4, 2.0, pitch=UP12),
        ], vel=96),
        P("Gospel of Drip", 145, 2, ["triplet", "1/32"], [E, K2], [
            T4(1, 2.5),
            D4(2, 3.5, vel=("up", 78, 100)),
        ], vel=94),
        P("Ave Trappa", 138, 4, ["triplet", "ramp"], [E, E, E, E], [
            T4(1, 1.5),
            D5(2, 0.5),
            T4(2, 2.5),
            T4(3, 1.5),
            D5(4, 1.0),
            R(4, 3.0, 64, 16, pitch=12),
        ], vel=95),
        P("Liber Choppa", 142, 4, ["choppy", "fill"], [EA, EA, EA, G3], [
            T4(1, 2.5),
            D5(2, 1.5),
            T4(3, 2.5),
            F7(4, 1.5),
            R(4, 3.0, 64, 16, vel=("up", 80, 110), pitch=("steps", [0, 12])),
        ], vel=92, acc=8),
        P("Opus Stash", 146, 2, ["busy", "1/64"], [E, K2], [
            T4(1, 0.5),
            D5(1, 2.5),
            T4(2, 1.0),
            R(2, 3.0, 64, 16, pitch=UP12),
        ], vel=95),
        P("Trap Sacrament", 150, 4, ["crazy", "fill"], [E, K2, E, G4], [
            T4(1, 1.5),
            D4(1, 3.5),
            F7(2, 2.5, pitch=[0, 0, 0, 0, 12, 12, 12]),
            T4(3, 0.5),
            D5(3, 2.0),
            T4(4, 0.5),
            R(4, 2.0, 64, 24, vel=("up", 78, 118), pitch=UP12),
        ], vel=96),
    ],
    # ------------------------------------------------------------------ 3
    # everything in threes: triplet rolls, triplet bars, 2/3-beat rolls, pitch 0..+5
    "TRINITAS": [
        P("Tres Diaboli", 145, 2, ["triplet"], [T, E], [
            T4(2, 2.5),
        ], vel=96),
        P("Holy Triplet", 140, 1, ["triplet", "2/3"], [E], [
            T4(1, 1.5),
        ], vel=98),
        P("Trinity Knife", 150, 2, ["1/48"], [E, T], [
            F7(1, 2.5),
            F6(2, 3.5),
        ], vel=97),
        P("Tri-Hex", 145, 4, ["triplet", "fill"], [E, T, E, T], [
            T4(1, 1.5),
            T4(2, 2.0),
            T4(3, 1.5),
            T7(4, 2.0),
        ], vel=96),
        P("Third Eye Roll", 140, 2, ["1/48", "pitch"], [T, E], [
            F7(1, 1.0, pitch=("ramp", 0, 5)),
            T4(2, 2.5, pitch=[0, 3, 5, 5]),
        ], vel=95),
        P("Triple Six", 155, 4, ["fill", "crazy"], [E, T, E, T], [
            T4(1, 1.5),
            T3(1, 3.5),
            F7(2, 1.0),
            T4(3, 2.5, pitch=("steps", [0, 5])),
            T7(4, 1.0),
            F6(4, 3.5, pitch=("ramp", 0, 5)),
        ], vel=98),
        P("Trine Tick", 150, 1, ["skip"], [TK], [
            T4(1, 1.5),
        ], vel=96),
        P("Pater Triplex", 150, 4, ["1/48", "fill", "crazy"], [T, E, T, E], [
            F7(1, 1.0),
            T4(1, 2.5),
            T4(2, 1.5, pitch=[0, 3, 3, 5]),
            F7(2, 3.0),
            T4(3, 0.5),
            R(3, 2.0, 48, 13),
            T4(4, 1.0),
            R(4, 2.5, 48, 18, pitch=("ramp", 0, 5)),
        ], vel=97),
    ],
    # ------------------------------------------------------------------ 4
    # rage: hard and straight, holes, few short 1/32 stabs, velocity ~115, pitch +2..+5
    "VOMITORIUM": [
        P("Bile Rage", 160, 2, ["rage", "1/8"], [E, E], [
            D3(2, 3.5),
        ], vel=116),
        P("Puke Pit", 155, 2, ["rage", "hole"], [E, G3], [
            D5(1, 2.5),
        ], vel=114),
        P("Acid Reflux", 158, 2, ["rage", "1/16"], [S, SG4], [
            D4(2, 3.5, pitch=("steps", [0, 2])),
        ], vel=116),
        P("Mosh Vomit", 160, 4, ["rage", "pitch"], [E, E, G3, E], [
            D3(2, 3.5),
            D5(4, 2.5, pitch=[0, 0, 0, 5, 5]),
        ], vel=114),
        P("Gutspill", 155, 4, ["rage", "burst"], [E, G4, E, G34], [
            D5(1, 1.5),
            D4(3, 3.5),
            D5(4, 1.5, pitch=("steps", [0, 3])),
        ], vel=113),
        P("Stomach Pump", 162, 2, ["rage", "pump"], [S, "xxxxxxxx....xxxx"], [
            D3(1, 1.0),
            D5(2, 2.5, pitch=("ramp", 0, 5)),
        ], vel=118),
        P("Toxic Spew", 165, 4, ["rage", "1/64"], [E, G3, E, G34], [
            D5(1, 2.5),
            H9(2, 1.5),
            D4(3, 3.5, pitch=2),
            D5(4, 1.0, pitch=5),
        ], vel=115),
        P("Retch Riot", 164, 2, ["crazy", "rage"], [S, "xxxx....xxxxxxxx"], [
            D3(1, 0.5),
            D5(1, 2.0, pitch=3),
            D5(2, 2.5, pitch=("ramp", 0, 5)),
        ], vel=117),
    ],
    # ------------------------------------------------------------------ 5
    # thrash/blast: the fastest - 1/64 and 1/96 bursts, long fills over a beat, ramp up, +12
    "BLAST RITUAL": [
        P("Blastfemy", 150, 2, ["1/64", "fill"], [E, E], [
            H9(1, 3.0, vel=("up", 82, 112)),
            R(2, 2.0, 64, 24, vel=("up", 75, 124), pitch=UP12),
        ], vel=100),
        P("Thrash Chapel", 155, 2, ["1/96", "fill"], [EA, EA], [
            Z13(1, 1.0),
            H9(1, 3.0, vel=("up", 85, 115)),
            R(2, 3.0, 96, 24, vel=("up", 80, 125), pitch=("steps", [0, 12])),
        ], vel=102, acc=10),
        P("Speed Kills", 160, 2, ["1/64", "fill"], [E, K2], [
            H9(1, 2.0, vel=("up", 85, 115)),
            R(2, 2.5, 64, 24, vel=("up", 78, 122), pitch=UP12),
        ], vel=104),
        P("Shredder Mass", 150, 4, ["1/96", "long fill"], [EA, EA, EA, G34], [
            Z13(2, 1.0),
            R(2, 3.0, 64, 16, vel=("up", 84, 120)),
            R(3, 2.0, 96, 24),
            R(4, 1.5, 64, 40, vel=("up", 75, 125), pitch=UP12),
        ], vel=102, acc=10),
        P("Riff Carnage", 155, 2, ["1/96", "riff"], [EA, EA], [
            H9(1, 1.5),
            R(2, 1.0, 64, 16, vel=("up", 80, 120)),
            R(2, 3.0, 96, 24, pitch=("steps", [0, 12])),
        ], vel=100, acc=12),
        P("Headbang Hex", 145, 4, ["headbang", "fill"], [E, E, E, G3], [
            H9(1, 1.0),
            R(2, 2.0, 96, 24, vel=("up", 80, 120)),
            H9(3, 1.0, pitch=12),
            H9(3, 3.0),
            Z13(4, 1.0),
            R(4, 3.0, 64, 16, vel=("up", 80, 122), pitch=UP12),
        ], vel=100),
        P("Mach Satanas", 165, 2, ["fastest", "long fill"], [EA, G34], [
            Z13(1, 0.5),
            R(1, 2.0, 64, 16, vel=("up", 80, 120)),
            R(2, 2.0, 64, 32, vel=("up", 70, 127), pitch=UP12),
        ], vel=105, acc=10),
        P("Pit Liturgy", 160, 4, ["crazy", "long fill"], [EA, EA, EA, G34], [
            R(1, 1.0, 96, 24, vel=("up", 80, 120)),
            H8(1, 3.5),
            R(2, 2.0, 96, 36),
            R(3, 1.0, 64, 16),
            Z12(3, 3.5, pitch=("steps", [0, 12])),
            R(4, 1.5, 96, 60, vel=("up", 70, 127), pitch=UP12),
        ], vel=104, acc=10),
    ],
    # ------------------------------------------------------------------ 6
    # plugg / pluggnb: airy, soft 70-95, few mostly triplet rolls, melodic +3/+5/+7
    "AURA FARM": [
        P("+1000 Aura", 150, 2, ["plugg", "soft"], [EO, EO], [
            T3(2, 3.5, pitch=[0, 5, 7]),
        ], vel=82, soft=16),
        P("Aura Debt", 145, 2, ["plugg", "melodic"], [EO, G4], [
            T4(1, 1.5, pitch=("steps", [0, 5])),
            T4(2, 1.5, pitch=[0, 3, 5, 7]),
        ], vel=80, soft=14),
        P("Halo Drip", 148, 2, ["plugg", "melodic"], [EO, EO], [
            T3(1, 3.5, pitch=[0, 3, 5]),
            T4(2, 2.5, pitch=[0, 5, 7, 7]),
        ], vel=80, soft=14),
        P("Glow Up Ghost", 152, 4, ["plugg", "soft"], [EO, EO, EO, G4], [
            T4(2, 1.5, pitch=("steps", [0, 7])),
            T7(4, 1.0, pitch=[0, 3, 5, 7, 5, 3, 0]),
        ], vel=78, soft=14),
        P("Mog Mode", 150, 4, ["pluggnb", "melodic"], [EO, K2, EO, EO], [
            T4(1, 1.5, pitch=[0, 0, 5, 5]),
            D4(2, 3.5, pitch=("ramp", 0, 7)),
            T4(3, 1.5, pitch=[0, 3, 5, 7]),
        ], vel=82, soft=16),
        P("Aura Leak", 145, 2, ["pluggnb", "1/32"], [EO, G3], [
            T4(1, 1.5, pitch=[0, 5, 7, 12]),
            D9(2, 3.0 - 1.0, vel=("up", 70, 92), pitch=("steps", [0, 3, 5, 7])),
        ], vel=76, soft=12),
        P("Main Character", 158, 4, ["melodic", "main"], [EO, EO, EO, EO], [
            T4(1, 2.5, pitch=[0, 3, 5, 7]),
            T4(2, 1.5, pitch=[7, 5, 3, 0]),
            T4(3, 2.5, pitch=[0, 5, 7, 12]),
            R(4, 2.0, 32, 8, pitch=("ramp", 0, 7)),
        ], vel=84, soft=16),
        P("Cloud Sigil", 155, 2, ["crazy", "melodic"], [EO, K2], [
            T4(1, 0.5, pitch=[0, 3, 7, 7]),
            T4(1, 2.5, pitch=[0, 5, 7, 12]),
            D5(2, 1.0, pitch=("ramp", 12, 0)),
            R(2, 2.5, 24, 9, vel=("up", 70, 92), pitch=[0, 3, 5, 7, 10, 12, 12, 12, 12]),
        ], vel=80, soft=14),
    ],
    # ------------------------------------------------------------------ 7
    # underground melodic: glitter rolls 1/48-1/96 that fall in pitch, soft 50-80
    "DELIRIUM": [
        P("Fever Dream", 150, 2, ["1/64", "melodic"], [EO, EO], [
            H9(1, 1.5, vel=("up", 58, 78), pitch=("ramp", 12, 0)),
            Z12(2, 3.5, pitch=("ramp", 12, 0)),
        ], vel=70, soft=12),
        P("Brain Rot", 145, 2, ["1/96", "pitch"], [EO, G4], [
            H9(1, 0.5, pitch=("steps", [0, 12])),
            Z13(1, 2.5, pitch=DOWN12),
            H9(2, 1.0, pitch=("ramp", 7, -5)),
        ], vel=68, soft=10),
        P("Lucid Snake", 148, 2, ["snake", "melodic"], [EO, EO], [
            Z13(1, 2.5, pitch=("ramp", 7, -5)),
            R(2, 2.0, 64, 17, pitch=("ramp", 12, -12)),
        ], vel=70, soft=12),
        P("Glitter Psychosis", 150, 4, ["glitter", "1/96"], [EO, EO, EO, G34], [
            Z13(1, 1.5, pitch=("ramp", 12, 0)),
            F7(2, 2.5, pitch=[0, 7, 12, 7, 12, 19, 12]),
            Z13(3, 1.5, pitch=("ramp", 19, 0)),
            R(4, 1.5, 96, 36, vel=("up", 52, 80), pitch=("ramp", 24, 0)),
        ], vel=66, soft=10),
        P("Pill Ladder", 145, 2, ["ladder", "pitch"], [EO, EO], [
            R(1, 1.0, 48, 13, pitch=("steps", [0, 3, 7, 12, 15, 19])),
            H9(2, 1.0, pitch=("steps", [17, 12, 5, 0])),
            Z12(2, 3.5, pitch=DOWN12),
        ], vel=68, soft=10),
        P("Melting Clock", 142, 4, ["melt", "pitch drop"], [EO, EO, EO, G3], [
            R(1, 2.0, 64, 17, vel=("down", 78, 52), pitch=("ramp", 19, 0)),
            F7(2, 1.5, pitch=[0, 12, 7, 19, 12, 24, 12]),
            H9(3, 2.5, pitch=12),
            R(4, 1.0, 96, 25, pitch=("ramp", 24, 0)),
            Z12(4, 3.5, vel=("up", 55, 80), pitch=DOWN12),
        ], vel=64, soft=10),
        P("Echo Asylum", 150, 2, ["echo", "crazy"], [EO, K2], [
            H9(1, 0.5, pitch=12),
            Z13(1, 2.0, pitch=24),
            F7(2, 0.5, pitch=[0, 7, 12, 19, 24, 19, 12]),
            R(2, 2.0, 96, 36, vel=("down", 78, 50), pitch=("ramp", 24, 7)),
        ], vel=70, soft=14),
        P("Mind Siphon", 155, 4, ["crazy", "1/96"], [EO, EO, EO, G34], [
            R(1, 2.0, 64, 17, pitch=("steps", [0, 7, 12, 19, 24, 19, 12, 7])),
            R(2, 1.0, 48, 13, pitch=("steps", [0, 12, 24])),
            Z12(2, 3.5, vel=("up", 52, 80), pitch=("ramp", 24, 0)),
            H9(3, 0.5),
            Z13(3, 2.0, pitch=("ramp", 0, 24)),
            F7(4, 0.5, pitch=[24, 19, 12, 7, 0, -5, -5]),
            R(4, 1.5, 96, 60, vel=("up", 50, 80), pitch=("ramp", 0, 24)),
        ], vel=66, soft=10),
    ],
    # ------------------------------------------------------------------ 8
    # drill: accented 1/8 with triplet skips (three hits over half a beat), shifted starts
    "NECRODRILL": [
        P("Grave Slide", 142, 1, ["drill", "skip"], ["X.x.X.x.X.x.X.x."], [
            T4(1, 1.5),
        ], vel=92, acc=20),
        P("Corpse Skip", 144, 2, ["drill", "skip"], ["X.x.X.x.X.x.X.x.", "X.x.X...X.x.X.x."], [
            T4(1, 1.25),
            T3(2, 3.5, vel=[110, 90, 90]),
        ], vel=90, acc=22),
        P("Necro Block", 143, 2, ["drill", "skip"], ["X.x.X.x.X.x.X.x.", "X.x.X.x.X...X.x."], [
            T4(1, 1.75),
            T4(2, 2.25),
        ], vel=90, acc=24),
        P("Tombstone Step", 140, 4, ["drill", "step"], ["X.x.X.x.X.x.X.x."] * 4, [
            T4(1, 1.5),
            T4(2, 0.75),
            T4(4, 2.25),
            T3(4, 3.5, vel=[112, 92, 92]),
        ], vel=92, acc=20),
        P("Cold Casket", 142, 2, ["drill", "cold"], ["X.x.X.x.X.x.X.x.", "X.x.X.x.....X.x."], [
            T4(1, 0.75),
            T4(1, 2.5),
            T4(2, 1.25, pitch=[0, 0, 2, 2]),
        ], vel=88, acc=26),
        P("Crypt Runner", 144, 4, ["drill", "runner"], ["X.x.X.x.X.x.X.x."] * 4, [
            T4(1, 1.25),
            T4(2, 2.75),
            T4(3, 0.5),
            T4(3, 2.25),
            T4(4, 1.25),
            T3(4, 3.5, vel=[115, 95, 95]),
        ], vel=90, acc=22),
        P("Bone Saw", 145, 2, ["drill", "saw"], ["X.x.X.x.X.x.X.x."] * 2, [
            T4(1, 0.75),
            T4(1, 2.25),
            T4(2, 0.25),
            T4(2, 1.75),
            T3(2, 3.5),
        ], vel=92, acc=22),
        P("Morgue Opps", 145, 4, ["drill", "crazy"], ["X.x.X.x.X.x.X.x.", "X.x.X...X.x.X.x."] * 2, [
            T4(1, 0.75),
            T3(1, 3.5),
            T4(2, 1.25, pitch=[0, 0, 2, 2]),
            T4(2, 2.75),
            T4(3, 0.25),
            T7(3, 1.75),
            T4(4, 0.75),
            T4(4, 2.25, vel=("up", 85, 115)),
        ], vel=90, acc=25),
    ],
    # ------------------------------------------------------------------ 9
    # Detroit bounce: 1/8 with 1/16 pickups, wide velocity, 1/32 bursts, ~4 pitches
    "MOTOR MORTIS": [
        P("Rust Bounce", 138, 2, ["detroit", "bounce"], ["X.o.X.o.X.o.X.ox", "X.o.X.oxX.o.X.o."], [
            D3(1, 1.25),
            D4(2, 3.5, pitch=[0, 4, 7, 7]),
        ], vel=95, acc=20, soft=24),
        P("Engine Exorcism", 135, 2, ["detroit", "1/32"], ["X.o.X.oxX.o.X.o.", "X.o.X.o.X.o.X.ox"], [
            D5(1, 0.5, vel=("down", 112, 80)),
            D3(2, 1.5, pitch=2),
            D4(2, 3.5, pitch=[0, 4, 7, 7]),
        ], vel=94, acc=20, soft=22),
        P("Chrome Carcass", 140, 2, ["detroit", "bounce"], ["X.o.X.o.X.oxX.o.", "X.o.X.o.X.o.X.o."], [
            D3(1, 2.25, pitch=4),
            D5(2, 2.5, pitch=[0, 4, 7, 4, 0]),
        ], vel=94, acc=22, soft=24),
        P("Assembly Hex", 136, 4, ["detroit", "1/32"], ["X.o.X.oxX.o.X.o."] * 4, [
            D3(1, 1.25),
            D5(2, 2.5, pitch=2),
            D5(4, 2.0, vel=("down", 115, 80)),
            H8(4, 3.5, pitch=7),
        ], vel=95, acc=22, soft=26),
        P("Burnout Ritual", 142, 2, ["burnout", "bounce"], ["X.oxX.o.X.oxX.o.", "X.o.X.o.X.o.X.ox"], [
            D3(1, 1.75, pitch=[0, 2, 4]),
            H9(2, 1.0, vel=("up", 78, 115)),
            D4(2, 3.5, pitch=7),
        ], vel=92, acc=22, soft=26),
        P("Scrapyard Knock", 138, 4, ["detroit", "knock"], ["X.o.X.oxX.o.X.o.", "X.oxX.o.X.o.X.o."] * 2, [
            D3(1, 2.25, pitch=4),
            D5(2, 1.5, vel=("down", 112, 78)),
            H9(3, 2.5, pitch=[0, 0, 2, 2, 4, 4, 7, 7, 7]),
            D4(4, 3.5, pitch=[7, 4, 2, 0]),
        ], vel=93, acc=22, soft=24),
        P("V8 Voodoo", 144, 2, ["v8", "crazy"], ["X.o.X.oxX.o.X.o.", "X.o.X.o.X.oxX.o."], [
            H9(1, 2.0, vel=("up", 75, 118), pitch=("steps", [0, 4])),
            D5(2, 0.5, pitch=[0, 2, 4, 7, 7]),
            H8(2, 3.5, vel=("down", 118, 76)),
        ], vel=95, acc=22, soft=26),
        P("Diesel Demon", 145, 4, ["crazy", "detroit"], ["X.o.X.oxX.o.X.o.", "X.oxX.o.X.o.X.ox"] * 2, [
            D5(1, 2.0),
            H8(1, 3.5, pitch=7),
            D9(2, 2.0, vel=("down", 120, 80)),
            H9(3, 0.5, pitch=("steps", [0, 4, 7, 4])),
            D5(4, 1.0, pitch=[0, 2, 4, 7, 7]),
            R(4, 2.5, 64, 16, vel=("up", 75, 120), pitch=("ramp", 0, 7)),
        ], vel=96, acc=24, soft=28),
    ],
    # ------------------------------------------------------------------ 10
    # jerk: twitches and holes, 1/32 stabs, medium velocity
    "SPASMUS": [
        P("Twitch", 150, 2, ["jerk", "holes"], ["x.x.x...x.x.x.x.", "x.x.x.x.x...x..."], [
            D3(2, 3.5),
        ], vel=92),
        P("Nerve Jerk", 148, 2, ["jerk", "holes"], ["x.x...x.x.x.x...", "x...x.x.x.x..x.."], [
            D3(1, 1.0),
            D4(2, 3.5),
        ], vel=90),
        P("Seizure Step", 150, 2, ["jerk", "holes"], ["x.x.x...x.x.....", "x...x.x.x.x.x.x."], [
            D5(1, 1.5),
            D3(2, 2.25),
        ], vel=90),
        P("Glitch Limb", 146, 4, ["glitch", "holes"],
          ["x.x...x.x.x.x...", "x...x.x.x...x.x.", "x.x...x.x.x.x...", "x.x.x..........."], [
            D3(1, 2.25),
            D5(2, 0.5),
            D5(4, 1.0),
            D4(4, 3.5),
        ], vel=92),
        P("Stop Drop Rot", 148, 2, ["stop", "jerk"], ["x.x.x...........", "x.x.x.x.....x.x."], [
            D5(1, 3.0 - 0.5),
            D3(2, 1.25),
        ], vel=90),
        P("Tic Tac Hex", 152, 4, ["tic", "jerk"], ["x.x.x...x...x.x.", "x...x.x.x.x...x."] * 2, [
            D3(1, 1.25),
            D5(2, 0.5),
            D3(3, 2.75),
            D5(4, 1.5),
            D4(4, 3.5),
        ], vel=92),
        P("Convulsio", 150, 2, ["convulse", "holes"], ["x...x.x...x.x...", "..x.x.x...x.x.x."], [
            D3(1, 2.25),
            D5(2, 0.5),
            D3(2, 2.75),
            D4(2, 3.5),
        ], vel=94),
        P("Short Circuit", 155, 4, ["crazy", "glitch"],
          ["x.x...x.x.x...x.", "x...x.x.x...x.x.", "x.x...x.x.x...x.", "x.x.x..........."], [
            D3(1, 1.25),
            D5(1, 2.5),
            D3(2, 0.75),
            D5(2, 2.5),
            D5(3, 1.5),
            D5(4, 0.5),
            D9(4, 2.0),
        ], vel=93),
    ],
    # ------------------------------------------------------------------ 11
    # dark half-time / memphis (60-80 BPM): 1/16 backbone at half tempo, long fills, +12/-12
    "REQUIEM": [
        P("Lacrimosa Hat", 70, 1, ["half-time", "1/32"], [S], [
            D4(1, 3.5),
        ], vel=95),
        P("Dies Irae Drip", 72, 2, ["memphis", "fill"], [S, S], [
            T4(1, 1.5),
            R(2, 2.0, 32, 16, pitch=("steps", [0, 12])),
        ], vel=96),
        P("Syrup Mass", 66, 2, ["syrup", "fill"], [S, SG4], [
            T4(1, 2.5),
            R(2, 2.0, 32, 12, pitch=("steps", [0, -12])),
        ], vel=94),
        P("Funeral Crawl", 62, 1, ["slow", "1/32"], [SG4], [
            R(1, 2.5, 32, 12, pitch=("steps", [0, 12])),
        ], vel=92),
        P("Memento Mori", 74, 2, ["memphis", "pitch"], ["XxxxXxxxXxxxXxxx", S], [
            T4(1, 2.5, pitch=12),
            R(2, 2.0, 32, 16, pitch=("steps", [0, 12, 0, -12])),
        ], vel=95, acc=8),
        P("Black Candle", 70, 2, ["dark", "memphis"], ["xxxoxxxoxxxoxxxo", SG4], [
            T4(1, 0.5),
            T7(2, 1.0),
            D4(2, 3.5, pitch=-12),
        ], vel=96, soft=14),
        P("Slow Exhumation", 64, 4, ["exhume", "long fill"], [S, S, S, S], [
            D4(1, 3.5),
            T4(2, 1.5),
            T7(3, 2.0, pitch=12),
            R(4, 1.0, 32, 24, pitch=("steps", [0, 12, -12])),
        ], vel=94),
        P("Codeine Crypt", 78, 2, ["crazy", "memphis"], ["XxxxXxxxXxxxXxxx", S], [
            T7(1, 1.0, pitch=("steps", [0, 12])),
            D4(1, 3.5, pitch=-12),
            D9(2, 0.5),
            R(2, 2.0, 24, 12, pitch=("steps", [12, 0, -12])),
        ], vel=95, acc=8),
    ],
    # ------------------------------------------------------------------ 12
    # chaos: rolls that change speed half-way, pitch drops, holes
    "MANIA": [
        P("Insanus", 155, 2, ["chaos", "speed change"], [E, G4], [
            R(1, 1.5, [(24, 3), (64, 5)], vel=("up", 85, 120)),
            R(2, 2.0, [(24, 2), (64, 4), (96, 6)], vel=("down", 120, 80), pitch=DOWN12),
        ], vel=100),
        P("Straitjacket", 160, 2, ["chaos", "pitch drop"], [EA, K2], [
            H9(1, 0.5),
            R(1, 2.5, [(32, 4), (96, 7)], pitch=("ramp", 12, -12)),
            R(2, 1.5, [(24, 3), (48, 3), (96, 7)], vel=("up", 80, 124)),
        ], vel=100, acc=10),
        P("Rabies", 158, 2, ["chaos", "pitch drop"], [E, G34], [
            R(1, 1.0, [(24, 3), (64, 5)], vel=("up", 85, 120)),
            R(2, 1.5, [(32, 4), (96, 13)], pitch=("ramp", 12, -12)),
        ], vel=100),
        P("Padded Room", 150, 4, ["chaos", "speed change"], [EA, G3, EA, G34], [
            R(1, 1.5, [(24, 3), (64, 5)]),
            R(2, 0.5, [(24, 2), (96, 9)], vel=("down", 118, 80)),
            R(3, 2.5, [(32, 4), (64, 5)], pitch=DOWN12),
            R(4, 1.0, [(24, 3), (64, 8), (96, 13)], vel=("up", 80, 124), pitch=("ramp", -12, 12)),
        ], vel=98, acc=10),
        P("Ego Death", 165, 2, ["chaos", "ego"], [E, K2], [
            R(1, 0.5, [(24, 3), (96, 7)]),
            H9(1, 2.0, pitch=("ramp", 12, 0)),
            R(2, 1.0, [(32, 3), (64, 6), (96, 10)], vel=("down", 124, 70), pitch=DOWN12),
        ], vel=102),
        P("13th Floor", 162, 4, ["chaos", "13"], [EA, EA, G3, EA], [
            R(1, 1.0, [(24, 4), (64, 5)]),
            R(2, 2.0, [(32, 4), (96, 13)], pitch=("ramp", 12, -12)),
            H9(3, 0.5, vel=("up", 80, 120)),
            R(3, 3.0, [(24, 3), (96, 7)]),
            R(4, 1.0, [(24, 3), (64, 8), (96, 18)], vel=("up", 75, 127), pitch=UP12),
        ], vel=100, acc=10),
        P("Psycho Loop", 168, 1, ["loop", "crazy"], [EA], [
            R(1, 1.0, [(24, 3), (64, 5)], pitch=UP12),
            R(1, 2.5, [(32, 2), (96, 7)], vel=("down", 120, 80), pitch=("ramp", 12, -12)),
        ], vel=104, acc=10),
        P("Unhinged", 170, 4, ["crazy", "chaos"], [EA, "X.x.X...X.x.X.x.", EA, G34], [
            R(1, 0.5, [(24, 3), (96, 7)], pitch=UP12),
            H8(1, 3.5, pitch=-12),
            R(2, 1.0, [(32, 4), (64, 4), (96, 7)], vel=("up", 80, 127)),
            R(3, 0.5, [(24, 2), (96, 9)]),
            R(3, 2.5, [(24, 3), (64, 5)], vel=("down", 125, 70), pitch=DOWN12),
            R(4, 1.0, [(24, 3), (48, 6), (64, 8), (96, 12)], vel=("up", 70, 127), pitch=("ramp", -12, 12)),
        ], vel=102, acc=10),
    ],
}
