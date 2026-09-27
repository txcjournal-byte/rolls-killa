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
            R(2, 3.0, 24, 3),
        ], vel=116),
    ],
    # ------------------------------------------------------------------ 2
    "LIBER TRAP": [
        P("Codex Trappus", 140, 2, ["triplet", "classic"], E8, [
            R(1, 1.5, 24, 4),
            R(2, 3.0, 32, 4),
        ], vel=95),
        P("Bando Evangelium", 140, 4, ["triplet", "1/64", "fill"], A16, [
            R(1, 3.5, 24, 3),
            R(2, 1.5, 24, 4),
            R(3, 3.5, 32, 4),
            R(4, 3.0, 64, 16, pitch=("ramp", 0, 12)),
        ], vel=92, acc=8),
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
            R(2, 1.5, 64, 8),
            R(2, 3.0, 96, 24, vel=("up", 80, 125), pitch=("steps", [0, 12])),
        ], vel=102, acc=10),
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
    ],
    # ------------------------------------------------------------------ 7
    "DELIRIUM": [
        P("Fever Dream", 150, 2, ["1/64", "melodic"], "xxoxxxoxxxoxxxox", [
            R(1, 1.5, 64, 8, vel=("up", 58, 78), pitch=("ramp", 0, 12)),
            R(2, 2.5, 48, 6, pitch=[0, 7, 12, 19, 12, 7]),
            R(2, 3.5, 96, 12, pitch=("ramp", 12, 24)),
        ], vel=70, soft=12),
        P("Brain Rot", 145, 2, ["1/96", "pitch"], "xxxoxxxoxxxoxxxo", [
            R(1, 0.5, 64, 8, pitch=("steps", [0, 12])),
            R(1, 2.5, 96, 12, pitch=("ramp", 0, 19)),
            R(2, 1.0, 48, 6, vel=("down", 78, 58)),
            R(2, 3.0, 64, 16, pitch=("ramp", 24, 0)),
        ], vel=68, soft=10),
    ],
    # ------------------------------------------------------------------ 8
    "NECRODRILL": [
        P("Grave Slide", 142, 1, ["drill", "skip"], "XxXxXxXx", [
            R(1, 1.5, 24, 3),
            R(1, 3.25, 24, 3),
        ], vel=92, acc=20),
        P("Corpse Skip", 144, 2, ["drill", "skip"], "X.xxX.x.X.xxX.x.", [
            R(1, 1.75, 24, 3),
            R(2, 0.75, 24, 3),
            R(2, 3.5, 24, 3, vel=[110, 90, 90]),
        ], vel=90, acc=22),
    ],
    # ------------------------------------------------------------------ 9
    "MOTOR MORTIS": [
        P("Rust Bounce", 138, 2, ["detroit", "bounce"], "XoxoXoxoXoxoXoxo", [
            R(1, 1.25, 32, 2),
            R(2, 2.5, 32, 3, pitch=[0, 4, 7]),
        ], vel=95, acc=20, soft=22),
        P("Engine Exorcism", 135, 2, ["detroit", "1/32"], "X.xoX.xoX.xoX.xo", [
            R(1, 0.75, 32, 4, vel=("down", 112, 80)),
            R(1, 3.0, 64, 4),
            R(2, 1.5, 32, 3, pitch=2),
            R(2, 3.25, 32, 4, pitch=[0, 4, 7, 7]),
        ], vel=94, acc=20, soft=22),
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
    ],
    # ------------------------------------------------------------------ 11
    "REQUIEM": [
        P("Lacrimosa Hat", 70, 1, ["half-time", "1/32"], E16, [
            R(1, 3.0, 32, 8),
        ], vel=95),
        P("Dies Irae Drip", 72, 2, ["memphis", "fill"], E16, [
            R(1, 1.5, 24, 3),
            R(2, 2.0, 32, 16, pitch=("steps", [0, 12])),
        ], vel=96),
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
    ],
}
