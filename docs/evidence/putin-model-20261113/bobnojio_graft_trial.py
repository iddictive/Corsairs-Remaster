"""Experimental head graft on stock GM, bind pose only; not for delivery."""
import math
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=(here/'no-earring-prototype.gm').read_bytes()
h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri_at=obj+104*h[8]; buff=tri_at+6*h[9]
kind,size=struct.unpack_from('<2i',data,buff); assert kind==4 and size%44==0 and h[8]==1
start=buff+8; n=size//44
old=[]
for i in range(n):
    vertex=bytearray(data[start+44*i:start+44*(i+1)])
    u,t=struct.unpack_from('<2f',vertex,36)
    struct.pack_into('<2f',vertex,36,u*.5,(t+1)*.5)
    old.append(vertex)
dt=np.dtype({'names':['p','bone'],'formats':[('<f4',3),'<u4'],'offsets':[0,16],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=n,offset=start)
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
# Cut only head-influenced faces above the neck; transitional triangles stay.
cut=1.6174
head_bones=(v['bone'][faces]&255)==16
remove=(v['p'][faces,:,][...,1].min(axis=1)>cut)&np.all(head_bones,axis=1)
kept=faces[~remove]
stl=(here/'bobnojio-head-reduced-1.5.STL').read_bytes()
count=struct.unpack_from('<I',stl,80)[0]
stldt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
source=np.frombuffer(stl,dtype=stldt,count=count,offset=84)['vertices']
source=source[np.all(source[:,:,2]>=83.,axis=1)]
points,ids=np.unique(source.reshape(-1,3),axis=0,return_inverse=True)
indices=ids.reshape(-1,3)
pos=np.column_stack((.010+points[:,0]*.0025,1.385+points[:,2]*.0028,.06-points[:,1]*.0025))
norm=np.zeros_like(pos,dtype=float)
tri=pos[indices]; cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0])
for i in range(3):np.add.at(norm,indices[:,i],cross)
norm/=np.maximum(np.linalg.norm(norm,axis=1,keepdims=True),1e-10)
assert n+len(pos)<65536 and len(indices)>5000
angle=-.15; c,s=math.cos(angle),math.sin(angle); zoom=500/.41
sx=(pos[:,0]*c+pos[:,2]*s+.004)*zoom+256
sy=490-(pos[:,1]-1.465)*zoom
# Fixed front projection is diagnostic only: side/back UVs need proper unwrap.
u=.5+sx/2048; t=sy/2048
rows=[struct.pack('<3ffI3fI2f',*p,1.0,16,*normal,0xffffffff,uu,tt)
      for p,normal,uu,tt in zip(pos,norm,u,t)]
newfaces=np.vstack((kept,indices+n)).astype('<u2')
prefix=bytearray(data[:tri_at]); h[9]=len(newfaces); struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj)); fields[8]=len(newfaces); fields[10]=n+len(rows)
struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
out=prefix+newfaces.tobytes()+struct.pack('<2i',4,(n+len(rows))*44)+b''.join(old)+b''.join(rows)+data[start+size:]
(here/'bobnojio-graft-prototype.gm').write_bytes(out)
print('removed stock faces',int(remove.sum()),'added head faces',len(indices),'head vertices',len(rows),'total faces',len(newfaces))
