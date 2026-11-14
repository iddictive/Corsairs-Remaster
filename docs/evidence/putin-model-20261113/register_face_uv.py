"""Fit facial texture landmarks to the stock mesh without changing skinning.

The face texture is sampled through the same GM surface, rather than baking a
lit portrait into the atlas. This is an offline prototype, not delivery.
"""
import json
import math
import struct
from pathlib import Path

import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
root = here.parents[2]
gm = (root/'src/assets/characters/putin-phonk/original.gm').read_bytes()
h = struct.unpack_from('<11i7f',gm)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(gm,dtype='<u2',count=h[9]*3,offset=off).reshape(-1,3)
off+=h[9]*6
typ,size=struct.unpack_from('<2i',gm,off); assert typ==4 and size%44==0
dtype=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44})
v=np.frombuffer(gm,dtype=dtype,count=size//44,offset=off+8)
p=v['p'].astype(float); uv=v['uv'].astype(float)%1
a=-.15; c,s=math.cos(a),math.sin(a); zoom=500/.41
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*zoom+256,490-(p[:,1]-1.465)*zoom,-p[:,0]*s+p[:,2]*c))

def raster(tri,width,height):
    lo=np.maximum(np.floor(tri.min(0)).astype(int),0); hi=np.minimum(np.ceil(tri.max(0)).astype(int),(width-1,height-1))
    if np.any(lo>hi): return None
    a,b,c=tri; den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
    if abs(den)<1e-8:return None
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; xx=xx+.5; yy=yy+.5
    w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den
    w1=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den; w2=1-w0-w1
    return lo,hi,w0,w1,w2,(w0>=0)&(w1>=0)&(w2>=0)

# Project skin UV to screen, storing the nearest skin triangle at each pixel.
depth=np.full((512,512),-np.inf); mapping=np.full((512,512,2),np.nan)
for face in faces:
    tex=uv[face]; p=screen[face]
    if np.ptp(tex[:,0])>.5 or np.ptp(tex[:,1])>.5:continue
    if np.min(tex[:,0])<.64 or np.min(tex[:,1])<.33 or np.max(tex[:,1])>.72:continue
    row=raster(p[:,:2],512,512)
    if row is None:continue
    lo,hi,w0,w1,w2,inside=row; z=w0*p[0,2]+w1*p[1,2]+w2*p[2,2]
    area=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; mask=inside&(z>area)
    area[mask]=z[mask]
    out=mapping[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    for k in range(2):out[:,:,k][mask]=(w0*tex[0,k]+w1*tex[1,k]+w2*tex[2,k])[mask]

orig=json.loads((here/'officer_5-original-preview-landmarks.json').read_text())
candidate=json.loads((here/'officer_5-candidate-preview-landmarks.json').read_text())
regions=('nose','noseCrest','outerLips')
controls=np.array([p for key in regions for p in orig[key]],dtype=float)
target=np.array([p for key in regions for p in candidate[key]],dtype=float)
# Anchor outside the face to zero displacement so the existing hair, ears,
# neckline and costume retain their original UVs.
anchors=np.array([(x,y) for x in (155,180,330,355) for y in (130,200,280,345)] + [(x,y) for x in (210,255,300) for y in (115,185,350)],dtype=float)
delta=np.vstack((np.clip(target-controls,-28,28),np.zeros_like(anchors)))
controls=np.vstack((controls,anchors)); sigma=24.
dd=controls[:,None,:]-controls[None,:,:]
kernel=np.exp(-np.sum(dd*dd,axis=2)/(2*sigma*sigma))
weights=np.linalg.solve(kernel+.08*np.eye(len(controls)),delta)
atlas=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)).copy()
result=atlas.copy(); best=np.full((1024,1024),-np.inf); opacity=np.zeros((1024,1024)); changed=0
for face in faces:
    tex=uv[face]; p=screen[face]
    if np.ptp(tex[:,0])>.5 or np.ptp(tex[:,1])>.5:continue
    if np.min(tex[:,0])<.64 or np.min(tex[:,1])<.33 or np.max(tex[:,1])>.72:continue
    row=raster(tex*1024,1024,1024)
    if row is None:continue
    lo,hi,w0,w1,w2,inside=row
    sx=w0*p[0,0]+w1*p[1,0]+w2*p[2,0]; sy=w0*p[0,1]+w1*p[1,1]+w2*p[2,1]; z=w0*p[0,2]+w1*p[1,2]+w2*p[2,2]
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    flat=np.column_stack((sx.ravel(),sy.ravel()))
    diff=flat[:,None,:]-controls[None,:,:]
    displacement=np.exp(-np.sum(diff*diff,axis=2)/(2*sigma*sigma))@weights
    q=(flat+displacement).reshape((*sx.shape,2)); xi=np.clip(np.rint(q[:,:,0]).astype(int),0,511); yi=np.clip(np.rint(q[:,:,1]).astype(int),0,511)
    suv=mapping[yi,xi]; ok=np.isfinite(suv).all(axis=2)
    ix=np.clip(np.nan_to_num(suv[:,:,0]*1024).astype(int),0,1023); iy=np.clip(np.nan_to_num(suv[:,:,1]*1024).astype(int),0,1023)
    outdepth=best[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    # Never project onto the cheek/ear seams; taper the correction to zero.
    fade=np.clip((1-((sx-255)/58)**2-((sy-245)/72)**2)*3,0,1)
    fade*=np.clip((xx-755)/20,0,1)*np.clip((665-yy)/20,0,1)
    mask=inside&ok&(z>outdepth)&(fade>0)
    if not mask.any():continue
    outdepth[mask]=z[mask]
    out=result[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; out[mask]=atlas[iy,ix][mask]
    outalpha=opacity[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; outalpha[mask]=fade[mask]
    changed+=int(mask.sum())
assert changed>1000,changed
blended=np.clip(atlas.astype(float)*(1-opacity[...,None])+result.astype(float)*opacity[...,None],0,255).astype('uint8')
Image.fromarray(blended).save(here/'registered-atlas.png')
print('mapped UV samples',changed,'max landmark shift',round(float(np.linalg.norm(target-controls[:len(target)],axis=1).max()),2))
