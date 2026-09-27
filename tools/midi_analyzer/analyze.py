#!/usr/bin/env python3
"""Hi-hat MIDI analyzer for tuning Rolls Killa presets (not part of the plugin).

Usage:
    python tools/midi_analyzer/analyze.py "C:/Users/me/Documents/drumkits"
    python tools/midi_analyzer/analyze.py --presets Resources/Presets/Factory
    python tools/midi_analyzer/analyze.py DIR --json stats.json --per-kit

MIDI mode scans every *.mid/*.midi whose path contains "hat" (use --all to take every file),
treats all notes as hi-hats and prints the statistics from section 3.2 of the spec.
Files programmed in half-time (tempo <= --half-time-below) are converted to double time,
so a 1/8 at 70 BPM counts as a 1/16 at 140 BPM.

Nothing is copied from the analyzed files - the output is statistics only.
"""

from __future__ import annotations

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from rollkit import Note, Stats, bars_for, read_midi  # noqa: E402


def iter_midi_files(root: str, take_all: bool):
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            if not f.lower().endswith((".mid", ".midi")):
                continue
            full = os.path.join(dirpath, f)
            if take_all or "hat" in full.lower() or "hh" in f.lower():
                yield full


def analyze_midi(root: str, args) -> dict[str, Stats]:
    per_kit: dict[str, Stats] = {}
    total = Stats()
    errors = 0
    for path in iter_midi_files(root, args.all):
        try:
            notes, tempo, length = read_midi(path)
        except Exception as exc:  # noqa: BLE001 - report and continue
            errors += 1
            print(f"skip {path}: {exc}", file=sys.stderr)
            continue
        if not notes:
            continue
        if tempo and tempo <= args.half_time_below:
            notes = [Note(n.beat * 2, n.len * 2, n.vel, n.pitch) for n in notes]
            length *= 2
            tempo *= 2
        bars = bars_for(notes, length)
        total.add(notes, bars, tempo)
        kit = os.path.relpath(path, root).split(os.sep)[0]
        per_kit.setdefault(kit, Stats()).add(notes, bars, tempo)
    if errors:
        print(f"{errors} files could not be read", file=sys.stderr)
    return {"TOTAL": total, **({k: v for k, v in sorted(per_kit.items())} if args.per_kit else {})}


def analyze_presets(folder: str) -> dict[str, Stats]:
    total = Stats()
    per_cat: dict[str, Stats] = {}
    for f in sorted(os.listdir(folder)):
        if not f.endswith(".json"):
            continue
        with open(os.path.join(folder, f), encoding="utf-8") as fh:
            p = json.load(fh)
        notes = [Note(n["beat"], n["len"], n["vel"], n["pitch"]) for n in p["notes"]]
        total.add(notes, p["bars"], p.get("bpmHint"))
        per_cat.setdefault(p["category"], Stats()).add(notes, p["bars"], p.get("bpmHint"))
    return {"TOTAL": total, **per_cat}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder", nargs="?", help="folder with drum kits (MIDI mode)")
    ap.add_argument("--presets", help="analyze a folder of preset JSON files instead")
    ap.add_argument("--all", action="store_true", help="take every MIDI file, not only *hat* ones")
    ap.add_argument("--per-kit", action="store_true", help="also print stats per top-level kit folder")
    ap.add_argument("--half-time-below", type=float, default=95.0, help="tempo treated as half-time (default 95)")
    ap.add_argument("--json", help="write the stats as JSON to this file")
    args = ap.parse_args()

    if args.presets:
        results = analyze_presets(args.presets)
    elif args.folder:
        results = analyze_midi(args.folder, args)
    else:
        ap.print_help()
        return 1

    for name, stats in results.items():
        print(f"===== {name} =====")
        print(stats.report())
        print()

    if args.json:
        out = {}
        for name, s in results.items():
            out[name] = {
                "files": s.files, "bars": s.bars, "rolls": s.rolls, "notes": s.notes,
                "rollsPerBar": s.rolls / s.bars if s.bars else 0,
                "rollRates": dict(s.rolls_by_rate), "intervalsInRolls": dict(s.intervals_in_rolls),
                "spacing": dict(s.spacing), "rollLengths": {str(k): v for k, v in s.roll_lengths.items()},
                "velocityShapes": dict(s.vel_shapes), "pitchIntervals": {str(k): v for k, v in s.pitch_intervals.items()},
                "rollStart": dict(s.start_pos), "tempos": s.tempos,
            }
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(out, fh, indent=2)
    return 0


if __name__ == "__main__":
    sys.exit(main())
