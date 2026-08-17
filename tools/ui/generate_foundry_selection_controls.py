#!/usr/bin/env python3
"""Generate reusable Foundry checkbox and radio-button state strips."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


def base_control(seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", (22, 22), (0, 0, 0, 0))
    pixels = image.load()
    for y in range(2, 20):
        for x in range(2, 20):
            grain = rng.randint(-5, 5)
            pixels[x, y] = (max(0, 43 + grain), max(0, 31 + grain), max(0, 25 + grain), 255)
    draw = ImageDraw.Draw(image)
    draw.rectangle((1, 1, 20, 20), outline=(67, 39, 8, 255))
    draw.rectangle((2, 2, 19, 19), outline=(180, 125, 30, 255))
    draw.line((4, 3, 17, 3), fill=(225, 176, 67, 255))
    return image


def checkbox(active: bool) -> Image.Image:
    image = base_control(1760 + int(active))
    if active:
        draw = ImageDraw.Draw(image)
        draw.line((5, 11, 9, 15), fill=(225, 176, 67, 255), width=3)
        draw.line((9, 15, 17, 6), fill=(225, 176, 67, 255), width=3)
    return image


def radio(active: bool) -> Image.Image:
    image = Image.new("RGBA", (22, 22), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.ellipse((2, 2, 19, 19), fill=(42, 30, 24, 255), outline=(67, 39, 8, 255), width=2)
    draw.ellipse((4, 4, 17, 17), outline=(180, 125, 30, 255), width=2)
    if active:
        draw.ellipse((7, 7, 14, 14), fill=(225, 176, 67, 255), outline=(99, 62, 12, 255))
    return image


def save_strip(path: Path, maker) -> None:
    strip = Image.new("RGBA", (44, 22), (0, 0, 0, 0))
    strip.paste(maker(False), (0, 0))
    strip.paste(maker(True), (22, 0))
    strip.save(path, optimize=True)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_selection_controls.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    save_strip(output / "foundry_checkbox_22x22_v1.png", checkbox)
    save_strip(output / "foundry_radio_22x22_v1.png", radio)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
