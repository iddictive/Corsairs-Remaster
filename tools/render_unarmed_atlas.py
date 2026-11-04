#!/usr/bin/env python3
"""Offline real-GM unarmed inspection; never starts or captures the game.

Uses native two-weight skinning and depth-tested triangles, not a Metal frame.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from prepare_roundhouse import SOURCES, animated_mesh, bind_skin, skin_coordinates
from prepare_port_rowboat import rig, fk

ROOT = Path(__file__).resolve().parents[1]
BANKS = ROOT / "experiments/native-metal/.cache/unarmed"
INPUTS = ROOT / "docs/evidence/roundhouse-kick/inputs"
EVIDENCE = ROOT / "docs/evidence/unarmed-boxing"
ACTIONS = (
    ("Стойка", "unarmed_idle", .5),
    ("Джеб", "unarmed_jab", 6/14),
    ("Кросс", "unarmed_cross", 9/19),
    ("Апперкот", "unarmed_uppercut", 13/21),
    ("Блок", "unarmed_block", .5),
    ("Уклон влево", "unarmed_dodge_left", .4),
    ("Уклон вправо", "unarmed_dodge_right", .4),
)

def panel(points, faces, angle, size, label, detail=False):
    width, height = size
    a = math.radians(angle)
    right = np.array((math.cos(a), 0., -math.sin(a)))
    toward_camera = np.array((math.sin(a), 0., math.cos(a)))
    points = np.asarray(points)
    zoom = min(width/1.8, (height-45)/1.9) * (1.5 if detail else 1.)
    centered = points - np.array((0., 0., .25 if detail else 0.))
    xy = np.column_stack((width/2 + centered @ right * zoom,
                          height-30 - (points[:, 1]-(.75 if detail else 0.))*zoom))
    depth = points @ toward_camera
    pixels = np.empty((height, width, 3), dtype=np.uint8)
    pixels[:] = (25, 30, 38)
    zbuffer = np.full((height, width), -np.inf)
    light = np.array((.3, .65, .7)); light /= np.linalg.norm(light)
    for face in faces:
        tri = xy[list(face)]
        lo = np.maximum(np.floor(tri.min(axis=0)).astype(int), (0, 0))
        hi = np.minimum(np.ceil(tri.max(axis=0)).astype(int), (width-1, height-1))
        if np.any(lo > hi):
            continue
        x0, y0 = tri[0]; x1, y1 = tri[1]; x2, y2 = tri[2]
        det = (y1-y2)*(x0-x2) + (x2-x1)*(y0-y2)
        if abs(det) < 1e-8:
            continue
        y, x = np.mgrid[lo[1]:hi[1]+1, lo[0]:hi[0]+1] + .5
        u = ((y1-y2)*(x-x2) + (x2-x1)*(y-y2))/det
        v = ((y2-y0)*(x-x2) + (x0-x2)*(y-y2))/det
        w = 1-u-v
        z = u*depth[face[0]] + v*depth[face[1]] + w*depth[face[2]]
        area = np.s_[lo[1]:hi[1]+1, lo[0]:hi[0]+1]
        mask = (u >= 0) & (v >= 0) & (w >= 0) & (z > zbuffer[area])
        normal = np.cross(points[face[1]]-points[face[0]], points[face[2]]-points[face[0]])
        brightness = .38 + .62*abs(normal @ light)/max(np.linalg.norm(normal), 1e-9)
        pixels[area][mask] = np.array((190, 201, 214))*brightness
        zbuffer[area][mask] = z[mask]
    image = Image.fromarray(pixels)
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Unicode.ttf", 16)
    ImageDraw.Draw(image).text((8, 8), label, fill=(230, 235, 245), font=font)
    return image

def load(name, bank_root):
    sources = json.loads(SOURCES.read_text())["rigs"]
    inputs = json.loads((INPUTS/"meshes.json").read_text())
    entry = inputs[name]
    data = (INPUTS/(entry["model"]+".gm")).read_bytes()
    if hashlib.sha256(data).hexdigest() != entry["sha256"]:
        raise ValueError("Real mesh input changed")
    vertices, faces = animated_mesh(data)
    bound = bind_skin(sources[name], vertices)
    animation = rig((bank_root/f"RESOURCE/animation/Unarmed_{name}.an").read_bytes())
    manifest = json.loads((ROOT/"src/assets/unarmed/manifest.json").read_text())
    def points(frame):
        p, q = fk(animation, animation["pose"](frame), animation["roots"][frame])
        return skin_coordinates(p, q, bound)
    return points, faces, animation, manifest["banks"][name]["actions"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rig", default="man", help="Rig name, or all for the six real-mesh peak overview")
    parser.add_argument("--angle", type=float, default=-50.)
    parser.add_argument("--bank-root", type=Path, default=BANKS)
    parser.add_argument("--output", type=Path, default=EVIDENCE/"boxing-pose-atlas.png")
    parser.add_argument("--motion", action="store_true", help="All baked frames, three views, GIF")
    parser.add_argument("--all-frames", action="store_true", help="Every baked frame, upper-body sheet")
    parser.add_argument("--action", action="append", choices=[a[1] for a in ACTIONS], help="Select actions (repeatable)")
    args = parser.parse_args()
    selected = [a for a in ACTIONS if args.action is None or a[1] in args.action]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    size = (320, 360)
    if args.rig == "all":
        if args.motion or args.all_frames:
            parser.error("all is a peak overview; select one rig for full-frame/motion inspection")
        names = list(json.loads((INPUTS/"meshes.json").read_text()))
        canvas = Image.new("RGB", (size[0]*len(selected), size[1]*len(names)))
        for row, name in enumerate(names):
            points, faces, _, actions = load(name, args.bank_root)
            for col, (title, action, contact) in enumerate(selected):
                first, last = actions[action]
                frame = first+round((last-first)*contact)
                canvas.paste(panel(points(frame), faces, args.angle, size,
                    f"{name} · {title} · {args.angle:g}°"), (col*size[0], row*size[1]))
        canvas.save(args.output)
        print(args.output)
        return
    points, faces, animation, actions = load(args.rig, args.bank_root)
    if args.motion:
        frames = []
        for title, action, _ in selected:
            first, last = actions[action]
            for frame in range(first, last+1):
                canvas = Image.new("RGB", (size[0]*3, size[1]))
                mesh = points(frame)
                for col, angle in enumerate((0., -55., 90.)):
                    canvas.paste(panel(mesh, faces, angle, size,
                        f"{args.rig}: {title} {frame-first}/{last-first} · {angle:g}°"), (col*size[0], 0))
                frames.append(canvas)
        frames[0].save(args.output, save_all=True, append_images=frames[1:],
                      duration=1000/animation["fps"], loop=0, optimize=False)
    else:
        rows = []
        for title, action, contact in selected:
            first, last = actions[action]
            phases = (0., .28, contact, .82, 1.) if action == "unarmed_uppercut" else (0., .21, contact, .79, 1.)
            samples = range(first, last+1) if args.all_frames else [
                first+round((last-first)*t) for t in phases]
            rows.extend((frame, f"{title} {frame-first}/{last-first}") for frame in samples)
        cols = 8 if args.all_frames else 5
        canvas = Image.new("RGB", (cols*size[0], math.ceil(len(rows)/cols)*size[1]), (16, 20, 26))
        for i, (frame, title) in enumerate(rows):
            canvas.paste(panel(points(frame), faces, args.angle, size,
                f"{args.rig} · {title} · {args.angle:g}°", args.all_frames),
                ((i % cols)*size[0], (i // cols)*size[1]))
        canvas.save(args.output)
    print(args.output)

if __name__ == "__main__":
    main()
