#!/usr/bin/env python3
"""Builds the Rolls Killa factory presets (Resources/Presets/Factory/*.json).

The presets are authored by hand in factory_presets.py with a tiny pattern language,
following the numbers from section 3.2/3.3 of the spec (no MIDI is copied from any kit).

    python tools/preset_authoring/build_presets.py          # validate + write JSON + print stats
    python tools/preset_authoring/build_presets.py --check  # validate only

Pattern language
----------------
P(name, bpm, bars, tags, base, rolls, vel=100, acc=12, soft=25)
  base  - hat grid, one string per bar (a single string is used for every bar).
          The string length sets the grid: 8 = 1/8, 16 = 1/16, 12 = 1/8 triplets.
          x = hit, X = accent (vel + acc), o = soft (vel - soft), . = rest
  rolls - list of R(...)

R(bar, pos, rate, n, vel=None, pitch=0)
  bar   - 1-based bar number
  pos   - start inside the bar in beats (0 = beat 1, 1.5 = the "and" of beat 2)
  rate  - 24 / 32 / 48 / 64 / 96, or a list of (rate, count) for rolls that change speed
  n     - number of notes (ignored when rate is a list)
  vel   - None = base velocity, int = flat, ("up", a, b), ("down", a, b) or an explicit list
  pitch - int, ("ramp", a, b), ("steps", [..]) held steps, or an explicit list
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(HERE, ".."))

from rollkit import (BEATS_PER_BAR, EPS, RATE_BY_DIVISION, ROLL_STEPS, ROLL_THRESHOLD,  # noqa: E402
                     Note, Stats, find_rolls, on_grid, rate_index)

CATEGORIES = ["PRIMITIVUS", "LIBER TRAP", "TRINITAS", "VOMITORIUM", "BLAST RITUAL", "AURA FARM",
              "DELIRIUM", "NECRODRILL", "MOTOR MORTIS", "SPASMUS", "REQUIEM", "MANIA"]
BPM_RANGE = {"PRIMITIVUS": (130, 160), "LIBER TRAP": (130, 150), "TRINITAS": (135, 155), "VOMITORIUM": (150, 165),
             "BLAST RITUAL": (140, 165), "AURA FARM": (140, 160), "DELIRIUM": (140, 155), "NECRODRILL": (140, 145),
             "MOTOR MORTIS": (130, 145), "SPASMUS": (140, 155), "REQUIEM": (60, 80), "MANIA": (140, 170)}
SKIP_CATEGORIES = {"NECRODRILL", "MOTOR MORTIS", "SPASMUS", "MANIA"}
MIXED_CATEGORIES = {"MANIA"}


class R:
    def __init__(self, bar, pos, rate, n=None, vel=None, pitch=0):
        self.bar, self.pos, self.vel, self.pitch = bar, pos, vel, pitch
        self.segments = list(rate) if isinstance(rate, (list, tuple)) else [(rate, n)]


class P:
    def __init__(self, name, bpm, bars, tags, base, rolls=(), vel=100, acc=12, soft=25):
        self.name, self.bpm, self.bars, self.tags = name, bpm, bars, list(tags)
        self.base = [base] if isinstance(base, str) else list(base)
        self.rolls, self.vel, self.acc, self.soft = list(rolls), vel, acc, soft
        self.category = None


def _curve(spec, n, default):
    if spec is None:
        return [default] * n
    if isinstance(spec, int):
        return [spec] * n
    if isinstance(spec, list):
        if len(spec) != n:
            raise ValueError(f"explicit list has {len(spec)} values, roll has {n} notes")
        return list(spec)
    kind = spec[0]
    if kind in ("up", "down", "ramp"):
        a, b = spec[1], spec[2]
        return [round(a + (b - a) * i / (n - 1)) if n > 1 else a for i in range(n)]
    if kind == "steps":
        steps = spec[1]
        return [steps[min(len(steps) - 1, i * len(steps) // n)] for i in range(n)]
    raise ValueError(f"unknown curve {spec}")


def render(p: P) -> list[Note]:
    notes: list[Note] = []
    for b in range(p.bars):
        row = p.base[b % len(p.base)]
        step = BEATS_PER_BAR / len(row)
        for i, ch in enumerate(row):
            if ch == ".":
                continue
            vel = {"x": p.vel, "X": p.vel + p.acc, "o": p.vel - p.soft}[ch]
            notes.append(Note(b * BEATS_PER_BAR + i * step, 0.1, vel, 0))

    for r in p.rolls:
        start = (r.bar - 1) * BEATS_PER_BAR + r.pos
        times = []
        t = start
        for div, count in r.segments:
            for _ in range(count):
                times.append(t)
                t += ROLL_STEPS[RATE_BY_DIVISION[div]]
        zone_start = start - ROLL_THRESHOLD + EPS
        zone_end = times[-1] + ROLL_THRESHOLD - EPS
        notes = [n for n in notes if not (zone_start < n.beat < zone_end)]
        vels = _curve(r.vel, len(times), p.vel)
        pitches = _curve(r.pitch, len(times), 0)
        notes += [Note(t, 0.05, v, pt) for t, v, pt in zip(times, vels, pitches)]

    notes.sort(key=lambda n: n.beat)
    total = p.bars * BEATS_PER_BAR
    for i, n in enumerate(notes):
        nxt = notes[i + 1].beat if i + 1 < len(notes) else total
        n.len = round(min(nxt - n.beat, 0.25) * 0.5, 6)
        n.beat = round(n.beat, 6)
    return notes


def validate(p: P, notes: list[Note]) -> list[str]:
    errors = []
    total = p.bars * BEATS_PER_BAR
    lo, hi = BPM_RANGE[p.category]
    if not lo <= p.bpm <= hi:
        errors.append(f"bpm {p.bpm} outside {lo}-{hi}")
    if p.bars not in (1, 2, 4):
        errors.append("bars must be 1, 2 or 4")
    for i, n in enumerate(notes):
        if not 1 <= n.vel <= 127:
            errors.append(f"velocity {n.vel} at {n.beat}")
        if not 0 <= n.beat < total - EPS:
            errors.append(f"note outside pattern at {n.beat}")
        if i + 1 < len(notes) and notes[i + 1].beat < n.beat + n.len - 1e-6:
            errors.append(f"overlap at {n.beat}")
    if notes and notes[-1].beat < (p.bars - 1) * BEATS_PER_BAR - EPS:
        errors.append("last bar empty")
    rolls = find_rolls(notes)
    for r in rolls:
        for k in range(r.first, r.last):
            if rate_index(notes[k + 1].beat - notes[k].beat) < 0:
                errors.append(f"roll @{r.start:.3f}: illegal step {notes[k + 1].beat - notes[k].beat:.4f}")
        skip_ok = p.category in SKIP_CATEGORIES and (on_grid(r.start, 0.25) or on_grid(r.start, 1 / 6))
        if not on_grid(r.start, 0.5) and not skip_ok:
            errors.append(f"roll @{r.start:.3f}: must start on a beat or 1/8")
        if r.mixed and p.category not in MIXED_CATEGORIES:
            errors.append(f"roll @{r.start:.3f}: mixed rates")
    declared = len(p.rolls)
    if len(rolls) != declared:
        errors.append(f"{declared} rolls declared but {len(rolls)} detected (rolls merged or base grid too dense)")
    if p.category == "PRIMITIVUS":
        if p.bars < 2 and rolls:
            errors.append("1-bar PRIMITIVUS preset may not contain a roll")
        for w in range(0, p.bars, 2):
            if sum(1 for r in rolls if r.bar in (w, w + 1)) > 1:
                errors.append(f"more than one roll in bars {w + 1}-{w + 2}")
    return errors


def to_json(data: dict) -> str:
    """One note per line - readable diffs, still plain JSON."""
    head = {k: v for k, v in data.items() if k != "notes"}
    lines = ["{"] + [f"  {json.dumps(k)}: {json.dumps(v)}," for k, v in head.items()] + ['  "notes": [']
    notes = [f"    {json.dumps(n)}" for n in data["notes"]]
    lines.append(",\n".join(notes))
    lines += ["  ]", "}", ""]
    return "\n".join(lines)


def slug(s: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="validate only, do not write files")
    ap.add_argument("--out", default=os.path.join(ROOT, "Resources", "Presets", "Factory"))
    args = ap.parse_args()

    from factory_presets import LIBRARY  # noqa: E402

    ok = True
    total = Stats()
    per_cat = {}
    outputs = {}
    for ci, cat in enumerate(CATEGORIES):
        presets = LIBRARY.get(cat, [])
        for pi, p in enumerate(presets):
            p.category = cat
            notes = render(p)
            errs = validate(p, notes)
            if errs:
                ok = False
                for e in errs:
                    print(f"[{cat} / {p.name}] {e}")
            total.add(notes, p.bars, p.bpm)
            per_cat.setdefault(cat, Stats()).add(notes, p.bars, p.bpm)
            fname = f"{ci + 1:02d}_{slug(cat)}_{pi + 1}_{slug(p.name)}.json"
            outputs[fname] = {
                "name": p.name, "category": cat, "bpmHint": p.bpm, "bars": p.bars, "tags": p.tags,
                "notes": [{"beat": n.beat, "len": n.len, "vel": n.vel, "pitch": n.pitch} for n in notes],
            }

    print(total.report())
    print()
    for cat, s in per_cat.items():
        print(f"{cat:13} presets {s.files}  rolls/bar {s.rolls / s.bars:.2f}  "
              f"flat {s.share(s.vel_shapes, 'flat') * 100:.0f}%  1/24 {s.share(s.rolls_by_rate, '1/24') * 100:.0f}%")

    if not ok:
        print("\nVALIDATION FAILED")
        return 1

    if not args.check:
        os.makedirs(args.out, exist_ok=True)
        for f in os.listdir(args.out):
            if f.endswith(".json") and f not in outputs:
                os.remove(os.path.join(args.out, f))
        for fname, data in outputs.items():
            with open(os.path.join(args.out, fname), "w", encoding="utf-8") as fh:
                fh.write(to_json(data))
        print(f"\nwrote {len(outputs)} presets to {os.path.relpath(args.out, ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
