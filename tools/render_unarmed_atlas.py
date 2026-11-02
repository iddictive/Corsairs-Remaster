#!/usr/bin/env python3
"""Render the offline skeletal mesh atlas for Unarmed Combat actions.

Generates a clear 7-row visual panel:
Row 1: Unarmed Boxing Stance (Idle)
Row 2: Left Jab (Fast Attack / LMB)
Row 3: Right Cross / Hook (Force Attack / RMB)
Row 4: Right Uppercut (Break Attack / MMB)
Row 5: High Double-Forearm Guard (Block / Space)
Rows 6–7: Left and right guarded duck/side slip
"""
from __future__ import annotations

import json
import math
from pathlib import Path
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from prepare_roundhouse import SOURCES, source_rig, animated_mesh, bind_skin, skin_coordinates
from prepare_port_rowboat import rig, fk, add, sub, scale, dot, cross, unit

UNARMED_DIR = ROOT / "experiments/native-metal/.cache/unarmed"
EVIDENCE_DIR = ROOT / "docs/evidence/unarmed-boxing"
EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)

sources = json.load(open(SOURCES))
man_src = sources["rigs"]["man"]
mesh_data = (ROOT / "docs/evidence/roundhouse-kick/inputs/Blad_0.gm").read_bytes()
vertices, faces = animated_mesh(mesh_data)
bound_vertices = bind_skin(man_src, vertices)

an_path = Path(sys.argv[1]) if len(sys.argv) > 1 else UNARMED_DIR / "RESOURCE/animation/Unarmed_man.an"
an_data = an_path.read_bytes()
parsed_an = rig(an_data)

def deform_frame(frame):
    p, q = fk(parsed_an, parsed_an["pose"](frame), parsed_an["roots"][frame])
    return skin_coordinates(p, q, bound_vertices)

manifest = json.loads((ROOT / "src/assets/unarmed/manifest.json").read_text())
ranges = manifest["banks"]["man"]["actions"]
actions = []
for title, name, phases in [
    ("Боксёрская стойка", "unarmed_idle", (0, .25, .5, .75, 1)),
    ("Левый джеб", "unarmed_jab", (0, .25, .45, .7, 1)),
    ("Правый кросс", "unarmed_cross", (0, .25, .45, .7, 1)),
    ("Правый апперкот", "unarmed_uppercut", (0, .3, .6, .8, 1)),
    ("Защита предплечьями", "unarmed_block", (0, .3, .5, .7, 1)),
    ("Уклон влево", "unarmed_dodge_left", (0, .2, .4, .7, 1)),
    ("Уклон вправо", "unarmed_dodge_right", (0, .2, .4, .7, 1)),
]:
    first, last = ranges[name]
    actions.append((title, [first + round((last-first)*p) for p in phases]))

cols = 5
rows = len(actions)
cell_w, cell_h = 240, 280
pad_x, pad_y = 20, 40
img_w = cols * cell_w + pad_x * 2
img_h = rows * cell_h + pad_y * 2 + 50

canvas = Image.new("RGB", (img_w, img_h), (16, 20, 26))
draw = ImageDraw.Draw(canvas)
font_path = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf"
font = ImageFont.truetype(font_path, 16)
heading = ImageFont.truetype(font_path, 20)

# Title
draw.text((pad_x, 15), "Кулачный бой — фазы скелетной анимации", font=heading, fill=(240, 244, 250))

light = unit((0.4, 0.7, 0.6))
azimuth = -50
a = math.radians(azimuth)
right = (math.cos(a), 0.0, -math.sin(a))
forward = (math.sin(a), 0.15, math.cos(a))
zoom = min(cell_w / 2.2, (cell_h - 40) / 2.1)

for r_idx, (act_name, frame_list) in enumerate(actions):
    y_base = pad_y + 40 + r_idx * cell_h
    # Section header
    draw.text((pad_x, y_base - 23), act_name, font=font, fill=(200, 220, 245))
    
    for c_idx, frame in enumerate(frame_list):
        x_base = pad_x + c_idx * cell_w
        box = (x_base + 4, y_base, cell_w - 8, cell_h - 16)
        bx, by, bw, bh = box
        draw.rectangle((bx, by, bx + bw - 1, by + bh - 1), fill=(22, 27, 35), outline=(45, 55, 68))
        
        # Project function
        def project(pt):
            return (bx + bw * 0.5 + dot(pt, right) * zoom, by + bh - 32 - pt[1] * zoom)
        
        # Ground
        ground = [project((-0.8, 0, -0.6)), project((0.8, 0, -0.6)), project((0.8, 0, 0.8)), project((-0.8, 0, 0.8))]
        draw.polygon(ground, fill=(30, 38, 48))
        
        # Deform
        points = deform_frame(frame)
        face_depths = []
        for face in faces:
            pts = [points[i] for i in face]
            depth = sum(dot(p, forward) for p in pts)
            face_depths.append((depth, pts))
        
        for _, tri in sorted(face_depths, key=lambda x: x[0]):
            norm = cross(sub(tri[1], tri[0]), sub(tri[2], tri[0]))
            length = math.sqrt(dot(norm, norm))
            if length < 1e-9: continue
            brightness = 0.35 + 0.65 * abs(dot(scale(norm, 1.0 / length), light))
            base_col = (185, 198, 215)
            color = tuple(int(v * brightness) for v in base_col)
            draw.polygon([project(p) for p in tri], fill=color)
        
        draw.text((bx + 10, by + bh - 24), f"Кадр {frame}", font=font, fill=(180, 195, 215))

output_png = EVIDENCE_DIR / "boxing-pose-atlas.png"
canvas.save(output_png)
print(f"Saved atlas image: {output_png} ({img_w}x{img_h})")
