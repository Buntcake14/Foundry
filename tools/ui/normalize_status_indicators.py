#!/usr/bin/env python3
"""Normalize the generated Foundry status sheet into four 24px sprite frames."""

from pathlib import Path
import sys

from PIL import Image


FRAME_SIZE = 24
FRAME_COUNT = 4


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: normalize_status_indicators.py SOURCE OUTPUT", file=sys.stderr)
        return 2

    source = Image.open(sys.argv[1]).convert("RGBA")
    cell_width = source.width // FRAME_COUNT
    strip = Image.new("RGBA", (FRAME_SIZE * FRAME_COUNT, FRAME_SIZE))

    for index in range(FRAME_COUNT):
        cell = source.crop((index * cell_width, 0, (index + 1) * cell_width, source.height))
        # The generated source includes an intentionally oversized ornamental
        # medallion. At 24px the outer ring obscures the actual status glyph,
        # so production frames use the strong central symbol only.
        left = int(cell.width * 0.20)
        right = int(cell.width * 0.80)
        top = int(cell.height * 0.16)
        bottom = int(cell.height * 0.84)
        cell = cell.crop((left, top, right, bottom))
        alpha_box = cell.getchannel("A").getbbox()
        if alpha_box:
            cell = cell.crop(alpha_box)
        cell.thumbnail((FRAME_SIZE, FRAME_SIZE), Image.Resampling.LANCZOS)
        x = index * FRAME_SIZE + (FRAME_SIZE - cell.width) // 2
        y = (FRAME_SIZE - cell.height) // 2
        strip.alpha_composite(cell, (x, y))

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    strip.save(output, optimize=True)
    print(f"wrote {output} ({strip.width}x{strip.height}, {FRAME_COUNT} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
