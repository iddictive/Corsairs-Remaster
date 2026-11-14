"""Experimental topology-guarded edge collapse on the CC BY head mesh.

Maintained alternatives: meshoptimizer and CGAL are not installed here; this
offline trial is limited to the task and checks its result independently.
"""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
raw=(here/'bobnojio-vvp.STL').read_bytes()
count=struct.unpack_from('<I',raw,80)[0];assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
tri=np.frombuffer(raw,dtype=dt,count=count,offset=84)['vertices']
tri=tri[np.all(tri[:,:,2]>=75.,axis=1)]
vertices,inv=np.unique(tri.reshape(-1,3),axis=0,return_inverse=True)
faces=inv.reshape(-1,3).astype(np.int32)
active_faces=np.ones(len(faces),dtype=bool)
live=np.ones(len(vertices),dtype=bool)
edges=np.sort(np.vstack((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]])),axis=1)
edges,degree=np.unique(edges,axis=0,return_counts=True)
assert np.all(degree<=2)
frozen=np.zeros(len(vertices),dtype=bool)
frozen[np.unique(edges[degree==1])]=True
neighbors=[set() for _ in vertices]
vertex_faces=[set() for _ in vertices]
for a,b in edges:
    neighbors[int(a)].add(int(b));neighbors[int(b)].add(int(a))
for i,(a,b,c) in enumerate(faces):
    vertex_faces[int(a)].add(i);vertex_faces[int(b)].add(i);vertex_faces[int(c)].add(i)
remaining=len(faces);collapsed=0
for turn in range(4):
    current=faces[active_faces]
    candidates=np.sort(np.vstack((current[:,[0,1]],current[:,[1,2]],current[:,[2,0]])),axis=1)
    candidates=np.unique(candidates,axis=0)
    candidates=candidates[~frozen[candidates].any(axis=1)]
    lengths=np.linalg.norm(vertices[candidates[:,0]]-vertices[candidates[:,1]],axis=1)
    candidates=candidates[np.argsort(lengths)]
    for a0,b0 in candidates:
        a,b=int(a0),int(b0)
        if not live[a] or not live[b] or b not in neighbors[a]:continue
        common=neighbors[a]&neighbors[b]
        shared=vertex_faces[a]&vertex_faces[b]
        if len(common)!=2 or len(shared)!=2:continue
        # Avoid foldovers when moving b's incident faces onto the existing a.
        changed=vertex_faces[b]-shared
        if changed:
            ids=np.fromiter(changed,dtype=np.int32)
            before=vertices[faces[ids]].astype(float)
            after=before.copy()
            which=faces[ids]==b
            after[which]=vertices[a]
            oldn=np.cross(before[:,1]-before[:,0],before[:,2]-before[:,0])
            newn=np.cross(after[:,1]-after[:,0],after[:,2]-after[:,0])
            dot=np.einsum('ij,ij->i',oldn,newn)
            norm=np.linalg.norm(oldn,axis=1)*np.linalg.norm(newn,axis=1)
            if np.any(norm<1e-12) or np.any(dot<.25*norm):continue
        for fid in shared:
            for k in faces[fid]:vertex_faces[int(k)].discard(fid)
            active_faces[fid]=False
        for fid in changed:
            face=faces[fid];face[face==b]=a;vertex_faces[a].add(fid)
        vertex_faces[b].clear()
        for k in tuple(neighbors[b]):
            neighbors[k].discard(b)
            if k!=a:
                neighbors[k].add(a);neighbors[a].add(k)
        neighbors[a].discard(b);neighbors[a].discard(a);neighbors[b].clear();live[b]=False
        remaining-=len(shared);collapsed+=1
        if remaining<=25000:break
    print('pass',turn+1,'faces',remaining,'collapsed',collapsed,flush=True)
    if remaining<=25000:break
faces=faces[active_faces]
edges=np.sort(np.vstack((faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]])),axis=1)
_,degree=np.unique(edges,axis=0,return_counts=True)
print('final boundary edges',int((degree==1).sum()),'nonmanifold edges',int((degree>2).sum()),flush=True)
assert not np.any(degree>2)
tri=vertices[faces];normals=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);length=np.linalg.norm(normals,axis=1)
assert np.all(length>1e-8);normals/=length[:,None]
out=np.zeros(len(tri),dtype=dt);out['normal']=normals;out['vertices']=tri
(here/'bobnojio-head-edge-collapse.STL').write_bytes(b'bobnojio CC BY 4.0 edge collapse trial'.ljust(80,b'\0')+struct.pack('<I',len(out))+out.tobytes())
