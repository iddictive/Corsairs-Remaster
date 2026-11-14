"""Compare welded topology and bridge UV spans at 1e-6 game units."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
for name,bridge_count in [('cgmodelsnet-stockuv-retopology.gm',694),('cgmodelsnet-recut-trial.gm',344)]:
    data=(here/name).read_bytes();h=struct.unpack_from('<11i7f',data)
    tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
    faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3);off=tri+h[9]*6;kind,size=struct.unpack_from('<2i',data,off);assert kind==4
    dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});v=np.frombuffer(data,dt,size//44,off+8)
    _,ids=np.unique(np.rint(v['p']*1e6).astype('int64'),axis=0,return_inverse=True);f=ids[faces]
    e=np.concatenate((f[:,[0,1]],f[:,[1,2]],f[:,[2,0]]));s=np.sort(e,axis=1)
    _,inv,c=np.unique(s,axis=0,return_inverse=True,return_counts=True)
    signed=np.bincount(inv,weights=np.where(e[:,0]<e[:,1],1,-1))
    x=v['p'][faces];area=np.linalg.norm(np.cross(x[:,1]-x[:,0],x[:,2]-x[:,0]),axis=1)
    uv=v['uv'][faces[-bridge_count:]];span=np.abs(uv-np.roll(uv,1,axis=1)).max(axis=(1,2))
    print(name,'vertices',len(v),'faces',len(faces),'boundary',int(sum(c==1)),'nonmanifold',int(sum(c>2)),'same-winding shared',int(sum((c==2)&(abs(signed)==2))),'zero-area',int(sum(area<1e-12)),'bridge UV span >0.25',int(sum(span>.25)),'max span',round(float(span.max()),6))
