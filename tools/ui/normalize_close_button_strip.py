#!/usr/bin/env python3
"""Normalize the generated Foundry close-button master into a 24 px strip."""

from pathlib import Path
import sys

from PIL import Image


FRAME_BOXES = ((57, 107, 491, 541), (600, 107, 1034, 541),
               (1141, 107, 1575, 541), (1683, 107, 2117, 541))
FRAME_SIZE = (24, 24)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: normalize_close_button_strip.py SOURCE OUTPUT", file=sys.stderr)
        return 2

    source = Image.open(sys.argv[1]).convert("RGBA")
    strip = Image.new("RGBA", (FRAME_SIZE[0] * len(FRAME_BOXES), FRAME_SIZE[1]))
    for index, box in enumerate(FRAME_BOXES):
        frame = source.crop(box).resize(FRAME_SIZE, Image.Resampling.LANCZOS)
        strip.alpha_composite(frame, (index * FRAME_SIZE[0], 0))

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    strip.save(output, optimize=True)
    print(f"wrote {output} ({strip.width}x{strip.height}, {len(FRAME_BOXES)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
