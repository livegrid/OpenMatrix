#!/usr/bin/env python3
"""Extract the gzipped web UI bytes from lib/UI/interface.cpp into
data/public/index.html.gz so the UI can be served from LittleFS instead of
the firmware PROGMEM blob.

Usage:
    python scripts/extract_interface_html.py

After running, flash the filesystem with:
    pio run -t uploadfs

Once verified end-to-end, remove the OPEN_MATRIX_HTML array from
lib/UI/interface.cpp to reclaim ~160 KB of firmware space.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "lib" / "UI" / "interface.cpp"
OUT_DIR = ROOT / "data" / "public"
OUT_FILE = OUT_DIR / "index.html.gz"


def main() -> int:
    if not SRC.exists():
        print(f"ERROR: {SRC} not found", file=sys.stderr)
        return 1

    text = SRC.read_text(encoding="utf-8", errors="replace")

    # Grab everything between the first `{` after OPEN_MATRIX_HTML and the
    # closing `};` of that array.
    match = re.search(
        r"OPEN_MATRIX_HTML\s*\[[^\]]*\]\s*PROGMEM\s*=\s*\{(.*?)\};",
        text,
        re.DOTALL,
    )
    if not match:
        print("ERROR: could not find OPEN_MATRIX_HTML array in interface.cpp",
              file=sys.stderr)
        return 2

    body = match.group(1)
    # Strip comments + newlines then split on commas.
    body = re.sub(r"//[^\n]*", "", body)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.DOTALL)
    nums = [n.strip() for n in body.split(",") if n.strip()]
    if not nums:
        print("ERROR: no bytes parsed from OPEN_MATRIX_HTML", file=sys.stderr)
        return 3

    try:
        data = bytes(int(n, 0) & 0xFF for n in nums)
    except ValueError as e:
        print(f"ERROR: could not parse byte value: {e}", file=sys.stderr)
        return 4

    # Sanity check: gzip magic number.
    if len(data) < 2 or data[0] != 0x1F or data[1] != 0x8B:
        print("WARNING: extracted bytes do not start with gzip magic (0x1F 0x8B). "
              "Continuing anyway.", file=sys.stderr)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    OUT_FILE.write_bytes(data)

    print(f"Wrote {len(data)} bytes to {OUT_FILE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
