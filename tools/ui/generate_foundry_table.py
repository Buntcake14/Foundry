#!/usr/bin/env python3
"""Generate reusable Foundry table header and row state strips."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


GOLD = (180, 125, 30, 255)
DARK_GOLD = (67, 39, 8, 255)
PAPER = (202, 193, 166, 255)


def textured_panel(size: tuple[int, int], base: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", size, (*base, 255))
    pixels = image.load()
    for y in range(size[1]):
        for x in range(size[0]):
            grain = rng.randint(-4, 4)
            pixels[x, y] = tuple(max(0, min(255, c + grain)) for c in base) + (255,)
    return image


def framed(image: Image.Image, selected: bool = False) -> Image.Image:
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, image.width - 1, image.height - 1), outline=DARK_GOLD)
    draw.line((1, 1, image.width - 2, 1), fill=GOLD)
    if selected:
        draw.rectangle((2, 2, image.width - 3, image.height - 3), outline=(225, 176, 67, 255))
    return image


def make_header() -> Image.Image:
    image = framed(textured_panel((320, 24), (89, 22, 37), 1760))
    draw = ImageDraw.Draw(image)
    for x in (136, 238, 301):
        draw.line((x, 2, x, 21), fill=(117, 75, 25, 255))
    return image


def make_row(frame: int) -> Image.Image:
    colors = [
        (202, 193, 166),  # normal
        (221, 207, 165),  # hover
        (151, 132, 101),  # selected
        (119, 114, 103),  # disabled
    ]
    image = framed(textured_panel((302, 26), colors[frame], 1800 + frame), frame == 2)
    draw = ImageDraw.Draw(image)
    for x in (136, 238):
        draw.line((x, 2, x, 23), fill=(117, 95, 61, 255))
    return image


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_table.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    make_header().save(output / "foundry_table_header_320x24_v1.png", optimize=True)
    strip = Image.new("RGBA", (302 * 4, 26), (0, 0, 0, 0))
    for frame in range(4):
        strip.paste(make_row(frame), (302 * frame, 0))
    strip.save(output / "foundry_table_row_302x26_v1.png", optimize=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
