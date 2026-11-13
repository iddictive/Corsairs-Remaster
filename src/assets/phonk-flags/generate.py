#!/usr/bin/env python3
"""Generate the three-cell Phonk national flag atlas as an uncompressed TGA."""
import math
import struct
from pathlib import Path

WIDTH, HEIGHT = 256, 128


def inside_star(x, y, cx, cy, radius):
    points = []
    for i in range(10):
        angle = -math.pi / 2 + i * math.pi / 5
        r = radius if i % 2 == 0 else radius * .4
        points.append((cx + r * math.cos(angle), cy + r * math.sin(angle)))
    inside = False
    for i, (ax, ay) in enumerate(points):
        bx, by = points[(i + 1) % len(points)]
        if (ay > y) != (by > y) and x < (bx - ax) * (y - ay) / (by - ay) + ax:
            inside = not inside
    return inside


def pixel(cell, x, y):
    if cell == 0:
        return ((255, 255, 255), (0, 57, 166), (213, 43, 30))[min(2, y * 3 // HEIGHT)]
    if cell == 1:
        return (0, 87, 183) if y < HEIGHT // 2 else (255, 215, 0)
    red, white, blue = (178, 34, 52), (255, 255, 255), (60, 59, 110)
    if x < 102 and y < 69:
        for row in range(9):
            count = 6 if row % 2 == 0 else 5
            for col in range(count):
                cx = (col + (0.5 if count == 6 else 1.0)) * 102 / 6
                cy = (row + .5) * 69 / 9
                if abs(x - cx) < 4 and abs(y - cy) < 4 and inside_star(x + .5, y + .5, cx, cy, 3.2):
                    return white
        return blue
    return red if y * 13 // HEIGHT % 2 == 0 else white


def main():
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, WIDTH * 3, HEIGHT, 24, 0x20)
    output = bytearray(header)
    for y in range(HEIGHT):
        for cell in range(3):
            for x in range(WIDTH):
                r, g, b = pixel(cell, x, y)
                output.extend((b, g, r))
    Path(__file__).with_name("national.tga").write_bytes(output)


if __name__ == "__main__":
    main()
