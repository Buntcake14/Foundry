#!/usr/bin/env python3
"""Generate the reusable Foundry vertical list scrollbar components."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


def textured(size: tuple[int, int], base: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", size, (*base, 255))
    pixels = image.load()
    for y in range(size[1]):
        for x in range(size[0]):
            d = rng.randint(-4, 4)
            pixels[x, y] = tuple(max(0, min(255, c + d)) for c in base) + (255,)
    return image


def frame(image: Image.Image) -> Image.Image:
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, image.width - 1, image.height - 1), outline=(67, 39, 8, 255))
    draw.rectangle((1, 1, image.width - 2, image.height - 2), outline=(180, 125, 30, 255))
    return image


def arrow(up: bool, state: int) -> Image.Image:
    bases = [(73, 26, 36), (101, 35, 47), (55, 19, 27), (55, 52, 48)]
    image = frame(textured((16, 16), bases[state], 1760 + state + int(up) * 10))
    draw = ImageDraw.Draw(image)
    points = [(8, 4), (4, 10), (12, 10)] if up else [(4, 6), (12, 6), (8, 12)]
    draw.polygon(points, fill=(225, 176, 67, 255), outline=(67, 39, 8, 255))
    return image


def thumb(state: int) -> Image.Image:
    bases = [(112, 76, 27), (151, 103, 35), (88, 58, 20), (69, 66, 59)]
    image = frame(textured((16, 18), bases[state], 1800 + state))
    draw = ImageDraw.Draw(image)
    draw.line((5, 7, 10, 7), fill=(225, 176, 67, 255))
    draw.line((5, 10, 10, 10), fill=(225, 176, 67, 255))
    return image


def save_strip(path: Path, width: int, height: int, maker) -> None:
    strip = Image.new("RGBA", (width * 4, height), (0, 0, 0, 0))
    for state in range(4):
        strip.paste(maker(state), (width * state, 0))
    strip.save(path, optimize=True)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_vertical_scrollbar.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    frame(textured((16, 16), (37, 25, 24), 1759)).save(
        output / "foundry_vscroll_track_16x16_v1.png", optimize=True)
    save_strip(output / "foundry_vscroll_thumb_16x18_v1.png", 16, 18, thumb)
    save_strip(output / "foundry_vscroll_up_16x16_v1.png", 16, 16, lambda s: arrow(True, s))
    save_strip(output / "foundry_vscroll_down_16x16_v1.png", 16, 16, lambda s: arrow(False, s))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
