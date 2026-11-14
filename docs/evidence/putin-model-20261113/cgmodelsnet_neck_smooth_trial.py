"""Smooth only the graft immediately above the cut; keep the join fixed."""
import math
import struct
import sys
from pathlib import Path

import numpy as np

here = Path(__file__).resolve().parent
data = bytearray((here / 'cgmodelsnet-body-scarf-deep.gm').read_bytes())
h = struct.unpack_from('<11i7f', data)
tri = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces = np.frombuffer(data, '<u2', h[9]*3, tri).reshape(-1, 3).copy()
off = tri+6*h[9]
kind, size = struct.unpack_from('<2i', data, off)
assert kind == 4 and size % 44 == 0
dt = np.dtype({'names':['p','n'], 'formats':[('<f4',3),('<f4',3)], 'offsets':[0,20], 'itemsize':44})
v = np.frombuffer(data, dt, size//44, off+8)
pos = v['p'].astype(float).copy()
head_start, head_end, bridge_end = 2106, 14993, 15623
head_faces = faces[np.all((faces >= head_start) & (faces < head_end), axis=1)] - head_start
head = pos[head_start:head_end]
keys, inverse = np.unique(np.rint(head*1e6).astype(np.int64), axis=0, return_inverse=True)
counts = np.bincount(inverse)
points = np.zeros((len(keys),3)); np.add.at(points, inverse, head); points /= counts[:,None]
gf = inverse[head_faces]
edges = np.vstack((gf[:,[0,1]], gf[:,[1,2]], gf[:,[2,0]]))
edges = np.unique(np.sort(edges, axis=1), axis=0); edges = edges[edges[:,0] != edges[:,1]]
degree = np.bincount(edges.flatten(), minlength=len(points))
field = points[:,1]+.25*(points[:,2]-.02)-.30*abs(points[:,0]-.01)-1.595
weight = np.clip(field/.002, 0, 1)*np.clip((.014-field)/.008, 0, 1)
weight[(field < 0) | (field >= .014)] = 0
original = points.copy()
for _ in range(6):
    sums = np.zeros_like(points)
    np.add.at(sums, edges[:,0], points[edges[:,1]])
    np.add.at(sums, edges[:,1], points[edges[:,0]])
    average = sums / np.maximum(degree[:,None], 1)
    points += .35*weight[:,None]*(average-points)
new = points[inverse]
delta = np.linalg.norm(new-head, axis=1)
assert np.max(delta) < .005
before = np.cross(head[head_faces[:,1]]-head[head_faces[:,0]], head[head_faces[:,2]]-head[head_faces[:,0]])
after = np.cross(new[head_faces[:,1]]-new[head_faces[:,0]], new[head_faces[:,2]]-new[head_faces[:,0]])
den = np.linalg.norm(before,axis=1)*np.linalg.norm(after,axis=1)
dot = np.sum(before*after,axis=1)/np.maximum(den,1e-20)
affected = np.any(delta[head_faces] > 1e-8, axis=1) & (den > 1e-18)
print('moved head vertices', int((delta>1e-8).sum()), 'max displacement', float(delta.max()))
print('affected faces', int(affected.sum()), 'rotated >60 degrees', int((dot[affected]<.5).sum()), 'flipped >90 degrees', int((dot[affected]<0).sum()))
assert not np.any(dot[affected] < 0)
v['p'][head_start:head_end] = new
# Keep the 607 head-side bridge copies coincident with their head vertices.
bridge = pos[15016:bridge_end]
lookup = {tuple(k): i for i,k in enumerate(keys)}
matched = 0
for i,p in enumerate(bridge, start=15016):
    j = lookup.get(tuple(np.rint(p*1e6).astype(np.int64)))
    assert j is not None
    v['p'][i] = p + points[j] - original[j]
    matched += 1
print('bridge copies kept coincident with head', matched)
out = here/'cgmodelsnet-neck-smooth-prototype.gm'; out.write_bytes(data)
renderer = here/'render_officer.py'
for texture, tag in [('cgmodelsnet-scarf-deep-bridge-atlas.png','textured'), ('sculpture-registration-neutral-atlas.png','neutral')]:
    for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'", "f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1
            source=source.replace(old,repr(f'cgmodelsnet-neck-smooth-{tag}-{label}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/texture),str(angle),str(out)]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
