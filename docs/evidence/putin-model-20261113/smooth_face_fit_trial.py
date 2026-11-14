"""Fit a smooth front depth field while keeping the stock head and skinning."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
mesh=here/'bobnojio-vvp.STL'
stldt=np.dtype([('n','<f4',3),('v','<f4',(3,3)),('attr','<u2')])
ref=np.memmap(mesh,dtype=stldt,mode='r',offset=84)['v'].reshape(-1,3)
res=256;lo=np.array([-.075,1.63]);hi=np.array([.095,1.79])
x=.010+ref[:,0]*.0025;y=1.385+ref[:,2]*.0028;z=.06-ref[:,1]*.0025
ix=np.floor((x-lo[0])/(hi[0]-lo[0])*(res-1)).astype(int)
iy=np.floor((y-lo[1])/(hi[1]-lo[1])*(res-1)).astype(int)
valid=(ix>=0)&(ix<res)&(iy>=0)&(iy<res)&(z>.065)&(z<.17)
depth=np.full((res,res),-np.inf,dtype=np.float32)
np.maximum.at(depth,(iy[valid],ix[valid]),z[valid])
for _ in range(4):
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
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n','w','bone'],'formats':[('<f4',3),('<f4',3),'<f4','<u4'],'offsets':[0,20,12,16],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8)
old=v['p'].astype(float).copy();normals=v['n'].astype(float).copy()
coords=np.rint((old[:,:2]-lo)/(hi-lo)*(res-1)).astype(int);coords=np.clip(coords,0,res-1)
target=depth[coords[:,1],coords[:,0]];diff=target-old[:,2]
eligible=((v['bone']&255)==16)&(v['w']>.99)&(old[:,1]>1.65)&(old[:,1]<1.75)&(abs(old[:,0]-.01)<.055)&(old[:,2]>.085)&np.isfinite(target)&(abs(diff)<.035)
# Repeated vertices must receive exactly the same displacement.
unique,idx=np.unique(np.round(old[eligible],6),axis=0,return_index=True)
samples=np.flatnonzero(eligible)[idx];controls=old[samples,:2];response=diff[samples]
assert len(controls)>=20
offset=np.median(response);response=np.clip(response-offset,-.012,.012)
sigma=.018;d=controls[:,None,:]-controls[None,:,:]
k=np.exp(-np.sum(d*d,axis=2)/(2*sigma*sigma))
coef=np.linalg.solve(k+.5*np.eye(len(controls)),response)
def smooth(t):
    t=np.clip(t,0,1);return t*t*(3-2*t)
def warp(p):
    d=p[:,None,:2]-controls[None,:,:]
    f=np.exp(-np.sum(d*d,axis=2)/(2*sigma*sigma))@coef
    fade=smooth((p[:,2]-.065)/.035)*smooth((p[:,1]-1.625)/.035)*smooth((1.785-p[:,1])/.04)
    q=p.copy();q[:,2]+=np.clip(.6*f*fade,-.006,.006)
    return q
new=warp(old);eps=1e-5
jac=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
nn=np.linalg.solve(np.transpose(jac,(0,2,1)),normals[...,None])[...,0]
nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-10)
v['p'][:]=new;v['n'][:]=nn
change=np.linalg.norm(new-old,axis=1)
assert np.isfinite(new).all() and np.isfinite(nn).all() and change.max()<=.00601
(here/'smooth-face-fit-prototype.gm').write_bytes(data)
print('controls',len(controls),'median offset removed',round(float(offset),6),'moved',int((change>1e-6).sum()),'max',round(float(change.max()),6))
