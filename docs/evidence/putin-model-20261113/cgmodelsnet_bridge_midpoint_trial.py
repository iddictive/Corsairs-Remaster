"""Subdivide only crossing edges of the neck bridge with fitted skin weights."""
import json
import struct
from pathlib import Path
import numpy as np
from fit_skin_trial import SkinFit

here=Path(__file__).resolve().parent
data=(here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes();h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7];tri=obj+104*h[8];off=tri+6*h[9]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3).copy();kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
start=off+8;rows=[data[start+i*44:start+(i+1)*44] for i in range(size//44)]
unpacked=[struct.unpack('<3ffI3fI2f',r) for r in rows];fitter=SkinFit();cache={};newfaces=[]
def midpoint(a,b):
    key=tuple(sorted((int(a),int(b))))
    if key in cache:return cache[key]
    va,vb=unpacked[a],unpacked[b];pos=(np.asarray(va[:3])+np.asarray(vb[:3]))*.5
    influences={}
    for r in (va,vb):
        for bone,weight in ((r[4]&255,r[3]),((r[4]>>8)&255,1-r[3])):
            influences[bone]=influences.get(bone,0)+weight*.5
    top=sorted(influences,key=influences.get,reverse=True)[:2]
    if len(top)==1:top*=2
    total=influences[top[0]]+influences[top[1]]
    default=(tuple(top),influences[top[0]]/total if total else 1.)
    pair,w=fitter.fit(va,vb,.5,pos,default)
    n=(np.asarray(va[5:8])+np.asarray(vb[5:8]))*.5;n/=max(np.linalg.norm(n),1e-12)
    uv=(np.asarray(va[9:11])+np.asarray(vb[9:11]))*.5
    row=struct.pack('<3ffI3fI2f',*pos,w,pair[0]|(pair[1]<<8),*n,va[8],*uv)
    cache[key]=len(rows);rows.append(row);unpacked.append(struct.unpack('<3ffI3fI2f',row))
    return cache[key]
for face in faces[-628:]:
    side=np.asarray(face)<15016
    assert 0<side.sum()<3
    unique=side if side.sum()==1 else ~side
    j=int(np.flatnonzero(unique)[0]);v0,v1,v2=[int(face[(j+k)%3]) for k in range(3)]
    m1=midpoint(v0,v1);m2=midpoint(v0,v2)
    newfaces.extend([(v0,m1,m2),(m1,v1,v2),(m1,v2,m2)])
assert len(rows)<65536 and len(newfaces)==628*3
outfaces=np.vstack((faces[:-628],np.asarray(newfaces,dtype='<u2'))).astype('<u2')
prefix=bytearray(data[:tri]);h[9]=len(outfaces);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(outfaces);fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
out=prefix+outfaces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[start+size:]
(here/'cgmodelsnet-bridge-midpoint-trial.gm').write_bytes(out)
(here/'cgmodelsnet-bridge-midpoint-map.json').write_text(json.dumps([{'edge':list(k),'midpoint':v} for k,v in cache.items()],indent=2)+'\n')
print('added vertices',len(cache),'bridge triangles',len(newfaces),'all vertices',len(rows),'all faces',len(outfaces))
fitter.report()
