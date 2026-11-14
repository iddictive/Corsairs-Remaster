"""Test moving the body-side neck toward the head bone over a connected band."""
import collections
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-stockuv-retopology.gm').read_bytes())
h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3)
off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
dt=np.dtype({'names':['p','w','b'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44})
v=np.frombuffer(data,dt,size//44,off+8);p=v['p'].astype(float).copy();n=len(v)
body=(np.arange(n)<2106)|((np.arange(n)>=14993)&(np.arange(n)<15016))|(np.arange(n)>=15623)
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
allowed=body&(field>-.060)&(abs(p[:,0]-.01)<.12)&(abs(p[:,2])<.16)
keys,inv=np.unique(np.rint(p*1e6).astype('int64'),axis=0,return_inverse=True)
graph=[set() for _ in keys]
bodyfaces=faces[np.all(body[faces],axis=1)]
for face in inv[bodyfaces]:
    for a,b in ((face[0],face[1]),(face[1],face[2]),(face[2],face[0])):
        graph[a].add(b);graph[b].add(a)
bridge=np.unique(faces[-694:]);seed=bridge[body[bridge]]
eligible=np.zeros(len(keys),bool);eligible[inv[allowed]]=True
seen=set(int(x) for x in inv[seed]);queue=collections.deque(seen)
while queue:
    k=queue.popleft()
    for x in graph[k]:
        if eligible[x] and x not in seen:seen.add(x);queue.append(x)
ids=np.flatnonzero(allowed & np.isin(inv,list(seen)))
assert np.all(np.isin(seed,ids))
for i in ids:
    a=int(v['b'][i]&255);b=int((v['b'][i]>>8)&255);w=float(v['w'][i]);t=np.clip((field[i]+.060)/.060,0,1);t=t*t*(3-2*t)
    if a==16: w=w+(1-w)*t
    elif b==16: w=w*(1-t)
    else:
        # Preserve the larger original influence within the two-weight format.
        a=a if w>=.5 else b;b=16;w=1-t
        v['b'][i]=(int(v['b'][i])&0xffff0000)|a|(b<<8)
    v['w'][i]=w
print('body/bridge rows changed',len(ids),'connected positions',len(seen),'seed',len(seed),flush=True)
(here/'cgmodelsnet-neck-body-follow-trial.gm').write_bytes(data)
