"""Rolls Killa factory presets - authored by hand (see build_presets.py for the pattern language).

Order inside a category = from the calmest (1) to the craziest (8).
Numbers to aim for (spec 3.2): triplet 1/24 rolls ~43 %, ~0.73 rolls per bar, 80 % flat velocity,
roll length mostly 2/3 beat, rolls start on the beat (58 %) or the 1/8 (36 %).
"""

from build_presets import P, R

E8 = "xxxxxxxx"                 # straight 1/8
E16 = "xxxxxxxxxxxxxxxx"        # straight 1/16
A16 = "XxxxXxxxXxxxXxxx"        # 1/16 with beat accents
T12 = "x.xx.xx.xx.x"            # triplet skip grid

LIBRARY = {
    # ------------------------------------------------------------------ 1
    "PRIMITIVUS": [
        P("Club Hit", 140, 2, ["1/8", "raw"], E8, [
            R(2, 3.5, 32, 4),
        ], vel=118),
        P("Stone Age", 150, 2, ["1/16", "raw"], E16, [
            R(2, 3.0, 24, 4),
        ], vel=116),
        P("Caveman Tick", 145, 2, ["1/16", "accent"], "XxXxXxXxXxXxXxXx", [
            R(2, 3.5, 32, 2),
        ], vel=114, acc=10),
        P("One Blow", 135, 4, ["1/8", "raw"], E8, [
            R(2, 3.5, 24, 3),
            R(4, 3.0, 32, 4),
        ], vel=120),
        P("Raw Bone", 150, 4, ["1/16", "roll"], E16, [
            R(1, 1.5, 32, 4),
            R(4, 2.0, 24, 4),
        ], vel=112),
        P("Blunt Force", 155, 2, ["hard", "1/32"], ["XxxxXxxxXxxxXxxx", "XxxxXxxxXxxxXx.."], [
            R(2, 3.5, 32, 4),
        ], vel=116, acc=11),
        P("Neanderthal", 140, 4, ["switch", "raw"], [E8, E8, E8, E16], [
            R(2, 1.5, 24, 3),
            R(4, 3.0, 32, 4),
        ], vel=122),
        P("Skull Tap", 160, 4, ["crazy", "1/32"], "X.xxX.xxX.xxX.xx", [
            R(1, 3.5, 32, 4),
            R(3, 2.5, 24, 4),
        ], vel=115, acc=12),
    ],
    # ------------------------------------------------------------------ 2
    "LIBER TRAP": [
        P("Codex Trappus", 140, 2, ["triplet", "classic"], E8, [
            R(1, 1.5, 24, 4),
            R(2, 3.0, 24, 4),
        ], vel=95),
        P("Bando Evangelium", 140, 4, ["triplet", "1/64", "fill"], A16, [
            R(1, 3.5, 24, 3),
            R(2, 1.5, 24, 4),
            R(3, 3.5, 32, 4),
            R(4, 3.0, 64, 16, pitch=("ramp", 0, 12)),
        ], vel=92, acc=8),
        P("Psalm 808", 140, 4, ["triplet", "fill"], E8, [
            R(1, 1.5, 24, 4),
            R(2, 3.0, 24, 4),
            R(3, 1.5, 24, 4),
            R(4, 3.0, 64, 16, pitch=("ramp", 0, 12)),
        ], vel=96),
        P("Gospel of Drip", 145, 2, ["triplet", "1/32"], E16, [
            R(1, 1.5, 24, 4),
            R(2, 2.5, 24, 4),
        ], vel=94),
        P("Ave Trappa", 138, 4, ["triplet", "ramp"], E8, [
            R(1, 3.5, 24, 3),
            R(2, 1.5, 24, 4, vel=("up", 76, 100)),
            R(3, 3.0, 32, 4),
            R(4, 2.0, 24, 6),
            R(4, 3.5, 64, 8, pitch=12),
        ], vel=95),
        P("Liber Choppa", 142, 4, ["choppy", "fill"], A16, [
            R(1, 1.0, 24, 4),
            R(2, 2.5, 24, 4),
            R(3, 1.0, 24, 4),
            R(4, 3.0, 64, 16, vel=("up", 80, 110), pitch=("steps", [0, 12])),
        ], vel=92, acc=8),
        P("Opus Stash", 146, 2, ["busy", "1/64"], E16, [
            R(1, 2.5, 32, 4),
            R(2, 1.5, 24, 4),
            R(2, 3.0, 64, 16, pitch=("ramp", 0, 12)),
        ], vel=95),
        P("Trap Sacrament", 150, 4, ["crazy", "fill"], A16, [
            R(1, 1.5, 24, 4),
            R(2, 2.0, 24, 4, pitch=[0, 0, 12, 12]),
            R(3, 0.5, 24, 4),
            R(3, 2.5, 24, 6),
            R(4, 3.0, 64, 16, vel=("up", 78, 118), pitch=("ramp", 0, 12)),
        ], vel=96, acc=8),
    ],
    # ------------------------------------------------------------------ 3
    "TRINITAS": [
        P("Tres Diaboli", 145, 2, ["triplet"], T12, [
            R(1, 1.0, 24, 4),
            R(2, 2.0, 24, 4, pitch=("steps", [0, 5])),
        ], vel=96),
        P("Holy Triplet", 140, 1, ["triplet", "2/3"], E8, [
            R(1, 1.5, 24, 4),
        ], vel=98),
        P("Trinity Knife", 150, 2, ["1/48"], T12, [
            R(1, 1.0, 48, 8),
            R(2, 2.0, 48, 8),
        ], vel=97),
        P("Tri-Hex", 145, 4, ["triplet", "fill"], E8, [
            R(1, 1.5, 24, 4),
            R(2, 2.0, 24, 4),
            R(3, 1.5, 24, 4),
            R(4, 2.0, 24, 6),
        ], vel=96),
        P("Third Eye Roll", 140, 2, ["1/48", "pitch"], T12, [
            R(1, 1.0, 48, 8, pitch=("steps", [0, 3, 5])),
            R(2, 2.0, 24, 4, pitch=[0, 3, 5, 5]),
        ], vel=95),
        P("Triple Six", 155, 4, ["fill", "crazy"], T12, [
            R(1, 1.0, 24, 4),
            R(2, 2.0, 48, 8),
            R(3, 1.0, 24, 4, pitch=("steps", [0, 5])),
            R(4, 1.0, 24, 6),
            R(4, 3.0, 48, 12, pitch=("ramp", 0, 5)),
        ], vel=98),
        P("Trine Tick", 150, 1, ["skip"], "x.x.xxx.x.xx", [
            R(1, 1.5, 24, 3),
        ], vel=96),
        P("Pater Triplex", 150, 4, ["1/48", "fill", "crazy"], [T12, E8, T12, E8], [
            R(1, 1.0, 48, 8),
            R(1, 3.0, 24, 4),
            R(2, 1.5, 24, 4, pitch=[0, 3, 3, 5]),
            R(3, 2.0, 48, 12),
            R(4, 1.0, 24, 4),
            R(4, 2.5, 48, 18, pitch=("ramp", 0, 5)),
        ], vel=97),
    ],
    # ------------------------------------------------------------------ 4
    "VOMITORIUM": [
        P("Bile Rage", 160, 2, ["rage", "1/16"], E16, [
            R(2, 3.5, 32, 3),
        ], vel=115),
        P("Puke Pit", 155, 2, ["rage", "1/32"], A16, [
            R(1, 2.5, 32, 2),
            R(2, 3.0, 32, 4, pitch=("steps", [0, 3])),
        ], vel=112, acc=8),
        P("Acid Reflux", 158, 2, ["rage", "1/32"], E16, [
            R(1, 3.5, 32, 2),
            R(2, 3.0, 32, 4, pitch=("steps", [0, 2])),
        ], vel=116),
        P("Mosh Vomit", 160, 4, ["rage", "pitch"], A16, [
            R(2, 3.5, 32, 3),
            R(4, 3.0, 32, 4, pitch=[0, 0, 5, 5]),
        ], vel=114, acc=6),
        P("Gutspill", 155, 4, ["rage", "burst"], ["XxxxXxxxXxxxXxxx", "XxxxXxxxXx.xXxxx"], [
            R(1, 1.5, 32, 2),
            R(2, 3.5, 32, 4),
            R(4, 3.0, 32, 8, pitch=("steps", [0, 3])),
        ], vel=113, acc=7),
        P("Stomach Pump", 162, 2, ["rage", "pump"], E16, [
            R(1, 1.0, 32, 3),
            R(2, 2.5, 32, 4, pitch=("ramp", 0, 5)),
        ], vel=118),
        P("Toxic Spew", 165, 4, ["rage", "1/64"], A16, [
            R(1, 2.5, 32, 3),
            R(2, 3.5, 64, 4),
            R(4, 2.0, 32, 4, pitch=2),
            R(4, 3.5, 64, 8, pitch=5),
        ], vel=115, acc=7),
        P("Retch Riot", 164, 2, ["crazy", "rage"], A16, [
            R(1, 2.0, 32, 4, pitch=3),
            R(2, 1.0, 32, 3),
            R(2, 2.5, 64, 8, pitch=("ramp", 0, 5)),
        ], vel=117, acc=6),
    ],
    # ------------------------------------------------------------------ 5
    "BLAST RITUAL": [
        P("Blastfemy", 150, 2, ["1/64", "fill"], E16, [
            R(1, 3.0, 64, 8, vel=("up", 82, 112)),
            R(2, 2.0, 64, 24, vel=("up", 75, 124), pitch=("ramp", 0, 12)),
        ], vel=100),
        P("Thrash Chapel", 155, 2, ["1/96", "fill"], A16, [
            R(1, 1.0, 96, 12),
            R(1, 3.0, 64, 8, vel=("up", 85, 115)),
            R(2, 3.0, 96, 24, vel=("up", 80, 125), pitch=("steps", [0, 12])),
        ], vel=102, acc=10),
        P("Speed Kills", 160, 2, ["1/64", "fill"], E16, [
            R(1, 2.0, 64, 8, vel=("up", 85, 115)),
            R(2, 2.5, 64, 24, vel=("up", 78, 122), pitch=("ramp", 0, 12)),
        ], vel=104),
        P("Shredder Mass", 150, 4, ["1/96", "long fill"], A16, [
            R(2, 1.0, 96, 12),
            R(2, 3.0, 64, 16, vel=("up", 84, 120)),
            R(3, 2.0, 96, 24),
            R(4, 2.0, 64, 32, vel=("up", 75, 125), pitch=("ramp", 0, 12)),
        ], vel=102, acc=10),
        P("Riff Carnage", 155, 2, ["1/96", "riff"], "XxXxXxXxXxXxXxXx", [
            R(2, 1.0, 64, 16, vel=("up", 80, 120)),
            R(2, 3.0, 96, 24, pitch=("steps", [0, 12])),
        ], vel=100, acc=12),
        P("Headbang Hex", 145, 4, ["headbang", "fill"], E16, [
            R(1, 1.0, 64, 8),
            R(2, 2.0, 96, 24, vel=("up", 80, 120)),
            R(4, 1.0, 96, 24),
            R(4, 3.0, 64, 16, vel=("up", 80, 122), pitch=("ramp", 0, 12)),
        ], vel=100),
        P("Mach Satanas", 165, 2, ["fastest", "long fill"], A16, [
            R(1, 0.5, 96, 12),
            R(1, 2.0, 64, 16, vel=("up", 80, 120)),
            R(2, 2.0, 64, 32, vel=("up", 70, 127), pitch=("ramp", 0, 12)),
        ], vel=105, acc=10),
        P("Pit Liturgy", 160, 4, ["crazy", "long fill"], A16, [
            R(1, 1.0, 96, 24, vel=("up", 80, 120)),
            R(1, 3.0, 64, 8),
            R(2, 2.0, 96, 36),
            R(3, 1.0, 64, 16),
            R(3, 3.0, 96, 12, pitch=("steps", [0, 12])),
            R(4, 1.5, 96, 60, vel=("up", 70, 127), pitch=("ramp", 0, 12)),
        ], vel=104, acc=10),
    ],
    # ------------------------------------------------------------------ 6
    "AURA FARM": [
        P("+1000 Aura", 150, 2, ["plugg", "soft"], E8, [
            R(2, 3.0, 24, 4, pitch=[0, 3, 5, 7]),
        ], vel=80),
        P("Aura Debt", 145, 2, ["plugg", "melodic"], "xxxxoxxx", [
            R(1, 1.5, 24, 4, pitch=("steps", [0, 5])),
            R(2, 3.5, 32, 4, pitch=[0, 3, 5, 7]),
        ], vel=78, soft=14),
        P("Halo Drip", 148, 2, ["plugg", "melodic"], "xxxxoxxx", [
            R(1, 3.5, 24, 3, pitch=[0, 3, 5]),
            R(2, 2.0, 24, 4, pitch=[0, 5, 7, 7]),
        ], vel=80, soft=14),
        P("Glow Up Ghost", 152, 4, ["plugg", "soft"], E8, [
            R(2, 1.5, 24, 4, pitch=("steps", [0, 7])),
            R(4, 3.0, 24, 6, pitch=[0, 3, 5, 7, 5, 3]),
        ], vel=78),
        P("Mog Mode", 150, 4, ["pluggnb", "melodic"], "xoxxxoxx", [
            R(1, 1.5, 24, 4, pitch=[0, 0, 5, 5]),
            R(2, 3.5, 32, 4, pitch=("ramp", 0, 7)),
            R(3, 1.5, 24, 4, pitch=[0, 3, 5, 7]),
        ], vel=82, soft=16),
        P("Aura Leak", 145, 2, ["pluggnb", "1/32"], "xxoxxxoxxxoxxxox", [
            R(1, 1.5, 24, 4, pitch=[0, 5, 7, 12]),
            R(2, 3.0, 32, 8, vel=("up", 70, 92), pitch=("steps", [0, 3, 5, 7])),
        ], vel=76, soft=12),
        P("Main Character", 158, 4, ["melodic", "main"], E8, [
            R(1, 3.0, 24, 4, pitch=[0, 3, 5, 7]),
            R(3, 3.0, 24, 4, pitch=[0, 5, 7, 12]),
            R(4, 2.0, 32, 8, pitch=("ramp", 0, 7)),
        ], vel=84),
        P("Cloud Sigil", 155, 2, ["crazy", "melodic"], "xoxxxoxxxoxxxoxx", [
            R(1, 2.5, 24, 4, pitch=[0, 5, 7, 12]),
            R(2, 1.0, 32, 4, pitch=("ramp", 12, 0)),
            R(2, 3.0, 24, 6, vel=("up", 70, 92), pitch=[0, 3, 5, 7, 10, 12]),
        ], vel=80, soft=14),
    ],
    # ------------------------------------------------------------------ 7
    "DELIRIUM": [
        P("Fever Dream", 150, 2, ["1/64", "melodic"], "xxoxxxoxxxoxxxox", [
            R(1, 1.5, 64, 8, vel=("up", 58, 78), pitch=("ramp", 0, 12)),
            R(2, 3.5, 96, 12, pitch=("ramp", 12, 24)),
        ], vel=70, soft=12),
        P("Brain Rot", 145, 2, ["1/96", "pitch"], "xxxoxxxoxxxoxxxo", [
            R(1, 0.5, 64, 8, pitch=("steps", [0, 12])),
            R(1, 2.5, 96, 12, pitch=("ramp", 0, 19)),
            R(2, 3.0, 64, 16, pitch=("ramp", 24, 0)),
        ], vel=68, soft=10),
        P("Lucid Snake", 148, 2, ["snake", "melodic"], "xxoxxxoxxxoxxxox", [
            R(1, 3.0, 96, 12, pitch=("ramp", 7, 19)),
            R(2, 2.0, 64, 16, pitch=("ramp", 0, 24)),
        ], vel=70, soft=12),
        P("Glitter Psychosis", 150, 4, ["glitter", "1/96"], "xxxoxxxoxxxoxxxo", [
            R(1, 1.5, 96, 12, pitch=("ramp", 12, 24)),
            R(2, 2.5, 48, 6, pitch=[0, 7, 12, 7, 12, 19]),
            R(3, 1.5, 96, 12, pitch=("ramp", 0, 19)),
            R(4, 2.5, 96, 36, vel=("up", 52, 80), pitch=("ramp", 0, 24)),
        ], vel=66, soft=10),
        P("Pill Ladder", 145, 2, ["ladder", "pitch"], "xxxoxxxoxxxoxxxo", [
            R(1, 1.0, 48, 12, pitch=("steps", [0, 3, 7, 12, 15, 19])),
            R(2, 1.0, 64, 16, pitch=("steps", [0, 5, 12, 17])),
            R(2, 3.0, 96, 24, pitch=("ramp", 12, 24)),
        ], vel=68, soft=10),
        P("Melting Clock", 142, 4, ["melt", "pitch drop"], "xxoxxxoxxxoxxxox", [
            R(1, 2.0, 64, 16, vel=("down", 78, 52), pitch=("ramp", 19, 0)),
            R(3, 2.5, 64, 8, pitch=12),
            R(4, 1.0, 96, 24, pitch=("ramp", 24, 0)),
            R(4, 3.0, 64, 16, vel=("up", 55, 80), pitch=("ramp", 0, 24)),
        ], vel=64, soft=10),
        P("Echo Asylum", 150, 2, ["echo", "crazy"], "xoxxxoxxxoxxxoxx", [
            R(1, 0.5, 64, 8, pitch=12),
            R(1, 2.0, 96, 12, pitch=24),
            R(2, 2.0, 96, 24, vel=("down", 78, 50), pitch=("ramp", 24, 7)),
        ], vel=70, soft=14),
        P("Mind Siphon", 155, 4, ["crazy", "1/96"], "xxoxxxoxxxoxxxox", [
            R(1, 2.0, 64, 16, pitch=("steps", [0, 7, 12, 19, 24, 19, 12, 7])),
            R(2, 1.0, 48, 12, pitch=("steps", [0, 12, 24])),
            R(2, 3.0, 96, 24, vel=("up", 52, 80), pitch=("ramp", 24, 0)),
            R(3, 2.0, 96, 24, pitch=("ramp", 0, 24)),
            R(4, 0.5, 48, 6, pitch=[24, 19, 12, 7, 0, -5]),
            R(4, 2.0, 96, 48, vel=("up", 50, 80), pitch=("ramp", 0, 24)),
        ], vel=66, soft=10),
    ],
    # ------------------------------------------------------------------ 8
    "NECRODRILL": [
        P("Grave Slide", 142, 1, ["drill", "skip"], "XxXxXxXx", [
            R(1, 3.25, 24, 3),
        ], vel=92, acc=20),
        P("Corpse Skip", 144, 2, ["drill", "skip"], "X.xxX.x.X.xxX.x.", [
            R(1, 1.75, 24, 3),
            R(2, 3.5, 24, 3, vel=[110, 90, 90]),
        ], vel=90, acc=22),
        P("Necro Block", 143, 2, ["drill", "skip"], "X.xxX.x.X.xxX.x.", [
            R(1, 1.75, 24, 3),
            R(2, 3.25, 24, 3),
        ], vel=90, acc=24),
        P("Tombstone Step", 140, 4, ["drill", "step"], "XxXxXxXx", [
            R(1, 1.5, 24, 3),
            R(2, 0.75, 24, 3),
            R(4, 2.25, 24, 3),
            R(4, 3.5, 24, 3, vel=[112, 92, 92]),
        ], vel=92, acc=20),
        P("Cold Casket", 142, 2, ["drill", "cold"], "X.xxx.xxX.xxx.x.", [
            R(1, 0.75, 24, 3),
            R(2, 1.75, 24, 3, pitch=[0, 0, 2]),
        ], vel=88, acc=26),
        P("Crypt Runner", 144, 4, ["drill", "runner"], "X.xX.xX.X.xX.xX.", [
            R(1, 1.25, 24, 3),
            R(2, 2.75, 24, 3),
            R(3, 0.5, 24, 3),
            R(4, 3.25, 24, 3, vel=[115, 95, 95]),
        ], vel=90, acc=22),
        P("Bone Saw", 145, 2, ["drill", "saw"], "X.xxX.xxX.xxX.xx", [
            R(1, 0.75, 24, 3),
            R(1, 3.25, 24, 3),
            R(2, 2.75, 24, 3),
        ], vel=92, acc=22),
        P("Morgue Opps", 145, 4, ["drill", "crazy"], "X.xxX.x.X.xxX.x.", [
            R(1, 0.75, 24, 3),
            R(1, 3.5, 24, 3),
            R(2, 1.25, 24, 3, pitch=[0, 0, 2]),
            R(3, 1.75, 24, 6),
            R(4, 2.25, 24, 3),
            R(4, 3.25, 24, 4, vel=("up", 85, 115)),
        ], vel=90, acc=25),
    ],
    # ------------------------------------------------------------------ 9
    "MOTOR MORTIS": [
        P("Rust Bounce", 138, 2, ["detroit", "bounce"], "XoxoXoxoXoxoXoxo", [
            R(1, 1.25, 32, 2),
            R(2, 2.5, 32, 3, pitch=[0, 4, 7]),
        ], vel=95, acc=20, soft=22),
        P("Engine Exorcism", 135, 2, ["detroit", "1/32"], "X.xoX.xoX.xoX.xo", [
            R(1, 0.75, 32, 4, vel=("down", 112, 80)),
            R(2, 1.5, 32, 3, pitch=2),
            R(2, 3.25, 32, 4, pitch=[0, 4, 7, 7]),
        ], vel=94, acc=20, soft=22),
        P("Chrome Carcass", 140, 2, ["detroit", "bounce"], "XoxoXoxoXoxoXoxo", [
            R(1, 2.25, 32, 2, pitch=4),
            R(2, 3.0, 32, 4, pitch=[0, 4, 7, 4]),
        ], vel=94, acc=22, soft=24),
        P("Assembly Hex", 136, 4, ["detroit", "1/32"], "X.xoX.xoX.xoX.xo", [
            R(1, 1.25, 32, 2),
            R(2, 2.5, 32, 3, pitch=2),
            R(4, 2.0, 32, 4, vel=("down", 115, 80)),
            R(4, 3.5, 64, 4, pitch=7),
        ], vel=95, acc=22, soft=26),
        P("Burnout Ritual", 142, 2, ["burnout", "bounce"], "XoxxXoxxXoxxXoxx", [
            R(1, 1.75, 32, 3, pitch=[0, 2, 4]),
            R(2, 1.0, 64, 8, vel=("up", 78, 115)),
            R(2, 2.75, 32, 3, pitch=7),
        ], vel=92, acc=22, soft=26),
        P("Scrapyard Knock", 138, 4, ["detroit", "knock"], "X.xoX.oxX.xoX.ox", [
            R(1, 2.25, 32, 3, pitch=4),
            R(2, 1.5, 32, 4, vel=("down", 112, 78)),
            R(3, 3.0, 64, 8, pitch=[0, 0, 2, 2, 4, 4, 7, 7]),
            R(4, 3.25, 32, 4, pitch=[7, 4, 2, 0]),
        ], vel=93, acc=22, soft=24),
        P("V8 Voodoo", 144, 2, ["v8", "crazy"], "XoxoXoxoXoxoXoxo", [
            R(1, 2.0, 64, 8, vel=("up", 75, 118), pitch=("steps", [0, 4])),
            R(2, 1.25, 32, 4, pitch=[0, 2, 4, 7]),
            R(2, 3.0, 64, 8, vel=("down", 118, 76)),
        ], vel=95, acc=22, soft=26),
        P("Diesel Demon", 145, 4, ["crazy", "detroit"], ["X.xoX.xoX.xoX.xo", "XoxoX.xoXoxoX.x."], [
            R(1, 2.25, 32, 4),
            R(2, 3.0, 32, 8, vel=("down", 120, 80)),
            R(3, 0.5, 64, 8, pitch=("steps", [0, 4, 7, 4])),
            R(4, 1.0, 32, 4, pitch=[0, 2, 4, 7]),
            R(4, 2.5, 64, 16, vel=("up", 75, 120), pitch=("ramp", 0, 7)),
        ], vel=96, acc=24, soft=28),
    ],
    # ------------------------------------------------------------------ 10
    "SPASMUS": [
        P("Twitch", 150, 2, ["jerk", "holes"], "xx.xx.x.xx.x.xx.", [
            R(1, 2.25, 32, 2),
            R(2, 3.5, 32, 3),
        ], vel=92),
        P("Nerve Jerk", 148, 2, ["jerk", "holes"], ["x.xxx..xx.x.x..x", "xx.x..xxx.xx..x."], [
            R(1, 1.0, 32, 3),
            R(2, 2.75, 32, 2),
        ], vel=90),
        P("Seizure Step", 150, 2, ["jerk", "holes"], ["x.xx..x.xx.x..x.", "xx..x.xx..x.x.x."], [
            R(1, 1.5, 32, 3),
            R(2, 2.25, 32, 2),
        ], vel=90),
        P("Glitch Limb", 146, 4, ["glitch", "holes"],
          ["xx.x.xx..x.xx.x.", "x.xx.x..xx.x.xx.", "xx.x.xx..x.xx.x.", "x.x..xx.x..x...."], [
            R(1, 2.25, 32, 2),
            R(2, 0.5, 32, 3),
            R(4, 1.0, 32, 4),
            R(4, 3.0, 32, 3),
        ], vel=92),
        P("Stop Drop Rot", 148, 2, ["stop", "jerk"], ["xxxx....xx.x..x.", "x.xx.x......xxxx"], [
            R(1, 3.0, 32, 4),
            R(2, 1.25, 32, 2),
        ], vel=90),
        P("Tic Tac Hex", 152, 4, ["tic", "jerk"], "x.x.xx.x.x.xx.x.", [
            R(1, 1.25, 32, 3),
            R(3, 0.75, 32, 3),
            R(4, 1.5, 32, 4),
            R(4, 3.25, 32, 2),
        ], vel=92),
        P("Convulsio", 150, 2, ["convulse", "holes"], ["x..xx.x..x.xx...", "..x.xx.x...x.x.x"], [
            R(1, 2.25, 32, 3),
            R(2, 0.5, 32, 3),
            R(2, 2.75, 32, 2),
        ], vel=94),
        P("Short Circuit", 155, 4, ["crazy", "glitch"],
          ["xx.x..xx.x..x.x.", "x..xx.x.xx...x.x", "xx.x..xx.x..x.x.", "x.x.x..x.x....xx"], [
            R(1, 1.25, 32, 2),
            R(2, 0.75, 32, 2),
            R(2, 2.5, 32, 4),
            R(3, 1.75, 32, 3),
            R(4, 0.5, 32, 3),
            R(4, 2.0, 32, 8),
        ], vel=93),
    ],
    # ------------------------------------------------------------------ 11
    "REQUIEM": [
        P("Lacrimosa Hat", 70, 1, ["half-time", "1/32"], E16, [
            R(1, 3.0, 24, 6),
        ], vel=95),
        P("Dies Irae Drip", 72, 2, ["memphis", "fill"], E16, [
            R(1, 1.5, 24, 4),
            R(2, 2.0, 32, 16, pitch=("steps", [0, 12])),
        ], vel=96),
        P("Syrup Mass", 66, 2, ["syrup", "fill"], E16, [
            R(1, 3.0, 24, 6),
            R(2, 2.0, 32, 16, pitch=("steps", [0, -12])),
        ], vel=94),
        P("Funeral Crawl", 62, 1, ["slow", "1/32"], E16, [
            R(1, 3.0, 32, 8, pitch=("steps", [0, 12])),
        ], vel=92),
        P("Memento Mori", 74, 2, ["memphis", "pitch"], A16, [
            R(1, 3.0, 24, 6, pitch=12),
            R(2, 2.0, 32, 16, pitch=("steps", [0, 12, 0, -12])),
        ], vel=95, acc=8),
        P("Black Candle", 70, 2, ["dark", "memphis"], "xxxoxxxoxxxoxxxo", [
            R(1, 0.5, 24, 4),
            R(2, 1.0, 24, 6),
            R(2, 3.0, 32, 8, pitch=-12),
        ], vel=96, soft=14),
        P("Slow Exhumation", 64, 4, ["exhume", "long fill"], E16, [
            R(1, 3.0, 32, 8),
            R(2, 1.5, 24, 4),
            R(3, 3.0, 24, 6, pitch=12),
            R(4, 1.0, 32, 24, pitch=("steps", [0, 12, -12])),
        ], vel=94),
        P("Codeine Crypt", 78, 2, ["crazy", "memphis"], A16, [
            R(1, 2.0, 24, 6, pitch=("steps", [0, 12])),
            R(2, 1.0, 32, 8),
            R(2, 2.5, 24, 9, pitch=("steps", [12, 0, -12])),
        ], vel=95, acc=8),
    ],
    # ------------------------------------------------------------------ 12
    "MANIA": [
        P("Insanus", 155, 2, ["chaos", "speed change"], E16, [
            R(1, 1.5, [(24, 3), (64, 4)], vel=("up", 85, 120)),
            R(2, 2.0, [(24, 2), (64, 4), (96, 6)], vel=("down", 120, 80), pitch=("ramp", 0, -12)),
        ], vel=100),
        P("Straitjacket", 160, 2, ["chaos", "pitch drop"], A16, [
            R(1, 0.75, 64, 6),
            R(1, 3.0, [(32, 4), (96, 6)], pitch=("ramp", 12, -12)),
            R(2, 1.5, [(24, 3), (48, 3), (96, 6)], vel=("up", 80, 124)),
        ], vel=100, acc=10),
        P("Rabies", 158, 2, ["chaos", "pitch drop"], E16, [
            R(1, 1.0, [(24, 3), (64, 4)], vel=("up", 85, 120)),
            R(2, 2.0, [(32, 4), (96, 12)], pitch=("ramp", 12, -12)),
        ], vel=100),
        P("Padded Room", 150, 4, ["chaos", "speed change"], A16, [
            R(1, 1.5, [(24, 3), (64, 4)]),
            R(2, 2.5, [(24, 2), (96, 8)], vel=("down", 118, 80)),
            R(3, 2.5, [(32, 4), (64, 4)], pitch=("ramp", 0, -12)),
            R(4, 1.5, [(24, 3), (64, 8), (96, 12)], vel=("up", 80, 124), pitch=("ramp", -12, 12)),
        ], vel=98, acc=10),
        P("Ego Death", 165, 2, ["chaos", "ego"], E16, [
            R(1, 0.5, [(24, 3), (96, 6)]),
            R(1, 2.25, 64, 8, pitch=("ramp", 12, 0)),
            R(2, 1.0, [(32, 3), (64, 6), (96, 9)], vel=("down", 124, 70), pitch=("ramp", 0, -12)),
        ], vel=102),
        P("13th Floor", 162, 4, ["chaos", "13"], A16, [
            R(1, 1.0, [(24, 4), (64, 6)]),
            R(2, 2.0, [(32, 4), (96, 12)], pitch=("ramp", 12, -12)),
            R(3, 2.5, [(24, 3), (48, 3), (96, 6)]),
            R(4, 1.0, [(24, 3), (64, 8), (96, 18)], vel=("up", 75, 127), pitch=("ramp", 0, 12)),
        ], vel=100, acc=10),
        P("Psycho Loop", 168, 1, ["loop", "crazy"], A16, [
            R(1, 1.0, [(24, 3), (64, 4)], pitch=("ramp", 0, 12)),
            R(1, 2.75, [(32, 2), (96, 6)], vel=("down", 120, 80), pitch=("ramp", 12, -12)),
        ], vel=104, acc=10),
        P("Unhinged", 170, 4, ["crazy", "chaos"], ["XxxxXxxxXxxxXxxx", "Xx.xXxx.XxxxX.xx"], [
            R(1, 0.5, [(24, 3), (96, 6)], pitch=("ramp", 0, 12)),
            R(2, 1.0, [(32, 4), (64, 4), (96, 6)], vel=("up", 80, 127)),
            R(2, 3.25, 64, 6, pitch=("ramp", 12, -12)),
            R(3, 2.5, [(24, 3), (64, 8)], vel=("down", 125, 70), pitch=("ramp", 0, -12)),
            R(4, 1.5, [(24, 3), (48, 6), (64, 8), (96, 12)], vel=("up", 70, 127), pitch=("ramp", -12, 12)),
        ], vel=102, acc=10),
    ],
}
