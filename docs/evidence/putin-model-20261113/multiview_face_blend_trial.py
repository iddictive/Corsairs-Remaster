"""Blend a side-view correction into the existing projected face island."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
data=(here/'profile-geometry-prototype.gm').read_bytes()
h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,dtype='<u2',count=3*h[9],offset=tri).reshape(-1,3)
off=tri+6*h[9]; typ,size=struct.unpack_from('<2i',data,off)
assert typ==4 and size%44==0
dt=np.dtype({'names':['p','n','uv'],'formats':[('<f4',3),('<f4',3),('<f4',2)],'offsets':[0,20,36],'itemsize':44})
verts=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8)
p=verts['p'].astype(float); n=verts['n'].astype(float); uv=verts['uv'].astype(float)
scale=500/.41; angle=1.; c,s=math.cos(angle),math.sin(angle)
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*scale+256,490-(p[:,1]-1.465)*scale,-p[:,0]*s+p[:,2]*c))
side_n=np.column_stack((n[:,0]*c+n[:,2]*s,n[:,1],-n[:,0]*s+n[:,2]*c))
depth=np.full((512,512),-np.inf)

def pixels(q,limit=512):
    lo=np.maximum(np.floor(q.min(axis=0)).astype(int),0); hi=np.minimum(np.ceil(q.max(axis=0)).astype(int),limit-1)
    if np.any(lo>hi):return None
    a,b,d=q; det=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1])
    if abs(det)<1e-8:return None
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; x=xx+.5; y=yy+.5
    w0=((b[1]-d[1])*(x-d[0])+(d[0]-b[0])*(y-d[1]))/det
    w1=((d[1]-a[1])*(x-d[0])+(a[0]-d[0])*(y-d[1]))/det
    w2=1-w0-w1
    return xx,yy,np.stack((w0,w1,w2),axis=-1),(w0>=0)&(w1>=0)&(w2>=0)

for face in faces:
    q=screen[face]; result=pixels(q[:,:2])
    if result is None:continue
    x,y,w,inside=result; z=w@q[:,2]; area=depth[y,x]
    depth[y,x]=np.where(inside & (z>area),z,area)

target=np.asarray(Image.open(here/'profile-refinement-target.png').convert('RGB').resize((512,512),Image.Resampling.LANCZOS),dtype=float)
baseline=np.asarray(Image.open(here/'profile-geometry-profile.png').convert('RGB'),dtype=float)
delta=np.clip(target-baseline,-30,30)
atlas=np.asarray(Image.open(here/'identity-geometry-face-atlas.png').convert('RGB'),dtype=float).copy()
original_atlas=atlas.copy()
front_depth=np.full((512,512),-np.inf); changed=np.zeros((512,512),bool)
for face in faces:
    t=uv[face]; q=np.column_stack(((t[:,0]-.5)*2048,t[:,1]*2048))
    if not (np.all((q[:,0]>=0)&(q[:,0]<512)) and np.all((q[:,1]>=0)&(q[:,1]<512))):continue
    result=pixels(q)
    if result is None:continue
    x,y,w,inside=result; xyz=w@p[face]; normals=w@n[face]; side=w@screen[face]; normal_side=w@side_n[face]
    sx=np.clip(np.floor(side[...,0]).astype(int),0,511); sy=np.clip(np.floor(side[...,1]).astype(int),0,511)
    frontz=-xyz[...,0]*math.sin(-.15)+xyz[...,2]*math.cos(-.15)
    visible=(side[...,2]>=depth[sy,sx]-.001)&(side[...,0]>=0)&(side[...,0]<512)&(side[...,1]>=0)&(side[...,1]<512)
    length=np.maximum(np.linalg.norm(normals,axis=-1),1e-8); normals=normals/length[...,None]
    weight=np.clip((.8-normals[...,2])/.55,0,1)*np.clip((normal_side[...,2]-.15)/.5,0,1)*.65
    valid=inside&visible&(frontz>front_depth[y,x])
    lighting=.72+.28*np.maximum(0,(normal_side@np.array([-.35,.25,.9028]))/length)
    region=atlas[y,x+1024]; original=original_atlas[y,x+1024]
    correction=delta[sy,sx]*weight[...,None]/lighting[...,None]
    region[valid]=np.clip(original[valid]+correction[valid],0,255)
    atlas[y,x+1024]=region; front_depth[y,x]=np.where(valid,frontz,front_depth[y,x]); changed[y,x]|=valid & (weight>.05)
Image.fromarray(atlas.astype('uint8')).save(here/'multiview-face-blend-atlas.png')
print('corrected face texels',int(changed.sum()))
renderer=here/'render_officer.py'
for angle,out in [(-.15,'multiview-face-blend-front.png'),(1,'multiview-face-blend-profile.png'),(-1,'multiview-face-blend-other-profile.png')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(out))
    sys.argv=[str(renderer),'officer_5',str(here/'multiview-face-blend-atlas.png'),str(angle),str(here/'profile-geometry-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
