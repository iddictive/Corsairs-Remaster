"""Experiment: move the front lip area forward without altering skinning."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=bytearray((here/'no-earring-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,off)
assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8)
p=v['p'].astype(float).copy(); normals=v['n'].astype(float).copy()
def warp(p):
    z=p[:,2]; y=p[:,1]
    t=np.clip((z-.045)/.035,0,1)
    blend=t*t*(3-2*t)
    depth=.008*np.exp(-.5*((y-1.639)/.014)**2)*blend
    q=p.copy(); q[:,2]+=depth
    return q
q=warp(p); eps=1e-5
j=np.stack([(warp(p+np.eye(3)[i]*eps)-warp(p-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
n=np.linalg.solve(np.transpose(j,(0,2,1)),normals[...,None])[...,0]
n/=np.maximum(np.linalg.norm(n,axis=1,keepdims=True),1e-10)
v['p'][:]=q; v['n'][:]=n
dist=np.linalg.norm(q-p,axis=1)
assert np.isfinite(n).all() and dist.max()<.0081
(here/'lips-depth-prototype.gm').write_bytes(data)
print('moved',int((dist>1e-6).sum()),'vertices, max',round(float(dist.max()),6))
