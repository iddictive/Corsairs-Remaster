"""List isolated GM triangle components in the head region for inspection."""
import math
import struct
import sys
from pathlib import Path
import numpy as np

data = Path(sys.argv[1]).read_bytes()
h = struct.unpack_from('<11i7f', data)
tri = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces = np.frombuffer(data, dtype='<u2', count=h[9]*3, offset=tri).reshape(-1, 3)
buff = tri + 6*h[9]
kind, size = struct.unpack_from('<2i', data, buff)
assert kind == 4 and size % 44 == 0
v = np.frombuffer(data, dtype=np.dtype({'names':['p'], 'formats':[('<f4',3)], 'itemsize':44}), count=size//44, offset=buff+8)['p']
parent = list(range(len(v)))
def root(i):
    while parent[i] != i:
        parent[i] = parent[parent[i]]
        i = parent[i]
    return i
for a,b,c in faces:
    a,b,c = map(int, (a,b,c))
    parent[root(b)] = root(a)
    parent[root(c)] = root(a)
groups = {}
for f in faces:
    groups.setdefault(root(int(f[0])), []).append(f)
for fs in sorted(groups.values(), key=len, reverse=True):
    ids = np.unique(fs)
    points = v[ids]
    if points[:,1].mean() < 1.6 or len(fs) < 2: continue
    c,s = math.cos(-.15),math.sin(-.15)
    x = ((points[:,0]*c+points[:,2]*s+.004)*(500/.41)+256)
    y = 490-(points[:,1]-1.465)*(500/.41)
    print('triangles',len(fs),'vertices',len(ids),'screen bounds', *(round(float(n),1) for n in (x.min(),y.min(),x.max(),y.max())), 'xyz',points.min(0).round(4),points.max(0).round(4))
