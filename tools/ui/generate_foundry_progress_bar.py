#!/usr/bin/env python3
"""Generate the reusable Foundry progress-bar fill and empty textures."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


WIDTH = 240
HEIGHT = 14


def make_bar(fill: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    pixels = image.load()

    for y in range(2, HEIGHT - 2):
        for x in range(2, WIDTH - 2):
            grain = rng.randint(-6, 6)
            shade = 8 if y < HEIGHT // 2 else -4
            pixels[x, y] = tuple(max(0, min(255, c + grain + shade)) for c in fill) + (255,)

    draw = ImageDraw.Draw(image)
    gold_dark = (79, 48, 9, 255)
    gold = (174, 121, 31, 255)
    gold_light = (224, 174, 65, 255)
    recess = (24, 17, 15, 255)
    draw.rectangle((0, 2, WIDTH - 1, HEIGHT - 3), outline=gold_dark)
    draw.line((2, 1, WIDTH - 3, 1), fill=gold)
    draw.line((2, HEIGHT - 2, WIDTH - 3, HEIGHT - 2), fill=gold_dark)
    draw.line((3, 2, WIDTH - 4, 2), fill=gold_light)
    draw.line((3, HEIGHT - 3, WIDTH - 4, HEIGHT - 3), fill=recess)
    draw.point((0, HEIGHT // 2), fill=gold_light)
    draw.point((WIDTH - 1, HEIGHT // 2), fill=gold_light)
    return image


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_progress_bar.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2

    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    fill_path = output / "foundry_progress_fill_240x14_v1.png"
    empty_path = output / "foundry_progress_empty_240x14_v1.png"
    make_bar((35, 92, 37), 1760).save(fill_path, optimize=True)
    make_bar((43, 35, 31), 1960).save(empty_path, optimize=True)
    print(f"wrote {fill_path}")
    print(f"wrote {empty_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
