"""Experimental visibility-tested UV projection; writes only investigation outputs."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
root = here.parents[2]
model = sys.argv[3] if len(sys.argv)>3 else 'officer_5'
gm_path = root/'src/assets/characters/putin-phonk/original.gm' if model == 'officer_5' else root/f'experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Characters/{model}.gm'
gm = gm_path.read_bytes()
h = struct.unpack_from('<11i7f', gm)
o = 72 + h[2] + 4*h[3] + 4*h[4] + 56*h[5] + 80*h[6] + 108*h[7] + 104*h[8]
faces = np.frombuffer(gm, dtype='<u2', count=h[9]*3, offset=o).reshape(-1,3)
o += h[9]*6
typ, size = struct.unpack_from('<2i', gm, o); assert typ == 4 and size % 44 == 0
v = np.frombuffer(gm, dtype=np.dtype({'names':['p','uv'], 'formats':[('<f4',3),('<f4',2)], 'offsets':[0,36], 'itemsize':44}), count=size//44, offset=o+8)
pos = v['p'].astype(float); uv = v['uv'].astype(float) % 1
angle = -.15; c, s = math.cos(angle), math.sin(angle)
x = pos[:,0]*c + pos[:,2]*s; z = -pos[:,0]*s + pos[:,2]*c
screen = np.column_stack(((x+.004)*(500/.41)+256, 490-(pos[:,1]-1.465)*(500/.41), z))
depth = np.full((512,512), -np.inf)

def raster(tri, width, height):
    lo = np.maximum(np.floor(tri.min(axis=0)).astype(int), 0)
    hi = np.minimum(np.ceil(tri.max(axis=0)).astype(int), (width-1,height-1))
    if np.any(lo > hi): return None
    a,b,c = tri
    det = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
    if abs(det)<1e-8: return None
    yy,xx = np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; xx=xx+.5; yy=yy+.5
    w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/det
    w1=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/det
    w2=1-w0-w1
    return lo,hi,w0,w1,w2,(w0>=0)&(w1>=0)&(w2>=0)

for f in faces:
    p=screen[f]; r=raster(p[:,:2],512,512)
    if r is None: continue
    lo,hi,a,b,c,inside=r; area=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    zz=a*p[0,2]+b*p[1,2]+c*p[2,2]; np.maximum(area,np.where(inside,zz,-np.inf),out=area)

base=np.array(Image.open(here/f'{model}-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)).astype(float)
target_name = sys.argv[1] if len(sys.argv)>1 else 'projection-target.png'
output_name = sys.argv[2] if len(sys.argv)>2 else 'projection-atlas.png'
target=np.array(Image.open(here/target_name).convert('RGB').resize((512,512),Image.Resampling.LANCZOS)).astype(float)
updated=0
best=np.full((1024,1024),-np.inf)
sampled=np.zeros_like(base)
opacity=np.zeros((1024,1024))
for f in faces:
    u=uv[f]*1024
    if np.ptp(u[:,0])>512 or np.ptp(u[:,1])>512: continue
    xmin,ymin,ymax = (650,350,710) if model == 'officer_5' else (740,500,940)
    if u[:,0].max()<xmin or u[:,1].max()<ymin or u[:,1].min()>ymax: continue
    r=raster(u,1024,1024)
    if r is None: continue
    lo,hi,a,b,c,inside=r; p=screen[f]
    sx=a*p[0,0]+b*p[1,0]+c*p[2,0]; sy=a*p[0,1]+b*p[1,1]+c*p[2,1]; zz=a*p[0,2]+b*p[1,2]+c*p[2,2]
    ix=np.clip(sx.astype(int),0,511); iy=np.clip(sy.astype(int),0,511)
    # Fade within the central face. Avoid eyes/ears and UV islands outside the face.
    fade=np.clip((1-((sx-256)/59)**2-((sy-224)/100)**2)*3,0,1)
    mask=inside&(sx>=0)&(sx<512)&(sy>=0)&(sy<512)&(zz>=depth[iy,ix]-.003)&(fade>0)
    if not mask.any(): continue
    area=best[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    mask &= zz > area
    area[mask]=zz[mask]
    patch=sampled[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; patch[mask]=target[iy,ix][mask]
    alpha=opacity[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; alpha[mask]=fade[mask]
    updated+=int(mask.sum())
assert updated>1000, updated
base=base*(1-opacity[...,None])+sampled*opacity[...,None]
Image.fromarray(np.clip(base,0,255).astype('uint8')).save(here/output_name)
print('updated UV samples',updated)
