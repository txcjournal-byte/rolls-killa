"""Shared helpers for the Rolls Killa tools (no third-party dependencies).

- Minimal Standard MIDI File reader
- Roll detection identical to Source/engine/Pattern.cpp (findRolls)
- Statistics identical to Source/engine/PresetValidator.cpp (computeStats)
"""

from __future__ import annotations

import math
import struct
from collections import Counter
from dataclasses import dataclass, field

BEATS_PER_BAR = 4
ROLL_THRESHOLD = 0.25
EPS = 1e-4
ROLL_STEPS = [1 / 6, 1 / 8, 1 / 12, 1 / 16, 1 / 24]
ROLL_NAMES = ["1/24", "1/32", "1/48", "1/64", "1/96"]
RATE_BY_DIVISION = {24: 0, 32: 1, 48: 2, 64: 3, 96: 4}

# Named spacings used in the interval histogram (section 3.2 of the spec)
SPACING_NAMES = [
    (1.0, "1/4"), (0.5, "1/8"), (1 / 3, "1/8T (0.333)"), (0.25, "1/16"),
    (1 / 6, "1/24 (0.167)"), (0.125, "1/32"), (1 / 12, "1/48 (0.083)"),
    (1 / 16, "1/64"), (1 / 24, "1/96 (0.042)"),
]


@dataclass
class Note:
    beat: float
    len: float = 0.1
    vel: int = 100
    pitch: int = 0


@dataclass
class Roll:
    first: int
    count: int
    start: float
    step: float
    length: float
    mixed: bool

    @property
    def last(self) -> int:
        return self.first + self.count - 1

    @property
    def bar(self) -> int:
        return int(self.start / BEATS_PER_BAR + EPS)


def rate_index(step: float) -> int:
    for i, s in enumerate(ROLL_STEPS):
        if abs(step - s) < 1e-3:
            return i
    return -1


def on_grid(beat: float, grid: float) -> bool:
    r = beat / grid
    return abs(r - round(r)) < 1e-3


def find_rolls(notes: list[Note]) -> list[Roll]:
    rolls = []
    i = 0
    n = len(notes)
    while i < n:
        j = i
        # a roll never continues over a bar line (a roll that lands on the downbeat resolves there)
        while (j + 1 < n and notes[j + 1].beat - notes[j].beat < ROLL_THRESHOLD - EPS
               and not on_grid(notes[j + 1].beat, BEATS_PER_BAR)):
            j += 1
        if j > i:
            step = notes[i + 1].beat - notes[i].beat
            last_step = notes[j].beat - notes[j - 1].beat
            mixed = any(abs((notes[k + 1].beat - notes[k].beat) - step) > 1e-3 for k in range(i + 1, j))
            rolls.append(Roll(i, j - i + 1, notes[i].beat, step, notes[j].beat - notes[i].beat + last_step, mixed))
        i = j + 1
    return rolls


def roll_clear_zone(start: float, last_note: float) -> tuple[float, float]:
    bar_start = math.floor(start / BEATS_PER_BAR + EPS) * BEATS_PER_BAR
    next_bar = (math.floor(last_note / BEATS_PER_BAR + EPS) + 1) * BEATS_PER_BAR
    return max(start - ROLL_THRESHOLD + EPS, bar_start - EPS), min(last_note + ROLL_THRESHOLD - EPS, next_bar - EPS)


def classify_velocity(notes: list[Note], roll: Roll) -> str:
    vels = [notes[k].vel for k in range(roll.first, roll.last + 1)]
    if max(vels) - min(vels) <= 8:
        return "flat"
    non_dec = all(b >= a - 2 for a, b in zip(vels, vels[1:]))
    non_inc = all(b <= a + 2 for a, b in zip(vels, vels[1:]))
    if non_dec and vels[-1] > vels[0]:
        return "up"
    if non_inc and vels[-1] < vels[0]:
        return "down"
    return "other"


