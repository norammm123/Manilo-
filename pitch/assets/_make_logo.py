# -*- coding: utf-8 -*-
"""Generate clean logo assets for the Manilo 掌音 pitch deck.

Input : logo.png  (721x667 RGBA, opaque cream background rgb(253,245,233))
Output: logo-mark.png    transparent, source colours (dark olive + coral)
        logo-mono.png    transparent, monochrome ink  #1C1C1C

The Editorial style of the deck is near-monochrome, so a full-colour coral
logo would fight the layout. logo-mono.png is what the cover uses; the
colour version is kept so the team can swap it back trivially.
"""
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "logo.png")
CROP = (58, 54, 635, 623)          # tight content bbox, +1px margin
PAD = 10                           # transparent breathing room
INK = (28, 28, 28)                 # --fg #1C1C1C


def lum(r, g, b):
    return 0.299 * r + 0.587 * g + 0.114 * b


def cut_background(im):
    """Return RGBA with the cream plate removed and a feathered alpha edge."""
    im = im.convert("RGB")
    w, h = im.size
    px = im.load()
    out = Image.new("RGBA", (w, h))
    op = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            # distance from the plate colour
            d = max(abs(r - 253), abs(g - 245), abs(b - 233))
            if d <= 8:
                a = 0
            elif d >= 30:
                a = 255
            else:                              # feather band -> soft edge
                a = int(round((d - 8) / 22.0 * 255))
            op[x, y] = (r, g, b, a)
    return out


def to_mono(im):
    """Keep alpha, replace every opaque pixel with flat ink."""
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    out = Image.new("RGBA", (w, h))
    op = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                op[x, y] = (0, 0, 0, 0)
            else:
                op[x, y] = (INK[0], INK[1], INK[2], a)
    return out


def finish(im, name):
    im = im.crop(CROP)
    w, h = im.size
    canvas = Image.new("RGBA", (w + PAD * 2, h + PAD * 2), (0, 0, 0, 0))
    canvas.paste(im, (PAD, PAD), im)
    path = os.path.join(HERE, name)
    canvas.save(path, "PNG", optimize=True)
    print(name, canvas.size, os.path.getsize(path), "bytes")


if __name__ == "__main__":
    raw = Image.open(SRC)
    cut = cut_background(raw)
    finish(cut, "logo-mark.png")
    finish(to_mono(cut), "logo-mono.png")
    print("done")
