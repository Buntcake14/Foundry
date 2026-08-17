#!/usr/bin/env python3
"""Generate reusable Foundry dropdown and dropdown-option textures."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


def surface(width: int, height: int, base: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    pixels = image.load()
    for y in range(2, height - 2):
        for x in range(2, width - 2):
            grain = rng.randint(-5, 5)
            shade = 5 if y < height // 2 else -4
            pixels[x, y] = tuple(max(0, min(255, c + grain + shade)) for c in base) + (255,)
    return image


def frame(image: Image.Image, arrow: bool = False) -> None:
    draw = ImageDraw.Draw(image)
    width, height = image.size
    dark = (69, 40, 8, 255)
    gold = (171, 116, 27, 255)
    light = (224, 174, 65, 255)
    draw.rectangle((0, 0, width - 1, height - 1), outline=dark)
    draw.rectangle((1, 1, width - 2, height - 2), outline=gold)
    draw.line((3, 2, width - 4, 2), fill=light)
    if arrow:
        x = width - 15
        y = height // 2
        draw.polygon(((x - 4, y - 2), (x + 4, y - 2), (x, y + 3)), fill=light)
        draw.line((width - 29, 4, width - 29, height - 5), fill=dark)


def strip(width: int, height: int, colors: list[tuple[int, int, int]], arrow: bool) -> Image.Image:
    output = Image.new("RGBA", (width * len(colors), height), (0, 0, 0, 0))
    for index, color in enumerate(colors):
        image = surface(width, height, color, 1760 + index)
        frame(image, arrow)
        output.paste(image, (index * width, 0))
    return output


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_dropdown.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    colors = [(74, 17, 29), (99, 25, 39), (54, 12, 22), (57, 51, 46)]
    strip(220, 28, colors, True).save(output / "foundry_dropdown_220x28_v1.png", optimize=True)
    option_colors = [(232, 224, 205), (218, 195, 151), (199, 179, 143), (137, 130, 116)]
    strip(220, 24, option_colors, False).save(output / "foundry_dropdown_option_220x24_v1.png", optimize=True)
    popup = surface(224, 76, (225, 216, 197), 1960)
    frame(popup)
    popup.save(output / "foundry_dropdown_popup_224x76_v1.png", optimize=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
