#!/usr/bin/env python3
"""Generate synthetic grayscale PGM images for batch processing tests."""

import argparse
import os
import random
import math
from PIL import Image, ImageDraw


def make_image(index: int, size: int = 512) -> Image.Image:
    """Create a synthetic grayscale image with geometric patterns."""
    img = Image.new("L", (size, size), color=0)
    draw = ImageDraw.Draw(img)

    rng = random.Random(index)

    # Random background gradient
    bg = rng.randint(20, 80)
    for y in range(size):
        val = int(bg + (y / size) * rng.randint(20, 60))
        draw.line([(0, y), (size, y)], fill=min(val, 255))

    # Draw several shapes to give the filter something interesting to detect
    for _ in range(rng.randint(4, 10)):
        shape = rng.choice(["rect", "ellipse", "line"])
        x0 = rng.randint(0, size - 1)
        y0 = rng.randint(0, size - 1)
        x1 = rng.randint(x0, min(x0 + 200, size - 1))
        y1 = rng.randint(y0, min(y0 + 200, size - 1))
        fill = rng.randint(100, 255)

        if shape == "rect":
            draw.rectangle([x0, y0, x1, y1], fill=fill)
        elif shape == "ellipse":
            draw.ellipse([x0, y0, x1, y1], fill=fill)
        else:
            draw.line([x0, y0, x1, y1], fill=fill, width=rng.randint(1, 5))

    # Add a sine-wave stripe pattern
    freq = rng.uniform(0.02, 0.08)
    amp = rng.randint(20, 60)
    pixels = img.load()
    for x in range(size):
        stripe = int(amp * math.sin(2 * math.pi * freq * x))
        for y in range(size):
            pixels[x, y] = min(255, max(0, pixels[x, y] + stripe))

    return img


def main():
    parser = argparse.ArgumentParser(description="Generate synthetic PGM test images")
    parser.add_argument("--count", type=int, default=120, help="Number of images to generate")
    parser.add_argument("--size", type=int, default=512, help="Image width and height in pixels")
    parser.add_argument("--outdir", default="data/input", help="Output directory")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    print(f"Generating {args.count} images ({args.size}x{args.size}) into '{args.outdir}' ...")
    for i in range(args.count):
        img = make_image(i, args.size)
        path = os.path.join(args.outdir, f"synthetic_{i:04d}.pgm")
        img.save(path)
        if (i + 1) % 20 == 0 or i == args.count - 1:
            print(f"  {i + 1}/{args.count} images written")

    print("Done.")


if __name__ == "__main__":
    main()
