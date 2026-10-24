#!/usr/bin/env python3
"""Recreate the offline real-GM phase sheets, without opening the game.

Diagnostic only: the stock two-weight GM mesh is deformed by the serialized AN
using the native inverse-bind/world-palette contract and native X reflection.
Pillow draws depth-sorted shaded triangles; this is not a Metal screenshot.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
from prepare_roundhouse import SOURCES, OUTPUT, MANIFEST, RECIPE, source_rig, animated_mesh, bind_skin, skin_coordinates
from prepare_port_rowboat import fk, add, sub, scale, dot, cross, unit, rotate, qi
from patch_metal_portobello_ground import HEADER, MATERIAL, OBJECT, LABEL

HERE = Path(__file__).parent
INPUTS = HERE / "inputs"
MODELS = json.loads(RECIPE.read_text())["support_models"]


def mesh(data):
    return animated_mesh(data)


def deform(vertices, source, animation, frame):
    p, q = fk(animation, animation["pose"](frame), animation["roots"][frame])
    return skin_coordinates(p, q, bind_skin(source, vertices))


def panel(canvas, box, points, faces, label, target=None, azimuth=-55, target_mesh=None):
    x, y, width, height = box
    draw = ImageDraw.Draw(canvas)
    draw.rectangle((x, y, x+width-1, y+height-1), fill=(24,29,37), outline=(57,65,78))
    a = math.radians(azimuth); right = (math.cos(a), 0., -math.sin(a)); forward = (math.sin(a), .15, math.cos(a))
    zoom = min(width/2.4, (height-40)/2.25)
    def project(p): return (x+width*.5+dot(p,right)*zoom, y+height-26-p[1]*zoom)
    ground = [project((-1,0,-.7)),project((1,0,-.7)),project((1,0,1)),project((-1,0,1))]
    draw.polygon(ground, fill=(34,42,52))
    light = unit((.4, .7, .6))
    rows = []
    for mesh_points, color in ((points,(178,191,207)), (target_mesh,(105,126,149))):
        if mesh_points is None: continue
        rows += [(sum(dot(mesh_points[j],forward) for j in face),[mesh_points[i] for i in face],color) for face in faces]
    for _, ps, base_color in sorted(rows, key=lambda item: item[0]):
        normal = cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])); length = math.sqrt(dot(normal,normal))
        if length < 1e-9: continue
        brightness = .32+.68*abs(dot(scale(normal,1/length),light))
        color = tuple(int(v*brightness) for v in base_color)
        draw.polygon([project(p) for p in ps], fill=color)
    if target:
        sx,sy = project((-target[0],target[1],target[2]))
        draw.ellipse((sx-5,sy-5,sx+5,sy+5),outline=(255,128,65),width=2)
    draw.text((x+9,y+8),label,fill=(225,232,242))
    return project


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture-meshes-from", type=Path)
    parser.add_argument("--motion", action="store_true", help="Render the full six-rig offline motion GIF")
    args = parser.parse_args()
    INPUTS.mkdir(parents=True, exist_ok=True)
    if args.capture_meshes_from:
        record = {}
        for rig_name, model in MODELS.items():
            relative = f"RESOURCE/MODELS/Characters/{model}.gm"
            data = (args.capture_meshes_from/relative).read_bytes()
            target = INPUTS/(model+".gm")
            if target.exists() and target.read_bytes()!=data: raise ValueError("Captured mesh changed")
            if not target.exists(): target.write_bytes(data)
            record[rig_name] = {"model": model, "source": relative, "sha256":hashlib.sha256(data).hexdigest(),"bytes":len(data)}
        (INPUTS/"meshes.json").write_text(json.dumps(record,indent=2,sort_keys=True)+"\n")
    record = json.loads((INPUTS/"meshes.json").read_text())
    sources = json.loads(SOURCES.read_text())["rigs"]
    manifest = json.loads(MANIFEST.read_text())
    from prepare_port_rowboat import rig
    phases = (0,16,32,42,54,68,96)
    labels = ("guard","wind-up","turn/chamber","heel sweep","face contact","follow-through","recovered")
    sheet = Image.new("RGB",(7*245,6*290))
    recovery = Image.new("RGB",(7*245,2*290))
    metrics = {}; animated = []
    for row, name in enumerate(MODELS):
        model = record[name]["model"]; data = (INPUTS/(model+".gm")).read_bytes()
        if hashlib.sha256(data).hexdigest()!=record[name]["sha256"]: raise ValueError("Mesh input changed")
        vertices, faces = mesh(data)
        animation = rig((OUTPUT/f"RESOURCE/animation/Roundhouse_{name}.an").read_bytes())
        guard = dict(animation, pose=lambda frame: sources[name]["guard_pose"], roots=[sources[name]["guard_root"]])
        neutral = deform(vertices, sources[name], guard, 0)
        target_points = [(-x,y,.7-z) for x,y,z in neutral]
        animated.append((name,vertices,faces,animation,target_points))
        first,last = manifest["banks"][name]["actions"]["roundhouse_kick"]
        target = manifest["checks"][name]["heel_target_xyz"]
        bounds = []
        for col,(phase,label) in enumerate(zip(phases,labels)):
            points = deform(vertices,sources[name],animation,first+phase)
            panel(sheet,(col*245,row*290,245,290),points,faces,f"{name}: {label}",target if phase==54 else None,target_mesh=target_points)
            bounds.append(min(p[1] for p in points))
        metrics[name] = {"phase_mesh_floor_min_y":bounds,"triangles":len(faces),"vertices":len(vertices)}
        if name=="man":
            for r, action in enumerate(("getup_front","getup_back")):
                first,last = manifest["banks"][name]["actions"][action]
                for col,phase in enumerate((0,20,40,60,80,100,120)):
                    points = deform(vertices,sources[name],animation,first+phase)
                    panel(recovery,(col*245,r*290,245,290),points,faces,f"{action}: {phase/60:.2f}s",azimuth=65)
        for action in ("getup_front","getup_back"):
            first,last = manifest["banks"][name]["actions"][action]
            metrics[name][action+"_mesh_floor_min_y"] = [min(p[1] for p in deform(vertices,sources[name],animation,first+phase)) for phase in (0,20,40,60,80,100,120)]
        if name == "Jessika":
            first,_ = manifest["banks"][name]["actions"]["roundhouse_kick"]
            points = deform(vertices,sources[name],animation,first+42)
            joints,_ = fk(animation,animation["pose"](first+42),animation["roots"][first+42])
            views = Image.new("RGB",(3*480,540))
            for col,(angle,label) in enumerate(((-55,"author view"),(0,"front"),(-90,"side"))):
                project = panel(views,(col*480,0,480,540),points,faces,
                                f"Jessika: same sweep frame 42 / {label}",azimuth=angle,target_mesh=target_points)
                draw = ImageDraw.Draw(views)
                for ids,color in ((sources[name]["body"]["joints"][9:12],(95,205,245)),
                                  (sources[name]["body"]["joints"][12:15],(255,150,65))):
                    chain = [project((-joints[j][0],joints[j][1],joints[j][2])) for j in ids]
                    draw.line(chain,fill=color,width=3)
                    for label,(x,y) in zip(("hip","knee","ankle"),chain):
                        draw.ellipse((x-4,y-4,x+4,y+4),fill=color)
                        draw.text((x+6,y-10),label,fill=color)
                draw.text((col*480+9,28),"orange: striking leg; blue: support leg",fill=(225,232,242))
            views.save(HERE/"jessika-sweep-views.png")
    sheet.save(HERE/"kick-phases.png");recovery.save(HERE/"getup-phases.png")
    if args.motion:
        frames = []
        for phase in range(0,97,3):
            frame = Image.new("RGB",(3*350,2*350))
            for index,(name,vertices,faces,animation,target_points) in enumerate(animated):
                first,_ = manifest["banks"][name]["actions"]["roundhouse_kick"]
                points = deform(vertices,sources[name],animation,first+phase)
                panel(frame,((index%3)*350,(index//3)*350,350,350),points,faces,f"{name}: {phase/60:.2f}s / target 0.7m",target_mesh=target_points)
            frames.append(frame.convert("P",palette=Image.Palette.ADAPTIVE,colors=96))
        frames[0].save(HERE/"kick-motion.gif",save_all=True,append_images=frames[1:],duration=[200]+[50]*(len(frames)-2)+[200],loop=0,optimize=True)
    (HERE/"mesh-metrics.json").write_text(json.dumps(metrics,indent=2,sort_keys=True)+"\n")
    print(json.dumps(metrics,sort_keys=True))


if __name__=="__main__": main()
