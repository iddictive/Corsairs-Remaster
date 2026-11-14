"""Visualize the prior 3D registration in the GM camera, without texture."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
dt=np.dtype([('n','<f4',3),('v','<f4',(3,3)),('attr','<u2')])
faces=np.memmap(here/'bobnojio-vvp.STL',dtype=dt,mode='r',offset=84)
verts=faces['v'].astype(np.float64)
points=np.concatenate((verts.reshape(-1,3),verts.mean(axis=1)))
cross=np.cross(verts[:,1]-verts[:,0],verts[:,2]-verts[:,0]);cross/=np.maximum(np.linalg.norm(cross,axis=1,keepdims=True),1e-10)
normals=np.concatenate((np.repeat(cross,3,axis=0),cross))
# The registration used by smooth_face_fit_trial.py, reproduced exactly.
p=np.column_stack((.010+points[:,0]*.0025,1.385+points[:,2]*.0028,.06-points[:,1]*.0025))
n=np.column_stack((normals[:,0],normals[:,2],-normals[:,1]))
keep=(p[:,1]>1.60)&(p[:,1]<1.83)&(abs(p[:,0]-.01)<.14)
p=p[keep];n=n[keep];scale=500/.41
for angle,label in [(-.15,'front'),(-math.pi/2,'profile')]:
    c,s=math.cos(angle),math.sin(angle)
    x=(p[:,0]*c+p[:,2]*s+.004)*scale+256; y=490-(p[:,1]-1.465)*scale
    z=-p[:,0]*s+p[:,2]*c; nx=n[:,0]*c+n[:,2]*s; nz=-n[:,0]*s+n[:,2]*c
    ix=np.floor(x).astype(int);iy=np.floor(y).astype(int);ok=(ix>=0)&(ix<512)&(iy>=0)&(iy<512)
    ix=ix[ok];iy=iy[ok];z=z[ok]; nn=np.column_stack((nx[ok],n[ok,1],nz[ok]))
    order=np.argsort(z);idx=iy*512+ix;selected=np.full(512*512,-1,dtype=int);selected[idx[order]]=order
    visible=selected>=0;light=np.array([-.35,.25,.9028]); light/=np.linalg.norm(light)
    shading=.35+.65*np.abs(nn[selected[visible]]@light);img=np.full((512*512,3),[18,22,30],dtype=np.uint8)
    img[visible]=np.clip(shading[:,None]*[210,213,218],0,255).astype(np.uint8)
    Image.fromarray(img.reshape(512,512,3)).save(here/f'sculpture-registered-{label}.png')

atlas=here/'sculpture-registration-neutral-atlas.png';Image.new('RGB',(1024,1024),(185,185,185)).save(atlas)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(-math.pi/2,'profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(f'stock-geometry-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(atlas),str(angle),str(here/'nose-subdivision-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
