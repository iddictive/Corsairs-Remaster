"""Reduce only vertex clusters joined by actual source-mesh edges."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
raw=(here/'bobnojio-vvp.STL').read_bytes()
count=struct.unpack_from('<I',raw,80)[0];assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
f=np.frombuffer(raw,dtype=dt,count=count,offset=84)['vertices']
f=f[np.all(f[:,:,2]>=75.,axis=1)]
points,inv=np.unique(f.reshape(-1,3),axis=0,return_inverse=True)
faces=inv.reshape(-1,3)
edges=np.sort(np.vstack((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]])),axis=1)
edges=np.unique(edges,axis=0)
voxel=1.5
cells=np.floor(points/voxel).astype(np.int32)
edges=edges[np.all(cells[edges[:,0]]==cells[edges[:,1]],axis=1)]
parent=np.arange(len(points),dtype=np.int32)
def root(i):
    while parent[i]!=i:
        parent[i]=parent[parent[i]]
        i=parent[i]
    return int(i)
for a,b in edges:
    a,b=root(int(a)),root(int(b))
    if a!=b:parent[b]=a
roots=np.array([root(i) for i in range(len(points))],dtype=np.int32)
_,inverse=np.unique(roots,return_inverse=True)
counts=np.bincount(inverse)
vertices=np.column_stack([np.bincount(inverse,weights=points[:,i])/counts for i in range(3)])
ids=inverse[faces]
ids=ids[(ids[:,0]!=ids[:,1])&(ids[:,1]!=ids[:,2])&(ids[:,0]!=ids[:,2])]
_,first=np.unique(np.sort(ids,axis=1),axis=0,return_index=True);ids=ids[np.sort(first)]
tri=vertices[ids];normal=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);length=np.linalg.norm(normal,axis=1)
good=length>1e-8;tri,normal,length,ids=tri[good],normal[good],length[good],ids[good];normal/=length[:,None]
out=np.zeros(len(tri),dtype=dt);out['normal']=normal;out['vertices']=tri
(here/'bobnojio-head-connected-1.5.STL').write_bytes(b'bobnojio CC BY 4.0 connected-cluster trial'.ljust(80,b'\0')+struct.pack('<I',len(out))+out.tobytes())
e=np.sort(np.vstack((ids[:,[0,1]],ids[:,[1,2]],ids[:,[2,0]])),axis=1)
_,degree=np.unique(e,axis=0,return_counts=True)
print('vertices',len(vertices),'faces',len(ids),'open edges',int((degree==1).sum()),'nonmanifold',int((degree>2).sum()))