@dataclass
class Stats:
    files: int = 0
    bars: float = 0
    notes: int = 0
    rolls: int = 0
    rolls_by_rate: Counter = field(default_factory=Counter)
    intervals_in_rolls: Counter = field(default_factory=Counter)
    spacing: Counter = field(default_factory=Counter)
    roll_lengths: Counter = field(default_factory=Counter)
    vel_shapes: Counter = field(default_factory=Counter)
    pitch_intervals: Counter = field(default_factory=Counter)
    start_pos: Counter = field(default_factory=Counter)
    tempos: list = field(default_factory=list)

    def add(self, notes: list[Note], bars: float, tempo: float | None = None):
        notes = sorted(notes, key=lambda n: n.beat)
        self.files += 1
        self.bars += bars
        self.notes += len(notes)
        if tempo:
            self.tempos.append(tempo)
        for a, b in zip(notes, notes[1:]):
            d = b.beat - a.beat
            name = next((nm for v, nm in SPACING_NAMES if abs(d - v) < 2e-3), "other")
            self.spacing[name] += 1
        for r in find_rolls(notes):
            self.rolls += 1
            ri = rate_index(r.step)
            self.rolls_by_rate[ROLL_NAMES[ri] if ri >= 0 else "other"] += 1
            for k in range(r.first, r.last):
                ki = rate_index(notes[k + 1].beat - notes[k].beat)
                self.intervals_in_rolls[ROLL_NAMES[ki] if ki >= 0 else "other"] += 1
            self.roll_lengths[round(r.length * 24) / 24] += 1
            self.vel_shapes[classify_velocity(notes, r)] += 1
            base = notes[r.first].pitch
            pitches = {notes[k].pitch - base for k in range(r.first, r.last + 1)} - {0}
            if not pitches:
                self.pitch_intervals[0] += 1
            for p in pitches:
                self.pitch_intervals[p] += 1
            if on_grid(r.start, 1.0):
                self.start_pos["beat"] += 1
            elif on_grid(r.start, 0.5):
                self.start_pos["1/8"] += 1
            elif on_grid(r.start, 0.25):
                self.start_pos["1/16 (skip)"] += 1
            else:
                self.start_pos["other"] += 1

    def share(self, counter: Counter, key) -> float:
        total = sum(counter.values())
        return counter[key] / total if total else 0.0

    def report(self) -> str:
        def pct_table(counter: Counter, top: int = 12) -> str:
            total = sum(counter.values()) or 1
            return "\n".join(f"    {(f'{k:.3f}' if isinstance(k, float) else str(k)):>14}: {v / total * 100:5.1f} %  ({v})"
                             for k, v in counter.most_common(top))

        lines = [
            f"files: {self.files}, bars: {self.bars:.0f}, notes: {self.notes}, rolls: {self.rolls}",
            f"rolls per bar: {self.rolls / self.bars:.2f}" if self.bars else "rolls per bar: -",
        ]
        if self.tempos:
            t = sorted(self.tempos)
            lines.append(f"tempo: median {t[len(t) // 2]:.0f} BPM, min {t[0]:.0f}, max {t[-1]:.0f}")
        lines += ["spacing between all notes:", pct_table(self.spacing),
                  "roll rate (first interval):", pct_table(self.rolls_by_rate),
                  "intervals inside rolls:", pct_table(self.intervals_in_rolls),
                  "roll length (beats):", pct_table(self.roll_lengths, 8),
                  "roll start:", pct_table(self.start_pos),
                  "velocity shape in rolls:", pct_table(self.vel_shapes),
                  "pitch intervals in rolls (0 = unchanged):", pct_table(self.pitch_intervals, 10)]
        return "\n".join(lines)


# ---------------------------------------------------------------------------
# Standard MIDI File reader
# ---------------------------------------------------------------------------
def _read_varlen(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    while True:
        b = data[pos]
        pos += 1
        value = (value << 7) | (b & 0x7F)
        if not b & 0x80:
            return value, pos


def read_midi(path: str) -> tuple[list[Note], float | None, float]:
    """Returns (notes in beats, first tempo in BPM or None, length in beats)."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"MThd":
        raise ValueError("not a MIDI file")
    hlen = struct.unpack(">I", data[4:8])[0]
    _fmt, ntracks, division = struct.unpack(">HHH", data[8:14])
    if division & 0x8000:
        raise ValueError("SMPTE time division not supported")
    ppq = division
    pos = 8 + hlen
    notes: list[Note] = []
    tempo = None
    end_tick = 0
    for _ in range(ntracks):
        if data[pos:pos + 4] != b"MTrk":
            break
        tlen = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        p = pos + 8
        tend = p + tlen
        tick = 0
        status = 0
        open_notes: dict[tuple[int, int], tuple[int, int]] = {}
        while p < tend:
            delta, p = _read_varlen(data, p)
            tick += delta
            b = data[p]
            if b & 0x80:
                status = b
                p += 1
            if status == 0xFF:
                mtype = data[p]
                mlen, p = _read_varlen(data, p + 1)
                if mtype == 0x51 and tempo is None and mlen == 3:
                    tempo = 60_000_000 / int.from_bytes(data[p:p + 3], "big")
                p += mlen
            elif status in (0xF0, 0xF7):
                slen, p = _read_varlen(data, p)
                p += slen
            else:
                kind = status & 0xF0
                ch = status & 0x0F
                if kind in (0xC0, 0xD0):
                    p += 1
                    continue
                d1, d2 = data[p], data[p + 1]
                p += 2
                if kind == 0x90 and d2 > 0:
                    open_notes[(ch, d1)] = (tick, d2)
                elif kind == 0x80 or (kind == 0x90 and d2 == 0):
                    if (ch, d1) in open_notes:
                        start, vel = open_notes.pop((ch, d1))
                        notes.append(Note(start / ppq, max(1, tick - start) / ppq, vel, d1))
        for (ch, key), (start, vel) in open_notes.items():
            notes.append(Note(start / ppq, 0.1, vel, key))
        end_tick = max(end_tick, tick)
        pos = tend
    notes.sort(key=lambda n: n.beat)
    return notes, tempo, end_tick / ppq


def bars_for(notes: list[Note], length_beats: float) -> int:
    last = max((n.beat for n in notes), default=0.0)
    length = max(length_beats, last + EPS)
    return max(1, math.ceil(length / BEATS_PER_BAR - 1e-6))
