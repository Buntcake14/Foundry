#!/usr/bin/env python3
"""Extract Foundry panel and section-header surfaces from the toolkit atlas."""

from collections import deque
from pathlib import Path
import sys

from PIL import Image


def clear_connected_background(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size
    seen = set()
    queue = deque([(x, y) for x in range(width) for y in (0, height - 1)])
    queue.extend((x, y) for y in range(height) for x in (0, width - 1))
    while queue:
        x, y = queue.popleft()
        if (x, y) in seen:
            continue
        red, green, blue, _ = pixels[x, y]
        if red >= 45 or green >= 45 or blue >= 45:
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


def save_surface(source: Image.Image, box: tuple[int, int, int, int],
                 size: tuple[int, int], output: Path) -> None:
    surface = clear_connected_background(source.crop(box))
    surface = surface.resize(size, Image.Resampling.LANCZOS)
    output.parent.mkdir(parents=True, exist_ok=True)
    surface.save(output, optimize=True)
    print(f"wrote {output} ({size[0]}x{size[1]})")


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: extract_toolkit_surfaces.py SOURCE PANEL_OUTPUT HEADER_OUTPUT", file=sys.stderr)
        return 2
    source = Image.open(sys.argv[1]).convert("RGB")
    save_surface(source, (28, 77, 640, 468), (250, 66), Path(sys.argv[2]))
    save_surface(source, (675, 25, 1513, 91), (360, 28), Path(sys.argv[3]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
