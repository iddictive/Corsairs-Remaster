"""Non-delivery 3D facial depth fit, retaining GM topology, UV and skinning."""
import hashlib
import json
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
source=Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(source).hexdigest()=='e9f96665fdf59fd35c542ad7fc0f076749b7519aeae7c4dba6d055935a61e787'
size=struct.unpack_from('<I',source,12)[0]; meta=json.loads(source[20:20+size]); offset=20+size+8+meta['bufferViews'][1]['byteOffset']
scan=np.frombuffer(source,dtype='<f4',count=1019802*3,offset=offset).reshape(-1,3)
# Convert the reference bust's head coordinates into the stock head's frame.
ref=np.empty_like(scan); ref[:,0]=.015+.36*scan[:,0]; ref[:,1]=1.655+.358*scan[:,1]; ref[:,2]=.031+.36*scan[:,2]
res=512; lo=np.array([-.09,1.62]); hi=np.array([.12,1.83]); p=(ref[:,:2]-lo)/(hi-lo)*(res-1)
valid=(p[:,0]>=0)&(p[:,0]<res)&(p[:,1]>=0)&(p[:,1]<res)
ij=np.floor(p[valid]).astype(int); depth=np.full((res,res),-np.inf,dtype=np.float32)
np.maximum.at(depth,(ij[:,1],ij[:,0]),ref[valid,2])
# Fill sparse point pixels using their immediate neighbors, then sample locally.
for _ in range(3):
    expanded=depth.copy()
    for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        shifted=np.roll(depth,(dy,dx),(0,1)); expanded=np.where(np.isfinite(expanded),expanded,shifted)
    depth=expanded
gm=bytearray((here.parents[2]/'src/assets/characters/putin-phonk/original.gm').read_bytes())
h=struct.unpack_from('<11i7f',gm); off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(gm,dtype='<u2',count=h[9]*3,offset=off).reshape(-1,3).copy(); off+=6*h[9]
typ,sz=struct.unpack_from('<2i',gm,off); assert typ==4 and sz%44==0
dt=np.dtype({'names':['p','normal','bone','uv'],'formats':[('<f4',3),('<f4',3),'<u4',('<f4',2)],'offsets':[0,20,16,36],'itemsize':44})
v=np.frombuffer(gm,dtype=dt,count=sz//44,offset=off+8); points=v['p']; uv=v['uv']%1
coord=np.rint((points[:,:2]-lo)/(hi-lo)*(res-1)).astype(int); coord=np.clip(coord,0,res-1)
sample=depth[coord[:,1],coord[:,0]]
mask=((v['bone']&255)==16)&(points[:,1]>1.645)&(points[:,1]<1.79)&(points[:,2]>.055)&(uv[:,0]>.65)&(uv[:,1]>.34)&(uv[:,1]<.71)&np.isfinite(sample)
delta=np.clip((sample-points[:,2])*.35,-.008,.008)
# UV seams may duplicate the same vertex. Apply the same geometric delta to
# every duplicate, including faces assigned to a neighboring UV island.
keys=np.rint(points*100000).astype(np.int32)
mapped={tuple(keys[i]):float(delta[i]) for i in np.flatnonzero(mask)}
all_delta=np.array([mapped.get(tuple(k),0.) for k in keys],dtype=np.float32)
mask=all_delta!=0
points[:,2]+=all_delta
# Recompute area-weighted smooth normals for the altered region.
tri=points[faces]; cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]); normals=np.zeros_like(points)
for i in range(3): np.add.at(normals,faces[:,i],cross)
length=np.linalg.norm(normals,axis=1); valid=mask&(length>1e-8)
new=normals[valid]/length[valid,None]; old=v['normal'][valid].copy(); flip=(new*old).sum(axis=1)<0; new[flip]*=-1
v['normal'][valid]=new
assert mask.sum()>30
out=here/'scan-fit-prototype.gm'; out.write_bytes(gm)
print('modified vertices',int(mask.sum()),'max depth change',float(abs(all_delta).max()),'sha256',hashlib.sha256(gm).hexdigest())
