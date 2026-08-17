#!/usr/bin/env python3
"""Generate Foundry divider and compact resource-card surfaces."""

from pathlib import Path
import random
import sys

from PIL import Image, ImageDraw


def texture(size: tuple[int, int], base: tuple[int, int, int], seed: int) -> Image.Image:
    rng = random.Random(seed)
    image = Image.new("RGBA", size, (*base, 255))
    pixels = image.load()
    for y in range(size[1]):
        for x in range(size[0]):
            grain = rng.randint(-3, 3)
            pixels[x, y] = tuple(max(0, min(255, c + grain)) for c in base) + (255,)
    return image


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_foundry_resource_card.py OUTPUT_DIRECTORY", file=sys.stderr)
        return 2
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)

    divider = Image.new("RGBA", (320, 8), (0, 0, 0, 0))
    draw = ImageDraw.Draw(divider)
    draw.line((0, 3, 319, 3), fill=(62, 38, 15, 255), width=1)
    draw.line((0, 4, 319, 4), fill=(190, 140, 42, 255), width=1)
    draw.polygon(((154, 4), (160, 0), (166, 4), (160, 7)),
                 fill=(91, 48, 17, 255), outline=(202, 155, 52, 255))
    divider.save(output / "foundry_divider_320x8_v1.png", optimize=True)

    card = texture((320, 62), (211, 203, 179), 1760)
    draw = ImageDraw.Draw(card)
    draw.rectangle((0, 0, 319, 61), outline=(66, 40, 14, 255))
    draw.rectangle((1, 1, 318, 60), outline=(181, 132, 37, 255))
    draw.rectangle((8, 8, 48, 52), fill=(71, 38, 29, 255), outline=(190, 143, 47, 255))
    draw.rectangle((56, 31, 308, 32), fill=(145, 118, 74, 180))
    card.save(output / "foundry_resource_card_320x62_v1.png", optimize=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
