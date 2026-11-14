"""Bounded nose-depth trial against a generated side silhouette, not a scan."""
import math
import struct
import sys
from pathlib import Path

import numpy as np

here = Path(__file__).resolve().parent
source = bytearray((here/'identity-geometry-face-prototype.gm').read_bytes())
h = struct.unpack_from('<11i7f', source)
off = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind, size = struct.unpack_from('<2i', source, off)
assert kind == 4 and size % 44 == 0
dtype = np.dtype({'names':['p','n','uv'], 'formats':[('<f4',3),('<f4',3),('<f4',2)], 'offsets':[0,20,36], 'itemsize':44})
v = np.frombuffer(source, dtype=dtype, count=size//44, offset=off+8)
old = v['p'].astype(float).copy()
norm = v['n'].astype(float).copy()
zoom = 500/.41

def smooth(t):
    t = np.clip(t, 0, 1)
    return t*t*(3-2*t)

def warp(p):
    # Approximate edge differences at 512px, tapering to zero outside nose.
    y = 490-(p[:,1]-1.465)*zoom
    pixels = np.interp(y, [195,205,215,225,235,245,255,265], [0,-1,-4,-4,0,4,1,0])
    influence = smooth((.030-abs(p[:,0]-.008))/.016)*smooth((p[:,2]-.105)/.025)
    out = p.copy()
    out[:,2] += pixels/(zoom*math.cos(1.))*influence
    return out

new = warp(old)
eps = 1e-5
j = np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)], axis=2)
norm = np.linalg.solve(np.transpose(j,(0,2,1)), norm[...,None])[...,0]
norm /= np.maximum(np.linalg.norm(norm,axis=1,keepdims=True),1e-10)
assert np.isfinite(new).all() and np.isfinite(norm).all()
assert np.linalg.norm(new-old,axis=1).max() < .007
v['p'][:] = new
v['n'][:] = norm
# Preserve the frontal image projection for its split face island.
uv = v['uv'].astype(float).copy()
front = (uv[:,0]>.5)&(uv[:,0]<.75)&(uv[:,1]>=0)&(uv[:,1]<.25)
c,s = math.cos(-.15), math.sin(-.15)
uv[front,0] += ((new[front,0]-old[front,0])*c+(new[front,2]-old[front,2])*s)*zoom/2048
uv[front,1] -= (new[front,1]-old[front,1])*zoom/2048
v['uv'][:] = uv
(here/'profile-geometry-prototype.gm').write_bytes(source)
print('changed vertices',int(np.count_nonzero(np.linalg.norm(new-old,axis=1)>1e-7)), 'max movement',float(np.linalg.norm(new-old,axis=1).max()))

renderer = here/'render_officer.py'
for angle, output in [(-.15,'profile-geometry-front.png'),(1,'profile-geometry-profile.png'),(-1,'profile-geometry-other-profile.png')]:
    text = renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'", "f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert text.count(needle) == 1
        text = text.replace(needle, repr(output))
    sys.argv = [str(renderer),'officer_5',str(here/'identity-geometry-face-atlas.png'),str(angle),str(here/'profile-geometry-prototype.gm')]
    exec(compile(text,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
