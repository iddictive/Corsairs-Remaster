"""Soften the rejected reference-depth fit across actual GM adjacency."""
import hashlib
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
root=here.parents[2]
base=(root/'src/assets/characters/putin-phonk/original.gm').read_bytes()
trial=(here/'scan-fit-prototype.gm').read_bytes()
assert len(base)==len(trial)
h=struct.unpack_from('<11i7f',base)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(base,dtype='<u2',count=h[9]*3,offset=off).reshape(-1,3).copy(); off+=6*h[9]
typ,size=struct.unpack_from('<2i',base,off); assert typ==4 and size%44==0
dtype=np.dtype({'names':['p','normal'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
out=bytearray(base); a=np.frombuffer(out,dtype=dtype,count=size//44,offset=off+8)
b=np.frombuffer(trial,dtype=dtype,count=size//44,offset=off+8)
orig=a['p'].copy(); target=b['p'][:,2]-orig[:,2]; field=target.copy()
edges=np.concatenate((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]],faces[:,[1,0]],faces[:,[2,1]],faces[:,[0,2]]))
allowed=(orig[:,1]>1.62)&(orig[:,1]<1.81)&(abs(orig[:,0])<.12)&(orig[:,2]>.035)
for _ in range(16):
    total=np.zeros(len(field)); count=np.zeros(len(field))
    np.add.at(total,edges[:,0],field[edges[:,1]]); np.add.at(count,edges[:,0],1)
    average=total/np.maximum(count,1)
    field=np.where(allowed,.65*average+.35*target,0)
a['p'][:,2]+=field
tri=a['p'][faces]; cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]); normal=np.zeros_like(a['p'])
for i in range(3):np.add.at(normal,faces[:,i],cross)
length=np.linalg.norm(normal,axis=1); valid=(np.abs(field)>1e-5)&(length>1e-8)
n=normal[valid]/length[valid,None]; old=a['normal'][valid].copy(); n[(n*old).sum(axis=1)<0]*=-1
a['normal'][valid]=n
path=here/'scan-smoothed-prototype.gm'; path.write_bytes(out)
print('changed',int(valid.sum()),'max shift',float(abs(field).max()),'sha256',hashlib.sha256(out).hexdigest())
