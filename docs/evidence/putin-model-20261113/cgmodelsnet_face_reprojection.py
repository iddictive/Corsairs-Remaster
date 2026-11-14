"""Project a face-only render correction onto the graft's cylindrical UVs."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
data=(here/'cgmodelsnet-bridge-shaped-radialfit-prototype.gm').read_bytes()
h=struct.unpack_from('<11i7f',data);tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3);off=tri+6*h[9];typ,size=struct.unpack_from('<2i',data,off);assert typ==4 and size%44==0
dt=np.dtype({'names':['p','n','uv'],'formats':[('<f4',3),('<f4',3),('<f4',2)],'offsets':[0,20,36],'itemsize':44})
v=np.frombuffer(data,dt,size//44,off+8);p=v['p'].astype(float);n=v['n'].astype(float);uv=v['uv'].astype(float)
angle=-.15;c,s=math.cos(angle),math.sin(angle);scale=500/.41
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*scale+256,490-(p[:,1]-1.465)*scale,-p[:,0]*s+p[:,2]*c))
norm=np.column_stack((n[:,0]*c+n[:,2]*s,n[:,1],-n[:,0]*s+n[:,2]*c))
def raster(q,w,h):
    lo=np.maximum(np.floor(q.min(0)).astype(int),0);hi=np.minimum(np.ceil(q.max(0)).astype(int),(w-1,h-1))
    if np.any(lo>hi):return None
    a,b,d=q;det=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1])
    if abs(det)<1e-9:return None
    y,x=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];xx=x+.5;yy=y+.5
    w0=((b[1]-d[1])*(xx-d[0])+(d[0]-b[0])*(yy-d[1]))/det
    w1=((d[1]-a[1])*(xx-d[0])+(a[0]-d[0])*(yy-d[1]))/det;w2=1-w0-w1
    return x,y,np.stack((w0,w1,w2),axis=-1),(w0>=0)&(w1>=0)&(w2>=0)
depth=np.full((512,512),-np.inf)
for face in faces:
    q=screen[face];r=raster(q[:,:2],512,512)
    if r is None:continue
    x,y,w,inside=r;z=w@q[:,2];old=depth[y,x];depth[y,x]=np.where(inside&(z>old),z,old)
baseline=np.asarray(Image.open(here/'cgmodelsnet-radialfit-repaired-front.png').convert('RGB'),dtype=float)
target=np.asarray(Image.open(here/'cgmodelsnet-face-refined-target.png').convert('RGB').resize((512,512),Image.Resampling.LANCZOS),dtype=float)
delta=np.clip(target-baseline,-100,100)
original=np.asarray(Image.open(here/'cgmodelsnet-radialfit-repaired-atlas.png').convert('RGB'),dtype=float);atlas=original.copy()
best=np.full((1024,2048),-np.inf);changed=np.zeros((1024,2048),bool)
for face in faces:
    if not np.all(face>=2106):continue
    q=uv[face]*2048;q[:,1]-=1024
    if np.any(q[:,0]<0) or np.any(q[:,0]>=2048) or np.any(q[:,1]<0) or np.any(q[:,1]>=1024):continue
    r=raster(q,2048,1024)
    if r is None:continue
    x,y,w,inside=r;sc=w@screen[face];nn=w@norm[face]
    sx=np.clip(np.floor(sc[...,0]).astype(int),0,511);sy=np.clip(np.floor(sc[...,1]).astype(int),0,511)
    length=np.maximum(np.linalg.norm(nn,axis=-1),1e-9);normal=nn/length[...,None]
    fade=np.clip((1-((sc[...,0]-255)/85)**2-((sc[...,1]-222)/95)**2)*3,0,1)
    valid=inside&(sc[...,0]>=0)&(sc[...,0]<512)&(sc[...,1]>=0)&(sc[...,1]<512)&(sc[...,2]>=depth[sy,sx]-.002)&(normal[...,2]>.2)&(fade>0)&(sc[...,2]>best[y,x])
    if not valid.any():continue
    light=.72+.28*np.maximum(0,normal@np.array([-.35,.25,.9028]))
    patch=atlas[y+1024,x];base=original[y+1024,x];candidate=np.clip(base+delta[sy,sx]*fade[...,None]/light[...,None],0,255)
    patch[valid]=candidate[valid];atlas[y+1024,x]=patch;best[y,x]=np.where(valid,sc[...,2],best[y,x]);changed[y,x]|=valid
Image.fromarray(atlas.astype('uint8')).save(here/'cgmodelsnet-face-refined-atlas.png')
print('updated head texels',int(changed.sum()))
renderer=here/'render_officer.py'
for a,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(f'cgmodelsnet-face-refined-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-face-refined-atlas.png'),str(a),str(here/'cgmodelsnet-bridge-shaped-radialfit-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
