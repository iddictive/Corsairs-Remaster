"""Remove three detached earring shells from the officer_5 prototype only."""
import struct
from pathlib import Path
import numpy as np

here = Path(__file__).resolve().parent
data = (here/'morph-prototype.gm').read_bytes()
h = list(struct.unpack_from('<11i7f', data))
obj = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri = obj+104*h[8]
faces = np.frombuffer(data, dtype='<u2', count=h[9]*3, offset=tri).reshape(-1,3)
buff = tri+6*h[9]
kind,size = struct.unpack_from('<2i', data, buff)
assert kind == 4 and size % 44 == 0 and h[8] == 1
verts = np.frombuffer(data, dtype=np.dtype({'names':['p'], 'formats':[('<f4',3)], 'itemsize':44}), count=size//44, offset=buff+8)['p']
parent = list(range(len(verts)))
def root(i):
    while parent[i] != i:
        parent[i] = parent[parent[i]]
        i = parent[i]
    return i
for face in faces:
    a,b,c = map(int, face)
    parent[root(b)] = root(a)
    parent[root(c)] = root(a)
groups = {}
for i,face in enumerate(faces):
    groups.setdefault(root(int(face[0])), []).append(i)
removed = []
for ids in groups.values():
    points = verts[np.unique(faces[ids])]
    lo,hi = points.min(0),points.max(0)
    if (len(ids) == 16 and lo[0] > .077 and hi[0] < .092 and
            lo[1] > 1.65 and hi[1] < 1.682 and lo[2] > .002 and hi[2] < .030):
        removed.extend(ids)
assert len(removed) == 48, len(removed)
mask = np.ones(len(faces), dtype=bool)
mask[removed] = False
h[9] -= len(removed)
prefix = bytearray(data[:tri])
struct.pack_into('<11i7f', prefix, 0, *h)
fields = list(struct.unpack_from('<3i4f6i8i4ii', prefix, obj))
assert fields[8] == len(faces)
fields[8] -= len(removed)
struct.pack_into('<3i4f6i8i4ii', prefix, obj, *fields)
(here/'no-earring-prototype.gm').write_bytes(prefix + faces[mask].tobytes() + data[buff:])
print('removed', len(removed), 'triangles; all vertices and surviving triangles unchanged')
