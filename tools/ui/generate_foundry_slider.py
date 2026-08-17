#!/usr/bin/env python3
"""Generate reusable horizontal Foundry slider textures."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


def textured(width: int, height: int, base: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    pixels = image.load()
    for y in range(1, height - 1):
        for x in range(1, width - 1):
            grain = rng.randint(-5, 5)
            pixels[x, y] = tuple(max(0, min(255, c + grain)) for c in base) + (255,)
    return image


def bordered(width: int, height: int, base: tuple[int, int, int], seed: int) -> Image.Image:
    image = textured(width, height, base, seed)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width - 1, height - 1), outline=(68, 40, 8, 255))
    draw.rectangle((1, 1, width - 2, height - 2), outline=(178, 124, 31, 255))
    draw.line((3, 2, width - 4, 2), fill=(225, 176, 67, 255))
    return image


def state_strip(draw_frame) -> Image.Image:
    output = Image.new("RGBA", (80, 20), (0, 0, 0, 0))
    for state in range(4):
        output.paste(draw_frame(state), (state * 20, 0))
    return output


def knob(state: int) -> Image.Image:
    colors = ((135, 86, 18), (180, 122, 25), (101, 58, 12), (76, 69, 59))
    image = bordered(20, 20, colors[state], 1760 + state)
    draw = ImageDraw.Draw(image)
    draw.ellipse((5, 5, 14, 14), fill=(225, 176, 67, 255), outline=(63, 35, 7, 255))
    return image


def arrow(direction: int):
    def make(state: int) -> Image.Image:
        colors = ((74, 17, 29), (99, 25, 39), (54, 12, 22), (57, 51, 46))
        image = bordered(20, 20, colors[state], 1960 + state + direction * 10)
        draw = ImageDraw.Draw(image)
        x = 10
        points = ((x + 3 * direction, 5), (x - 3 * direction, 10), (x + 3 * direction, 15))
        draw.polygon(points, fill=(225, 176, 67, 255))
        return image
    return make


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_slider.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)

    track = Image.new("RGBA", (220, 20), (0, 0, 0, 0))
    draw = ImageDraw.Draw(track)
    draw.rectangle((0, 7, 219, 13), fill=(29, 21, 18, 255), outline=(70, 42, 10, 255))
    draw.line((2, 8, 217, 8), fill=(178, 124, 31, 255))
    track.save(output / "foundry_slider_track_220x20_v1.png", optimize=True)
    state_strip(knob).save(output / "foundry_slider_knob_20x20_v1.png", optimize=True)
    state_strip(arrow(1)).save(output / "foundry_slider_left_20x20_v1.png", optimize=True)
    state_strip(arrow(-1)).save(output / "foundry_slider_right_20x20_v1.png", optimize=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
