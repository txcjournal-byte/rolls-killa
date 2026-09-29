#!/usr/bin/env python3
"""Builds the Rolls Killa Mini skin from the approved design (docs/design/mini_metal.webp).

The design picture itself is the plugin's face. This script cleans the parts that move
(the PUFF ring, lit lamps, BPM digits, hat waveform and name) and cuts the moving parts
out as sprites. Output: Resources/MiniSkin/*.png|jpg (embedded with BinaryData).

    pip install pillow numpy
    python tools/mini_skin/make_skin.py [--debug DIR]

All coordinates are pixels of the 2000 x 733 design (the plugin is 600 x 220 points,
1 point = 3.333 design pixels) and must match Source/MiniEditor.cpp.
"""

from __future__ import annotations

import argparse
import os

import numpy as np
from PIL import Image, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
DESIGN = os.path.join(ROOT, "docs", "design", "mini_metal.webp")
OUT = os.path.join(ROOT, "Resources", "MiniSkin")

# ---- geometry of the design (x0, y0, x1, y1) ----------------------------------------
RING = (1244, 440, 1318, 592)          # steel ring on the blunt (+ its shadow)
RING_SOURCE_DX = -78                   # clean blunt to copy over the ring
RING_SPRITE = (1250, 444, 1310, 586)
LAMPS = [(1632, 154), (1712, 154), (1792, 154), (1873, 154)]   # BARS 1 2 4 8, centres
LAMP_R = 30
DOTS = [(868 + i * 41, 378) for i in range(8)]                  # KILL history, centres
DOT_R = 15
MOOD = [(117, 428), (211, 428), (307, 428)]                     # CHILL TRAP CRAZY LEDs
MOOD_R = 13
TOGGLE = (1816, 594, 1922, 626)        # MIDI | HOST switch
LCD_GLASS = (1090, 90, 1296, 176)      # BPM digits
WAVE_GLASS = (444, 460, 630, 552)      # hat waveform
NAME_PLATE_TEXT = (458, 574, 616, 601)
NAME_PLATE_CLEAN_X = 447               # a text-free column of the name plate


def crop_circle(im: Image.Image, c, r) -> Image.Image:
    return im.crop((c[0] - r, c[1] - r, c[0] + r, c[1] + r))


def paste_circle(dst: Image.Image, sprite: Image.Image, c, r):
    """Pastes a round sprite with a soft edge (keeps the plate around it)."""
    size = sprite.size[0]
    yy, xx = np.mgrid[0:size, 0:size]
    d = np.sqrt((xx - size / 2 + 0.5) ** 2 + (yy - size / 2 + 0.5) ** 2) / (size / 2)
    mask = np.clip((1.0 - d) / 0.12, 0.0, 1.0)
    dst.paste(sprite, (c[0] - r, c[1] - r), Image.fromarray((mask * 255).astype(np.uint8)))


def fill_glass(arr: np.ndarray, box, sample):
    x0, y0, x1, y1 = box
    arr[y0:y1, x0:x1] = arr[sample[1], sample[0]]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--debug", help="also write crops with the geometry drawn in")
    args = ap.parse_args()

    src = Image.open(DESIGN).convert("RGB")
    assert src.size == (2000, 733), src.size
    plate = src.copy()
    os.makedirs(OUT, exist_ok=True)

    # ---- sprites (from the untouched design) ----
    src.crop(RING_SPRITE).save(os.path.join(OUT, "ring.png"))
    crop_circle(src, LAMPS[2], LAMP_R).save(os.path.join(OUT, "lamp_on.png"))
    crop_circle(src, LAMPS[0], LAMP_R).save(os.path.join(OUT, "lamp_off.png"))
    crop_circle(src, DOTS[0], DOT_R).save(os.path.join(OUT, "dot_on.png"))
    crop_circle(src, DOTS[1], DOT_R).save(os.path.join(OUT, "dot_off.png"))
    crop_circle(src, MOOD[1], MOOD_R).save(os.path.join(OUT, "mood_on.png"))
    crop_circle(src, MOOD[0], MOOD_R).save(os.path.join(OUT, "mood_off.png"))
    toggle = src.crop(TOGGLE)
    toggle.save(os.path.join(OUT, "toggle_midi.png"))
    toggle.transpose(Image.FLIP_LEFT_RIGHT).save(os.path.join(OUT, "toggle_host.png"))

    # ---- clean plate ----
    # the ring: copy the blunt just left of it, cross-faded at both ends
    x0, y0, x1, y1 = RING
    arr = np.asarray(plate).astype(np.float32)
    patch = arr[y0:y1, x0 + RING_SOURCE_DX:x1 + RING_SOURCE_DX].copy()
    w = x1 - x0
    fade = 10
    alpha = np.ones(w, dtype=np.float32)
    alpha[:fade] = np.linspace(0.0, 1.0, fade)
    alpha[-fade:] = np.linspace(1.0, 0.0, fade)
    # the ring itself must be fully covered: the fades sit outside RING_SPRITE
    arr[y0:y1, x0:x1] = arr[y0:y1, x0:x1] * (1 - alpha[None, :, None]) + patch * alpha[None, :, None]

    # lit lamps -> unlit
    plate = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    paste_circle(plate, crop_circle(src, LAMPS[0], LAMP_R), LAMPS[2], LAMP_R)
    paste_circle(plate, crop_circle(src, DOTS[1], DOT_R), DOTS[0], DOT_R)
    paste_circle(plate, crop_circle(src, MOOD[0], MOOD_R), MOOD[1], MOOD_R)

    arr = np.asarray(plate).copy()
    fill_glass(arr, LCD_GLASS, (LCD_GLASS[0] + 3, LCD_GLASS[1] + 3))
    fill_glass(arr, WAVE_GLASS, (WAVE_GLASS[0] + 3, WAVE_GLASS[1] + 3))
    tx0, ty0, tx1, ty1 = NAME_PLATE_TEXT
    column = arr[ty0:ty1, NAME_PLATE_CLEAN_X:NAME_PLATE_CLEAN_X + 1]
    arr[ty0:ty1, tx0:tx1] = np.repeat(column, tx1 - tx0, axis=1)
    plate = Image.fromarray(arr)
    plate = plate.filter(ImageFilter.UnsharpMask(radius=1, percent=20, threshold=2))
    plate.save(os.path.join(OUT, "plate.jpg"), quality=90, optimize=True)

    if args.debug:
        from PIL import ImageDraw
        os.makedirs(args.debug, exist_ok=True)
        dbg = plate.copy()
        d = ImageDraw.Draw(dbg)
        for box in (RING, RING_SPRITE, TOGGLE, LCD_GLASS, WAVE_GLASS, NAME_PLATE_TEXT):
            d.rectangle(box, outline=(0, 255, 0))
        for c, r in [(c, LAMP_R) for c in LAMPS] + [(c, DOT_R) for c in DOTS] + [(c, MOOD_R) for c in MOOD]:
            d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), outline=(0, 255, 255))
        dbg.save(os.path.join(args.debug, "skin_debug.png"))
        src.save(os.path.join(args.debug, "design.png"))

    for f in sorted(os.listdir(OUT)):
        print(f, os.path.getsize(os.path.join(OUT, f)), "bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
