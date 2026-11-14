"""Offline conservative edge-collapse trial; never changes the candidate."""
import struct
from pathlib import Path
import numpy as np

here = Path(__file__).resolve().parent
data = (here / 'cgmodelsnet-stockuv-retopology.gm').read_bytes()
h = list(struct.unpack_from('<11i7f', data))
obj = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri = obj+104*h[8]
faces = np.frombuffer(data, '<u2', h[9]*3, tri).reshape(-1,3).copy()
off = tri+6*h[9]
kind, size = struct.unpack_from('<2i', data, off)
assert kind == 4 and size % 44 == 0
start = off+8
rows = np.frombuffer(data, dtype='u1', count=size, offset=start).reshape(-1,44).copy()
dt = np.dtype({'names':['p','uv'], 'formats':[('<f4',3),('<f4',2)], 'offsets':[0,36], 'itemsize':44})
v = rows.reshape(-1).view(dt)
p, uv = v['p'].astype(float), v['uv'].astype(float)
head = np.all((faces>=2106)&(faces<14993),axis=1)
assert head.sum()==25000 and not np.any((~head)&np.any((faces>=2106)&(faces<14993),axis=1))
f = faces[head].astype(np.int32)
active = np.ones(len(f),bool)
live = np.ones(len(rows),bool)
edges = np.sort(np.vstack((f[:,[0,1]],f[:,[1,2]],f[:,[2,0]])),axis=1)
edges, degree = np.unique(edges,axis=0,return_counts=True)
assert degree.max() <= 2
frozen = np.zeros(len(rows),bool)
frozen[np.unique(edges[degree==1])] = True
neighbors = [set() for _ in rows]
vertex_faces = [set() for _ in rows]
for a,b in edges:
    neighbors[int(a)].add(int(b)); neighbors[int(b)].add(int(a))
for i,face in enumerate(f):
    for a in face: vertex_faces[int(a)].add(i)
remaining=len(f)
for turn in range(4):
    current=f[active]
    edges=np.sort(np.vstack((current[:,[0,1]],current[:,[1,2]],current[:,[2,0]])),axis=1)
    edges=np.unique(edges,axis=0)
    edges=edges[~frozen[edges].any(axis=1)]
    edges=edges[np.max(np.abs(uv[edges[:,0]]-uv[edges[:,1]]),axis=1)<.02]
    edges=edges[np.argsort(np.linalg.norm(p[edges[:,0]]-p[edges[:,1]],axis=1))]
    for a,b in edges:
        a,b=int(a),int(b)
        if not live[a] or not live[b] or b not in neighbors[a]: continue
        common=neighbors[a]&neighbors[b]; shared=vertex_faces[a]&vertex_faces[b]
        if len(common)!=2 or len(shared)!=2: continue
        changed=vertex_faces[b]-shared
        if changed:
            ids=np.fromiter(changed,dtype=np.int32)
            before=p[f[ids]]; after=before.copy(); after[f[ids]==b]=p[a]
            old=np.cross(before[:,1]-before[:,0],before[:,2]-before[:,0])
            new=np.cross(after[:,1]-after[:,0],after[:,2]-after[:,0])
            den=np.linalg.norm(old,axis=1)*np.linalg.norm(new,axis=1)
            if np.any(den<1e-15) or np.any(np.einsum('ij,ij->i',old,new)<.85*den): continue
            if np.max(np.linalg.norm(after-np.roll(after,1,axis=1),axis=2))>.006: continue
        for fid in shared:
            for k in f[fid]: vertex_faces[int(k)].discard(fid)
            active[fid]=False
        for fid in changed:
            face=f[fid];face[face==b]=a;vertex_faces[a].add(fid)
        vertex_faces[b].clear()
        for k in tuple(neighbors[b]):
            neighbors[k].discard(b)
            if k!=a: neighbors[k].add(a);neighbors[a].add(k)
        neighbors[a].discard(b);neighbors[a].discard(a);neighbors[b].clear();live[b]=False
        remaining-=2
        if remaining<=12000: break
    print('pass',turn+1,'head faces',remaining,flush=True)
    if remaining<=12000: break
out=np.concatenate((faces[:np.flatnonzero(head)[0]],f[active],faces[np.flatnonzero(head)[-1]+1:])).astype(np.int32)
assert len(out)==len(faces)-25000+remaining
used=np.unique(out)
keep=np.concatenate((np.arange(2106),used[(used>=2106)&(used<14993)],np.arange(14993,len(rows))))
remap=np.full(len(rows),-1,dtype=np.int32); remap[keep]=np.arange(len(keep))
out=remap[out];assert out.min()>=0 and out.max()<65536
prefix=bytearray(data[:tri]);h[9]=len(out);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(out);fields[10]=len(keep)
struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
(here/'cgmodelsnet-head-decimated-trial.gm').write_bytes(prefix+out.astype('<u2').tobytes()+struct.pack('<2i',4,len(keep)*44)+rows[keep].tobytes()+data[start+size:])
print('vertices',len(rows),'->',len(keep),'faces',len(faces),'->',len(out),flush=True)
