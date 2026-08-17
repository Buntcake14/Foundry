#!/usr/bin/env python3
"""Normalize generated Foundry button artwork into an equal-frame sprite strip."""

from pathlib import Path
import sys

from PIL import Image


# The generated master contains a few nearly transparent pixels well above and
# below the painted controls. Crop the shared visible band deliberately so
# those pixels cannot squash the artwork during normalization.
FRAME_BOXES = ((24, 288, 482, 432), (512, 288, 970, 432),
               (1001, 288, 1459, 432), (1490, 288, 1948, 432))
DEFAULT_FRAME_SIZE = (200, 48)


def main() -> int:
    if len(sys.argv) not in (3, 5):
        print("usage: normalize_button_strip.py SOURCE OUTPUT [WIDTH HEIGHT]", file=sys.stderr)
        return 2

    frame_size = DEFAULT_FRAME_SIZE if len(sys.argv) == 3 else (int(sys.argv[3]), int(sys.argv[4]))
    if frame_size[0] <= 0 or frame_size[1] <= 0:
        print("frame dimensions must be positive", file=sys.stderr)
        return 2

    source = Image.open(sys.argv[1]).convert("RGBA")
    strip = Image.new("RGBA", (frame_size[0] * len(FRAME_BOXES), frame_size[1]))

    for index, box in enumerate(FRAME_BOXES):
        frame = source.crop(box).resize(frame_size, Image.Resampling.LANCZOS)
        strip.alpha_composite(frame, (index * frame_size[0], 0))

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    strip.save(output, optimize=True)
    print(f"wrote {output} ({strip.width}x{strip.height}, {len(FRAME_BOXES)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
