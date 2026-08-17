#!/usr/bin/env python3
"""Generate the reusable five-category Foundry notification frame strip."""

from pathlib import Path
import sys

from PIL import Image, ImageDraw


FRAME = (340, 92)
CATEGORIES = (
    (42, 91, 126),    # information
    (166, 116, 35),   # economic
    (45, 105, 61),    # diplomatic
    (123, 38, 43),    # military
    (170, 45, 35),    # critical
)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_notification.py OUTPUT", file=sys.stderr)
        return 2

    output = Path(sys.argv[1])
    # Clausewitz/Project Alice sprite sheets advance frames horizontally.
    strip = Image.new("RGBA", (FRAME[0] * len(CATEGORIES), FRAME[1]), (0, 0, 0, 0))
    for frame, accent in enumerate(CATEGORIES):
        image = Image.new("RGBA", FRAME, (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        # Deep burgundy body, antique-gold double frame, and a semantic accent.
        draw.rounded_rectangle((1, 1, 338, 90), radius=5,
                               fill=(47, 20, 22, 246), outline=(91, 65, 25, 255), width=1)
        draw.rounded_rectangle((3, 3, 336, 88), radius=4,
                               outline=(197, 154, 55, 255), width=1)
        draw.rectangle((7, 8, 11, 83), fill=accent + (255,))
        draw.line((48, 39, 326, 39), fill=(112, 77, 31, 210), width=1)
        strip.alpha_composite(image, (frame * FRAME[0], 0))

    output.parent.mkdir(parents=True, exist_ok=True)
    strip.save(output, optimize=True)
    print(f"wrote {output} ({strip.width}x{strip.height}, {len(CATEGORIES)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
