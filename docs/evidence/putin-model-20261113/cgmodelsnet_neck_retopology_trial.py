"""Split body cut edges and rebuild the body/head bridge with matched seams."""
import collections
import struct
from pathlib import Path
import numpy as np
from fit_skin_trial import SkinFit

here=Path(__file__).resolve().parent
data=(here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes();h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7];tri=obj+104*h[8];off=tri+6*h[9]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3).copy();kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
start=off+8;rows=[data[start+44*i:start+44*(i+1)] for i in range(size//44)];verts=[struct.unpack('<3ffI3fI2f',r) for r in rows]
pos=np.asarray([r[:3] for r in verts]);keys,inv=np.unique(np.rint(pos*1e6).astype(int),axis=0,return_inverse=True)
bodyfaces=faces[:-628];lookup=collections.defaultdict(list)
for fid,face in enumerate(bodyfaces):
    for j in range(3):
        a,b=int(face[j]),int(face[(j+1)%3]);lookup[tuple(sorted((int(inv[a]),int(inv[b]))))].append((fid,j))
fitter=SkinFit();replacement={};bodyloop=[];segments=4
for i in range(22):
    ba,bb=14993+i,14994+i
    edgekey=tuple(sorted((int(inv[ba]),int(inv[bb]))));matches=lookup[edgekey];assert len(matches)==1
    fid,j=matches[0];assert fid not in replacement
    face=bodyfaces[fid];a,b,c=[int(face[(j+k)%3]) for k in range(3)]
    same=inv[a]==inv[ba];assert same or inv[a]==inv[bb]
    va,vb=verts[a],verts[b];bridge_a,bridge_b=(ba,bb) if same else (bb,ba)
    newbody=[a];newbridge=[bridge_a]
    for k in range(1,segments):
        t=k/segments;location=(1-t)*np.asarray(va[:3])+t*np.asarray(vb[:3])
        weights={}
        for row,mul in ((va,1-t),(vb,t)):
            for bone,w in ((row[4]&255,row[3]),((row[4]>>8)&255,1-row[3])):weights[bone]=weights.get(bone,0)+w*mul
        top=sorted(weights,key=weights.get,reverse=True)[:2]
        if len(top)==1:top*=2
        total=weights[top[0]]+weights[top[1]];default=(tuple(top),weights[top[0]]/total if total else 1.)
        pair,w=fitter.fit(va,vb,t,location,default)
        normal=(1-t)*np.asarray(va[5:8])+t*np.asarray(vb[5:8]);normal/=max(np.linalg.norm(normal),1e-12)
        uv=(1-t)*np.asarray(va[9:11])+t*np.asarray(vb[9:11])
        row=struct.pack('<3ffI3fI2f',*location,w,pair[0]|pair[1]<<8,*normal,va[8],*uv)
        newbody.append(len(rows));rows.append(row);verts.append(struct.unpack('<3ffI3fI2f',row))
        uvb=(1-t)*np.asarray(verts[bridge_a][9:11])+t*np.asarray(verts[bridge_b][9:11]);bridge_row=bytearray(row);struct.pack_into('<2f',bridge_row,36,*uvb)
        newbridge.append(len(rows));rows.append(bytes(bridge_row));verts.append(struct.unpack('<3ffI3fI2f',bridge_row))
    newbody.append(b);newbridge.append(bridge_b)
    replacement[fid]=[(newbody[k],newbody[k+1],c) for k in range(segments)]
    if not same:newbridge=newbridge[::-1]
    bodyloop.extend(newbridge[:-1])
bodyloop.append(15015);assert len(bodyloop)==89
headloop=list(range(15016,15623));assert len(headloop)==607
pos=np.asarray([r[:3] for r in verts]);m,k=len(bodyloop)-1,len(headloop)-1
cost=np.full((m+1,k+1),np.inf);choice=np.zeros((m+1,k+1),dtype=np.uint8);cost[0,0]=0
for i in range(m+1):
    for j in range(k+1):
        if i<m:
            score=cost[i,j]+np.sum((pos[bodyloop[i+1]]-pos[headloop[j]])**2)
            if score<cost[i+1,j]:cost[i+1,j]=score;choice[i+1,j]=1
        if j<k:
            score=cost[i,j]+np.sum((pos[bodyloop[i]]-pos[headloop[j+1]])**2)
            if score<cost[i,j+1]:cost[i,j+1]=score;choice[i,j+1]=2
steps=[];i,j=m,k
while i or j:
    step=choice[i,j];assert step in (1,2);steps.append(step)
    if step==1:i-=1
    else:j-=1
bridge=[];i=j=0
for step in reversed(steps):
    if step==1:bridge.append((bodyloop[i],bodyloop[i+1],headloop[j]));i+=1
    else:bridge.append((bodyloop[i],headloop[j+1],headloop[j]));j+=1
outfaces=[]
for fid,face in enumerate(bodyfaces):outfaces.extend(replacement.get(fid,[tuple(face)]))
outfaces.extend(bridge);outfaces=np.asarray(outfaces,dtype='<u2');assert len(rows)<65536
prefix=bytearray(data[:tri]);h[9]=len(outfaces);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(outfaces);fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
(here/'cgmodelsnet-neck-retopology-trial.gm').write_bytes(prefix+outfaces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[start+size:])
print('split body faces',len(replacement),'new vertices',len(rows)-size//44,'bridge faces',len(bridge),'vertices',len(rows),'triangles',len(outfaces))
fitter.report()
