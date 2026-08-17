#!/usr/bin/env python3
"""Generate the reusable Foundry footer surface and secondary button strip."""

from pathlib import Path
from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "assets" / "foundry_ui" / "buttons"


def secondary_button() -> None:
    width, height = 144, 30
    strip = Image.new("RGBA", (width * 4, height), (0, 0, 0, 0))
    states = (
        ((42, 35, 31, 255), (111, 82, 35, 255), (205, 165, 78, 255)),
        ((57, 46, 38, 255), (151, 111, 44, 255), (238, 199, 103, 255)),
        ((30, 25, 23, 255), (91, 65, 30, 255), (173, 132, 57, 255)),
        ((39, 37, 35, 220), (70, 65, 57, 220), (116, 107, 90, 220)),
    )
    for frame, (fill, border, highlight) in enumerate(states):
        image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        draw.rounded_rectangle((1, 1, width - 2, height - 2), radius=4, fill=fill, outline=border, width=2)
        draw.line((7, 4, width - 8, 4), fill=highlight, width=1)
        draw.line((7, height - 5, width - 8, height - 5), fill=(15, 12, 11, 210), width=1)
        for x, y in ((5, 5), (width - 6, 5), (5, height - 6), (width - 6, height - 6)):
            draw.rectangle((x - 1, y - 1, x + 1, y + 1), fill=highlight)
        strip.alpha_composite(image, (frame * width, 0))
    strip.save(OUT / "foundry_secondary_button_144x30_v1.png")


def action_bar() -> None:
    width, height = 340, 42
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((0, 0, width - 1, height - 1), radius=3,
                           fill=(38, 24, 24, 238), outline=(132, 94, 35, 255), width=2)
    draw.line((7, 4, width - 8, 4), fill=(211, 164, 72, 210), width=1)
    draw.line((7, height - 5, width - 8, height - 5), fill=(12, 8, 8, 220), width=1)
    draw.line((width // 2, 7, width // 2, height - 8), fill=(104, 74, 36, 170), width=1)
    image.save(OUT / "foundry_action_bar_340x42_v1.png")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    secondary_button()
    action_bar()


if __name__ == "__main__":
    main()
