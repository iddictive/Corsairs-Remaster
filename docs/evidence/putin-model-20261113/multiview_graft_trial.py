"""Experimental multi-view cylindrical texture bake and GM head graft."""
import math
import os
import struct
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
data=(here/'no-earring-prototype.gm').read_bytes()
h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri_at=obj+104*h[8]; buff=tri_at+6*h[9]
kind,size=struct.unpack_from('<2i',data,buff); assert kind==4 and size%44==0 and h[8]==1
start=buff+8; n=size//44
rows=[]
for i in range(n):
    vertex=bytearray(data[start+44*i:start+44*(i+1)])
    u,t=struct.unpack_from('<2f',vertex,36); struct.pack_into('<2f',vertex,36,u*.5,(t+1)*.5)
    rows.append(vertex)
v=np.frombuffer(data,dtype=np.dtype({'names':['p'],'formats':[('<f4',3)],'itemsize':44}),count=n,offset=start)
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
centers=v['p'][faces].mean(axis=1)
remove=(centers[:,1]>1.6174)&(abs(centers[:,0]-.01)<.14)&(centers[:,2]>-.12)&(centers[:,2]<.19)
kept=faces[~remove]
if os.environ.get('PUTIN_TRIAL_BODY_CLIP')=='1':
    from clip_body_trial import clip_body
    rows,kept=clip_body(rows,faces,v['p'],float(os.environ.get('PUTIN_TRIAL_BODY_PLANE','1.625')))
    n=len(rows)
raw=(here/'bobnojio-head-reduced-1.5.STL').read_bytes(); count=struct.unpack_from('<I',raw,80)[0]
stldt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
source=np.frombuffer(raw,dtype=stldt,count=count,offset=84)['vertices']
source=source[np.all(source[:,:,2]>=83.,axis=1)]
points,ids=np.unique(source.reshape(-1,3),axis=0,return_inverse=True)
indices=ids.reshape(-1,3)
pos=np.column_stack((.010+points[:,0]*.0025,1.385+points[:,2]*.0028,.06-points[:,1]*.0025))
normal=np.zeros_like(pos,dtype=float); tri=pos[indices]
cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0])
for j in range(3):np.add.at(normal,indices[:,j],cross)
normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-10)
u=(np.arctan2(pos[:,0]-.010,pos[:,2]-.06)/(2*math.pi)+.5)%1
t=np.clip((1.813-pos[:,1])/(1.813-1.6174),0,1)
# Duplicate vertices on the cylindrical seam to preserve interpolation.
outfaces=[]; remap={}; texcoords=[]; packed=[]
for face in indices:
    us=u[face].copy()
    if us.max()-us.min()>.5: us[us<.5]+=1
    face_out=[]
    for j,source_id in enumerate(face):
        source_id=int(source_id); shifted=int(us[j]>=1)
        key=(source_id,shifted)
        if key not in remap:
            remap[key]=len(packed); texcoords.append((us[j],t[source_id]))
            p=pos[source_id]; nn=normal[source_id]
            packed.append(struct.pack('<3ffI3fI2f',*p,1.,16,*nn,0xffffffff,us[j],.5+.5*t[source_id]))
        face_out.append(remap[key])
    outfaces.append(face_out)
outfaces=np.asarray(outfaces,dtype=np.int32); texcoords=np.asarray(texcoords)
assert n+len(packed)<65536
coords=texcoords[outfaces].copy(); coords[:,:,0]*=2048; coords[:,:,1]*=1024
images=[]
for name,angle in [('photo-feathered-target.png',-.15),('photo-face-profile.png',1.),('photo-face-other-profile.png',-1.),('photo-face-back.png',math.pi)]:
    images.append((np.asarray(Image.open(here/name).convert('RGB'),dtype=float),angle))
tile=np.zeros((1024,2048,3),dtype=np.uint8); covered=np.zeros((1024,2048),dtype=bool)
tri_pos=pos[indices]
for fi,xyz in enumerate(tri_pos):
    q=coords[fi]; lo=np.floor(q.min(0)).astype(int); hi=np.ceil(q.max(0)).astype(int)
    lo[1]=max(0,lo[1]);hi[1]=min(1023,hi[1])
    if hi[0]<lo[0] or hi[1]<lo[1] or hi[0]-lo[0]>1024:continue
    a,b,c=q; det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
    if abs(det)<1e-9:continue
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];xx=xx+.5;yy=yy+.5
    w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/det
    w1=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/det;w2=1-w0-w1
    valid=(w0>=0)&(w1>=0)&(w2>=0)
    if not valid.any():continue
    pts=w0[...,None]*xyz[0]+w1[...,None]*xyz[1]+w2[...,None]*xyz[2]
    theta=np.arctan2(pts[:,:,0]-.010,pts[:,:,2]-.06)
    color=np.zeros((*xx.shape,3)); weight=np.zeros(xx.shape)
    for img,angle in images:
        c,s=math.cos(angle),math.sin(angle)
        sx=(pts[:,:,0]*c+pts[:,:,2]*s+.004)*(500/.41)+256
        sy=490-(pts[:,:,1]-1.465)*(500/.41)
        ix=np.clip(sx.astype(int),0,511); iy=np.clip(sy.astype(int),0,511)
        facing=np.maximum(0,np.cos(theta+angle))**8
        good=(sx>=0)&(sx<512)&(sy>=0)&(sy<512)
        w=facing*good; color+=img[iy,ix]*w[...,None]; weight+=w
    color/=np.maximum(weight[...,None],1e-8)
    # Approximate inverse of the stock portrait's baked lighting.
    color=np.clip(color/.90,0,255).astype('uint8')
    xo=(xx[valid].astype(int)%2048); yo=yy[valid].astype(int)
    tile[yo,xo]=color[valid]; covered[yo,xo]=True
# Extend colored texels outside islands so filtering does not pull black seams.
for _ in range(12):
    old=covered.copy()
    for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        hit=np.roll(old,(dy,dx),(0,1))&~covered
        if dy<0:hit[-1,:]=False
        if dy>0:hit[0,:]=False
        tile[hit]=np.roll(tile,(dy,dx),(0,1))[hit];covered|=hit
atlas=np.asarray(Image.open(here/'photo-face-atlas.png').convert('RGB')).copy()
atlas[1024:2048,:,:]=tile
Image.fromarray(atlas).save(here/'multiview-graft-atlas.png')
newfaces=np.vstack((kept,outfaces+n)).astype('<u2')
prefix=bytearray(data[:tri_at]);h[9]=len(newfaces);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(newfaces);fields[10]=n+len(packed)
struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
result=prefix+newfaces.tobytes()+struct.pack('<2i',4,(n+len(packed))*44)+b''.join(rows)+b''.join(packed)+data[start+size:]
(here/'multiview-graft-prototype.gm').write_bytes(result)
print('removed faces',int(remove.sum()),'added',len(outfaces),'vertices',len(packed),'atlas coverage',int(covered.sum()))
