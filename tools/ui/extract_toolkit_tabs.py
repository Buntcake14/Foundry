#!/usr/bin/env python3
"""Extract a two-frame interactive tab strip from the approved toolkit atlas."""

from collections import deque
from pathlib import Path
import sys

from PIL import Image


# Frame zero is inactive and frame one is active, matching checkbox_button.
# Both are derived from the intact active-tab silhouette; the atlas's charcoal
# tab touches its black backdrop, which makes reliable alpha extraction
# impossible at its left edge.
FRAME_BOXES = ((34, 498, 234, 576), (34, 498, 234, 576))
FRAME_SIZE = (120, 30)


def clear_connected_background(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    seen = set()
    queue = deque()

    for x in range(width):
        queue.append((x, 0))
        queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y))
        queue.append((width - 1, y))

    def background(x: int, y: int) -> bool:
        red, green, blue, _ = pixels[x, y]
        return red < 45 and green < 45 and blue < 45

    while queue:
        x, y = queue.popleft()
        if (x, y) in seen or not background(x, y):
            continue
        seen.add((x, y))
        pixels[x, y] = (0, 0, 0, 0)
        if x:
            queue.append((x - 1, y))
        if x + 1 < width:
            queue.append((x + 1, y))
        if y:
            queue.append((x, y - 1))
        if y + 1 < height:
            queue.append((x, y + 1))
    return rgba


def burgundy_surface(image: Image.Image, active: bool) -> Image.Image:
    """Tint the parchment active face while preserving its antique-gold trim."""
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = pixels[x, y]
            gold = red > 90 and red > green * 1.18 and green > blue * 1.15
            if alpha and not gold:
                light = (red + green + blue) / 3.0
                if active:
                    pixels[x, y] = (
                        min(150, int(35 + light * 0.45)),
                        min(35, int(5 + light * 0.09)),
                        min(50, int(12 + light * 0.13)),
                        alpha,
                    )
                else:
                    pixels[x, y] = (
                        min(90, int(20 + light * 0.26)),
                        min(25, int(4 + light * 0.06)),
                        min(34, int(9 + light * 0.08)),
                        alpha,
                    )
    return image


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: extract_toolkit_tabs.py SOURCE OUTPUT", file=sys.stderr)
        return 2

    source = Image.open(sys.argv[1]).convert("RGB")
    strip = Image.new("RGBA", (FRAME_SIZE[0] * len(FRAME_BOXES), FRAME_SIZE[1]))
    for index, box in enumerate(FRAME_BOXES):
        frame = clear_connected_background(source.crop(box))
        frame = burgundy_surface(frame, index == 1)
        frame = frame.resize(FRAME_SIZE, Image.Resampling.LANCZOS)
        strip.alpha_composite(frame, (index * FRAME_SIZE[0], 0))

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    strip.save(output, optimize=True)
    print(f"wrote {output} ({strip.width}x{strip.height}, {len(FRAME_BOXES)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
