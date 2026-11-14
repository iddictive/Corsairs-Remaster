"""Locally subdivide rigid-head triangles, with geometric seam matching."""
import math
import struct
import sys
from collections import defaultdict
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=(here/'profile-geometry-prototype.gm').read_bytes()
h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
assert h[8]==1 and h[10]==1
tri=obj+104; end=tri+6*h[9]
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri).reshape(-1,3).copy()
kind,size=struct.unpack_from('<2i',data,end); assert kind==4 and size%44==0
start=end+8
raw=[data[start+44*i:start+44*(i+1)] for i in range(size//44)]
rows=[struct.unpack('<3ffI3fI2f',v) for v in raw]
p=np.asarray([v[:3] for v in rows]); bones=np.asarray([v[4] for v in rows]); weights=np.asarray([v[3] for v in rows])
def position(i):return tuple(np.round(p[i],6))
def edge(a,b):return tuple(sorted((position(a),position(b))))
incidents=defaultdict(list)
for face in faces:
    for a,b,o in ((face[0],face[1],face[2]),(face[1],face[2],face[0]),(face[2],face[0],face[1])):
        incidents[edge(a,b)].append((int(a),int(b),int(o)))
selected=set()
for f in faces:
    center=p[f].mean(axis=0)
    rigid=np.all((bones[f]&255)==16) and np.all(weights[f]>.9999)
    if rigid and abs(center[0]-.008)<.032 and 1.665<center[1]<1.71 and center[2]>.115:
        selected.update(edge(a,b) for a,b in ((f[0],f[1]),(f[1],f[2]),(f[2],f[0])))
# Every split side of a coincident geometric edge must be rigidly head-bound.
selected={e for e in selected if all((bones[[a,b]]&255==16).all() and (weights[[a,b]]>.9999).all() for a,b,_ in incidents[e])}
mid_geometry={}
for e in selected:
    endpoints=np.asarray(e)
    opposite={position(o) for _,_,o in incidents[e]}
    if len(opposite)==2:
        pos=3/8*(endpoints[0]+endpoints[1])+1/8*sum(map(np.asarray,opposite))
    else:
        pos=endpoints.mean(axis=0)
    mid_geometry[e]=pos
newraw=list(raw); mid_indices={}; newfaces=[]
def midpoint(a,b):
    key=tuple(sorted((int(a),int(b))))
    if key in mid_indices:return mid_indices[key]
    a,b=key; ra,rb=rows[a],rows[b]
    normal=np.asarray(ra[5:8])+np.asarray(rb[5:8]); normal/=max(np.linalg.norm(normal),1e-8)
    u=(ra[9]+rb[9])/2; v=(ra[10]+rb[10])/2
    row=struct.pack('<3ffI3fI2f',*mid_geometry[edge(a,b)],ra[3],ra[4],*normal,ra[8],u,v)
    mid_indices[key]=len(newraw); newraw.append(row)
    return mid_indices[key]
for face in faces:
    a,b,c=map(int,face); flags=[edge(a,b) in selected,edge(b,c) in selected,edge(c,a) in selected]
    count=sum(flags)
    if count==0:newfaces.append((a,b,c)); continue
    if count==1:
        while not flags[0]:a,b,c=b,c,a;flags=flags[1:]+flags[:1]
        ab=midpoint(a,b);newfaces.extend(((a,ab,c),(ab,b,c)))
    elif count==2:
        while not (flags[0] and flags[1]):a,b,c=b,c,a;flags=flags[1:]+flags[:1]
        ab=midpoint(a,b);bc=midpoint(b,c)
        newfaces.extend(((b,bc,ab),(a,ab,c),(ab,bc,c)))
    else:
        ab=midpoint(a,b);bc=midpoint(b,c);ca=midpoint(c,a)
        newfaces.extend(((a,ab,ca),(ab,b,bc),(ca,bc,c),(ab,bc,ca)))
assert 0<len(selected)<1000 and len(newraw)<65536
assert len(newfaces)<65536 and np.isfinite(np.asarray(list(mid_geometry.values()))).all()
prefix=bytearray(data[:tri]); h[9]=len(newfaces); struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj)); fields[8]=len(newfaces); fields[10]=len(newraw)
struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
out=prefix+np.asarray(newfaces,dtype='<u2').tobytes()+struct.pack('<2i',4,len(newraw)*44)+b''.join(newraw)+data[start+size:]
(here/'nose-subdivision-prototype.gm').write_bytes(out)
print('geometric split edges',len(selected),'added vertices',len(newraw)-len(raw),'added triangles',len(newfaces)-len(faces))
print('linear midpoints for non-two-sided geometric edges',sum(len({position(o) for _,_,o in incidents[e]})!=2 for e in selected))
renderer=here/'render_officer.py'
for angle,output in [(-.15,'nose-subdivision-front.png'),(1,'nose-subdivision-profile.png'),(-1,'nose-subdivision-other-profile.png'),(-math.pi/2,'nose-subdivision-true-profile.png')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(output))
    sys.argv=[str(renderer),'officer_5',str(here/'identity-geometry-face-atlas.png'),str(angle),str(here/'nose-subdivision-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
