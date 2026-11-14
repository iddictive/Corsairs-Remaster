"""Experimental voxel reduction of a head cut from the CC BY source bust."""
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
source=here/'bobnojio-vvp.STL'
raw=source.read_bytes()
count=struct.unpack_from('<I',raw,80)[0]
assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
faces=np.frombuffer(raw,dtype=dt,count=count,offset=84)['vertices']
# Cut under the jaw; no attempt is made yet to close or fit the neck boundary.
head=faces[np.all(faces[:,:,2]>=75.,axis=1)]
points=head.reshape(-1,3).astype(np.float64)
voxel=float(sys.argv[1]) if len(sys.argv)>1 else 1.0
assert .5<=voxel<=3.0
keys=np.floor(points/voxel).astype(np.int32)
_,inverse=np.unique(keys,axis=0,return_inverse=True)
counts=np.bincount(inverse)
vertices=np.column_stack([np.bincount(inverse,weights=points[:,i])/counts for i in range(3)])
ids=inverse.reshape(-1,3)
valid=(ids[:,0]!=ids[:,1])&(ids[:,1]!=ids[:,2])&(ids[:,0]!=ids[:,2])
ids=ids[valid]
# Remove geometrically duplicated triangles while retaining their winding.
_,indices=np.unique(np.sort(ids,axis=1),axis=0,return_index=True)
ids=ids[np.sort(indices)]
tri=vertices[ids]
normal=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0])
length=np.linalg.norm(normal,axis=1)
good=length>1e-8
tri,normal,length=tri[good],normal[good],length[good]
normal/=length[:,None]
out=np.zeros(len(tri),dtype=dt)
out['normal']=normal; out['vertices']=tri
path=here/f'bobnojio-head-reduced-{voxel:g}.STL'
path.write_bytes(b'bobnojio CC BY 4.0 experimental head cut'.ljust(80,b'\0')+struct.pack('<I',len(out))+out.tobytes())
print('source faces',count,'cut faces',len(head),'reduced faces',len(out),'cluster vertices',len(vertices),'voxel',voxel)
print('open neckline: not yet joined to stock body; no UV or skin weights')
