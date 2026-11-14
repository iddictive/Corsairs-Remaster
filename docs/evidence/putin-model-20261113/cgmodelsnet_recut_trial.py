"""Clip a higher head contour and rebuild the bridge; diagnostic GM only."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=(here/'cgmodelsnet-stockuv-retopology.gm').read_bytes();h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7];tri=obj+104*h[8]
faces=np.frombuffer(data,'<u2',3*h[9],tri).reshape(-1,3).copy();off=tri+6*h[9]
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
start=off+8;rows=[data[start+44*i:start+44*(i+1)] for i in range(size//44)]
verts=[struct.unpack('<3ffI3fI2f',r) for r in rows];p=np.asarray([x[:3] for x in verts])
head=np.all((faces>=2106)&(faces<14993),axis=1)
assert head.sum()==25000 and np.all(~head[-694:])
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
q=np.clip((.025-p[:,2])/.04,0,1);q=q*q*(3-2*q)
f=field-.004-.004*q
newhead=[];cut_edges=[];cache={}
def cross(a,b):
    key=tuple(sorted((int(a),int(b))))
    if key in cache:return cache[key]
    a,b=key;t=f[a]/(f[a]-f[b]);assert 0<=t<=1
    x,y=verts[a],verts[b];assert x[4]==y[4] and abs(x[3]-y[3])<1e-6
    pos=(1-t)*np.array(x[:3])+t*np.array(y[:3]);normal=(1-t)*np.array(x[5:8])+t*np.array(y[5:8]);normal/=np.linalg.norm(normal)
    uv=(1-t)*np.array(x[9:11])+t*np.array(y[9:11])
    row=struct.pack('<3ffI3fI2f',*pos,x[3],x[4],*normal,x[8],*uv)
    cache[key]=len(rows);rows.append(row);verts.append(struct.unpack('<3ffI3fI2f',row));return cache[key]
for face in faces[head]:
    poly=[];crossings=[];prev=int(face[-1]);pin=f[prev]>=0
    for val in face:
        cur=int(val);cin=f[cur]>=0
        if pin!=cin:
            v=cross(prev,cur);poly.append(v);crossings.append(v)
        if cin:poly.append(cur)
        prev,pin=cur,cin
    if len(crossings)==2:cut_edges.append(crossings)
    for j in range(1,len(poly)-1):newhead.append((poly[0],poly[j],poly[j+1]))
pos=np.asarray([v[:3] for v in verts]);keys,inv=np.unique(np.rint(pos*1e6).astype('int64'),axis=0,return_inverse=True)
def ordered(edges):
    adj={};represent={}
    for a,b in edges:
        ka,kb=int(inv[a]),int(inv[b]);represent.setdefault(ka,int(a));represent.setdefault(kb,int(b))
        if ka==kb:continue
        adj.setdefault(ka,set()).add(kb);adj.setdefault(kb,set()).add(ka)
    assert all(len(v)==2 for v in adj.values()),{n:sum(len(v)==n for v in adj.values()) for n in range(5)}
    first=next(iter(adj));order=[first];prev=-1
    while True:
        nxt=next(x for x in adj[order[-1]] if x!=prev)
        if nxt==first:break
        assert nxt not in order
        prev=order[-1];order.append(nxt)
    assert len(order)==len(adj)
    return np.array([represent[x] for x in order],dtype=int)
headloop=ordered(cut_edges)
bridge=faces[-694:];bodymask=lambda x:not (15016<=x<15623)
bedges=[]
for face in bridge:
    b=[int(x) for x in face if bodymask(int(x))]
    if len(b)==2:bedges.append(b)
bodyloop=ordered(bedges)
def area(ids):
    x,z=pos[ids,0],pos[ids,2];return np.sum(x*np.roll(z,-1)-z*np.roll(x,-1))
if area(bodyloop)*area(headloop)<0:headloop=headloop[::-1]
dist=np.linalg.norm(pos[bodyloop,None,:]-pos[headloop][None,:,:],axis=2)
i,j=np.unravel_index(np.argmin(dist),dist.shape);bodyloop=np.roll(bodyloop,-i);headloop=np.roll(headloop,-j)
bodyloop=np.r_[bodyloop,bodyloop[0]];headloop=np.r_[headloop,headloop[0]];m,k=len(bodyloop)-1,len(headloop)-1
cost=np.full((m+1,k+1),np.inf);choice=np.zeros((m+1,k+1),np.uint8);cost[0,0]=0
for i in range(m+1):
    for j in range(k+1):
        if i<m:
            s=cost[i,j]+np.sum((pos[bodyloop[i+1]]-pos[headloop[j]])**2)
            if s<cost[i+1,j]:cost[i+1,j]=s;choice[i+1,j]=1
        if j<k:
            s=cost[i,j]+np.sum((pos[bodyloop[i]]-pos[headloop[j+1]])**2)
            if s<cost[i,j+1]:cost[i,j+1]=s;choice[i,j+1]=2
steps=[];i,j=m,k
while i or j:
    s=choice[i,j];assert s in (1,2);steps.append(s);i-=s==1;j-=s==2
steps.reverse()
# Project each new head boundary point onto the old head boundary's UV track.
oldids=np.arange(15016,15623);oldp=pos[oldids];olduv=np.array([verts[i][9:11] for i in oldids])
newhc=[]
for idx in headloop:
    nearest=np.argmin(np.linalg.norm(oldp-pos[idx],axis=1));row=bytearray(rows[idx]);struct.pack_into('<2f',row,36,*olduv[nearest]);newhc.append(len(rows));rows.append(bytes(row))
newbridge=[];i=j=0
for s in steps:
    if s==1:newbridge.append((bodyloop[i],bodyloop[i+1],newhc[j]));i+=1
    else:newbridge.append((bodyloop[i],newhc[j+1],newhc[j]));j+=1
# Match the existing bridge's oriented head boundary; the stock-derived body
# boundary retains its prior winding mismatch and is not silently corrected.
newbridge=[(a,c,b) for a,b,c in newbridge]
seamcopies={};fixed=0
for face_index,face in enumerate(newbridge):
    us=[struct.unpack_from('<f',rows[int(idx)],36)[0] for idx in face]
    if max(us)-min(us)<=.25:continue
    low=sum(u<.75 for u in us)>=2
    for n,(idx,u) in enumerate(zip(face,us)):
        if (low and u>.75) or (not low and u<.75):
            key=(int(idx),low)
            if key not in seamcopies:
                row=bytearray(rows[int(idx)]);struct.pack_into('<f',row,36,.5 if low else .999999)
                seamcopies[key]=len(rows);rows.append(bytes(row))
            face=list(face);face[n]=seamcopies[key]
    newbridge[face_index]=face
    fixed+=1
outfaces=np.concatenate((faces[:-694][~head[:-694]],np.asarray(newhead),np.asarray(newbridge))).astype('<u2')
assert len(rows)<65536
prefix=bytearray(data[:tri]);h[9]=len(outfaces);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(outfaces);fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
(here/'cgmodelsnet-recut-trial.gm').write_bytes(prefix+outfaces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[start+size:])
print('cut vertices',len(cache),'head faces',len(newhead),'body loop',m,'head loop',k,'bridge',len(newbridge),'vertices',len(rows),flush=True)
