"""Bounded depth-field fit of CC BY bust onto the skinned stock head."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
mesh=here/'bobnojio-vvp.STL'
dt=np.dtype([('n','<f4',3),('v','<f4',(3,3)),('attr','<u2')])
ref=np.memmap(mesh,dtype=dt,mode='r',offset=84)['v'].reshape(-1,3)
res=256; lo=np.array([-.09,1.60]); hi=np.array([.11,1.80])
x=.010+ref[:,0]*.0025
y=1.385+ref[:,2]*.0028
z=.06-ref[:,1]*.0025
ix=((x-lo[0])/(hi[0]-lo[0])*(res-1)).astype(int)
iy=((y-lo[1])/(hi[1]-lo[1])*(res-1)).astype(int)
valid=(ix>=0)&(ix<res)&(iy>=0)&(iy<res)
depth=np.full((res,res),-np.inf,dtype=np.float32)
np.maximum.at(depth,(iy[valid],ix[valid]),z[valid])
for _ in range(12):
    old=depth.copy()
    for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        shifted=np.roll(old,(dy,dx),(0,1))
        if dy<0:shifted[-1,:]=-np.inf
        if dy>0:shifted[0,:]=-np.inf
        if dx<0:shifted[:,-1]=-np.inf
        if dx>0:shifted[:,0]=-np.inf
        depth=np.where(np.isfinite(depth),depth,shifted)
data=bytearray((here/'photo-face-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,off); assert kind==4 and size%44==0
dtype=np.dtype({'names':['p','n','bone'],'formats':[('<f4',3),('<f4',3),'<u4'],'offsets':[0,20,16],'itemsize':44})
v=np.frombuffer(data,dtype=dtype,count=size//44,offset=off+8)
old=v['p'].astype(float).copy()
xy=np.rint((old[:,:2]-lo)/(hi-lo)*(res-1)).astype(int)
xy=np.clip(xy,0,res-1); sample=depth[xy[:,1],xy[:,0]]
eligible=((v['bone']&255)==16)&(old[:,1]>1.62)&(old[:,1]<1.78)&(old[:,2]>.065)&(np.abs(old[:,0]-.01)<.065)&np.isfinite(sample)
raw=np.zeros(len(old)); raw[eligible]=sample[eligible]-old[eligible,2]
print('eligible',eligible.sum(),'raw deviation min/median/max',np.round(np.percentile(raw[eligible],[0,50,100]),4))
# Small bounded experiment only. Recalculate normals from retained triangles.
delta=np.clip(raw*.2,-.004,.004)
v['p'][:,2]=old[:,2]+delta
tri_at=off-6*h[9]
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
p=v['p'].astype(float); triangles=p[faces]
cross=np.cross(triangles[:,1]-triangles[:,0],triangles[:,2]-triangles[:,0])
normal=np.zeros_like(p)
for i in range(3):np.add.at(normal,faces[:,i],cross)
length=np.linalg.norm(normal,axis=1)
mask=(abs(delta)>1e-7)&(length>1e-8)
nn=normal[mask]/length[mask,None]; oldn=v['n'][mask].copy(); flip=(nn*oldn).sum(1)<0; nn[flip]*=-1
v['n'][mask]=nn
assert np.isfinite(v['p']).all() and np.isfinite(v['n']).all()
(here/'bobnojio-fit-prototype.gm').write_bytes(data)
print('moved',int(mask.sum()),'max depth change',round(float(abs(delta).max()),6))
