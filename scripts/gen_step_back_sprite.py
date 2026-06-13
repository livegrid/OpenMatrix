#!/usr/bin/env python3
"""Regenerate lib/EffectManager/StepBackSprite.cpp from a proximity-warning PNG."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image

DEFAULT_SRC = Path(__file__).resolve().parents[1] / "assets" / "proximity-warning.png"
OUT = Path(__file__).resolve().parents[1] / "lib" / "EffectManager" / "StepBackSprite.cpp"


def pixel_on(img: Image.Image, x: int, y: int) -> bool:
    px = img.getpixel((x, y))
    if img.mode == "RGBA":
        r, g, b, a = px
        if a < 32:
            return False
        lum = r + g + b
        # White/light glyphs on dark background
        if lum > 384:
            return True
        # Black/dark glyphs on transparent background
        if lum < 384 and a > 128:
            return True
        return False
    return px > 127


def generate(src: Path, out: Path) -> None:
    img = Image.open(src)
    w, h = img.size
    if w != 64:
        print(f"warning: expected 64px width, got {w}px", file=sys.stderr)

    lines = [
        '#include "StepBackSprite.h"',
        "",
        f"// {w}x{h} proximity warning (1-bit monochrome, MSB = left).",
        f"// Generated from {src.name} — foreground pixels drawn in red at runtime.",
        "",
        "const uint8_t kStepBackSpriteBitmap[] PROGMEM = {",
    ]
    on = 0
    for y in range(h):
        row_bytes = []
        for bx in range(8):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and pixel_on(img, x, y):
                    byte |= 0x80 >> bit
                    on += 1
            row_bytes.append(f"0x{byte:02X}")
        lines.append("    " + ", ".join(row_bytes) + ",")
    lines.append("};")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {out} ({w}x{h}, {on} foreground pixels)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "source",
        nargs="?",
        type=Path,
        default=DEFAULT_SRC,
        help=f"PNG source (default: {DEFAULT_SRC})",
    )
    args = parser.parse_args()
    if not args.source.is_file():
        print(f"error: source not found: {args.source}", file=sys.stderr)
        sys.exit(1)
    generate(args.source, OUT)


if __name__ == "__main__":
    main()
