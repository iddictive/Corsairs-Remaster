"""Offline GM trial: isolate front face UVs while preserving skinned geometry."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

here=Path(__file__).resolve().parent
root=here.parents[2]
gm_path=Path(sys.argv[2]) if len(sys.argv)>2 else root/'src/assets/characters/putin-phonk/original.gm'
original=gm_path.read_bytes()
h=list(struct.unpack_from('<11i7f',original))
obj_at=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
assert h[8]==1 and h[10]==1
tri_at=obj_at+104; buff=tri_at+6*h[9]
typ,size=struct.unpack_from('<2i',original,buff); assert typ==4 and size%44==0
start=buff+8; count=size//44
raw=[bytearray(original[start+i*44:start+(i+1)*44]) for i in range(count)]
faces=np.frombuffer(original,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3).copy()
rows=[struct.unpack('<3ffI3fI2f',v) for v in raw]
uv=np.array([v[9:11] for v in rows]); p=np.array([v[:3] for v in rows])
assert np.all((uv[:,0]>=0)&(uv[:,0]<=1)&(uv[:,1]>=-1)&(uv[:,1]<=0))
angle=-.15; c,s=math.cos(angle),math.sin(angle); zoom=500/.41
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*zoom+256,490-(p[:,1]-1.465)*zoom,-p[:,0]*s+p[:,2]*c))
for i,v in enumerate(raw):struct.pack_into('<2f',v,36,uv[i,0]*.5,(uv[i,1]+1)*.5)
duplicates={}; selected=0; chosen=[]
for i,f in enumerate(faces):
    tex=uv[f]; scr=screen[f]; center=scr.mean(0)
    # Front-facing skin only; eye balls use a separate atlas island.
    cross=np.cross(p[f[1]]-p[f[0]],p[f[2]]-p[f[0]])
    if not (np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))):continue
    if not (185<center[0]<325 and 145<center[1]<325 and center[2]>.045):continue
    if np.linalg.norm(cross)<1e-10:continue
    # Side-facing skin is not visible to a frontal reference and must retain
    # its own original atlas coordinates for correct profile rendering.
    normals=np.array([rows[int(j)][5:8] for j in f])
    if np.any(normals@np.array([-s,0,c])<.55):continue
    if np.any(scr[:,0]<10) or np.any(scr[:,0]>500) or np.any(scr[:,1]<10) or np.any(scr[:,1]>500):continue
    for j,index in enumerate(f):
        index=int(index)
        if index not in duplicates:
            v=bytearray(raw[index]); x,y=screen[index,:2]
            struct.pack_into('<2f',v,36,.5+x/2048,y/2048)
            duplicates[index]=len(raw); raw.append(v)
        faces[i,j]=duplicates[index]
    selected+=1
    chosen.append(i)
assert 30<selected<1000 and len(raw)<65536,(selected,len(raw))
obj=list(struct.unpack_from('<3i4f6i8i4ii',original,obj_at)); obj[10]=len(raw)
head=bytearray(original[:tri_at]); struct.pack_into('<3i4f6i8i4ii',head,obj_at,*obj)
result=head+faces.astype('<u2').tobytes()+struct.pack('<2i',4,len(raw)*44)+b''.join(raw)+original[start+size:]
(here/'separate-face-prototype.gm').write_bytes(result)
atlas=Image.new('RGB',(2048,2048))
base=Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)
target_path=here/(sys.argv[1] if len(sys.argv)>1 else 'projection-target.png')
target=np.asarray(Image.open(target_path).convert('RGB').resize((512,512),Image.Resampling.LANCZOS),dtype=float)
baseline_path=here/(sys.argv[3] if len(sys.argv)>3 else ('landmark-morph-baseline.png' if len(sys.argv)>2 else 'projection-identity-input.png'))
baseline=np.asarray(Image.open(baseline_path).convert('RGB'),dtype=float)
tex=np.asarray(base,dtype=float); depth=np.full((512,512),-np.inf); pixels=np.zeros((512,512,3)); lit=np.ones((512,512)); visible=np.zeros((512,512),dtype=np.uint8)
norm=np.array([v[5:8] for v in rows]); norm=np.column_stack((norm[:,0]*c+norm[:,2]*s,norm[:,1],-norm[:,0]*s+norm[:,2]*c))
origfaces=np.frombuffer(original,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
chosen=set(chosen)
for face_id,f in enumerate(origfaces):
    q=screen[f]; x0=max(0,int(np.floor(q[:,0].min()))); x1=min(511,int(np.ceil(q[:,0].max())))
    y0=max(0,int(np.floor(q[:,1].min()))); y1=min(511,int(np.ceil(q[:,1].max())))
    if x1<x0 or y1<y0:continue
    a,b,d=q[:,:2]; det=(b[1]-d[1])*(a[0]-d[0])+(d[0]-b[0])*(a[1]-d[1])
    if abs(det)<1e-8:continue
    yy,xx=np.mgrid[y0:y1+1,x0:x1+1]; xx=xx+.5; yy=yy+.5
    w0=((b[1]-d[1])*(xx-d[0])+(d[0]-b[0])*(yy-d[1]))/det
    w1=((d[1]-a[1])*(xx-d[0])+(a[0]-d[0])*(yy-d[1]))/det; w2=1-w0-w1
    z=w0*q[0,2]+w1*q[1,2]+w2*q[2,2]; area=depth[y0:y1+1,x0:x1+1]; ok=(w0>=0)&(w1>=0)&(w2>=0)&(z>area)
    area[ok]=z[ok]; vis=visible[y0:y1+1,x0:x1+1]; vis[ok]=255 if face_id in chosen else 0
    if face_id not in chosen:continue
    u=w0*uv[f[0],0]+w1*uv[f[1],0]+w2*uv[f[2],0]; t=w0*uv[f[0],1]+w1*uv[f[1],1]+w2*uv[f[2],1]
    ix=np.clip(((u%1)*1024).astype(int),0,1023); iy=np.clip(((t%1)*1024).astype(int),0,1023)
    patch=pixels[y0:y1+1,x0:x1+1]; patch[ok]=tex[iy,ix][ok]
    n=w0[...,None]*norm[f[0]]+w1[...,None]*norm[f[1]]+w2[...,None]*norm[f[2]]
    n/=np.maximum(np.linalg.norm(n,axis=2,keepdims=True),1e-8)
    lighting=.72+.28*np.maximum(0,n@np.array([-.35,.25,.9028]))
    lp=lit[y0:y1+1,x0:x1+1]; lp[ok]=lighting[ok]
blur=np.asarray(Image.fromarray(visible).filter(ImageFilter.GaussianBlur(6)),dtype=float)/255
weight=(blur*(visible>0))[...,None]
face=np.clip(pixels+(target-baseline)/lit[...,None]*weight,0,255).astype('uint8')
# Back-facing portions of selected triangles are absent in the frontal depth
# buffer but become visible in profile. Extend neighboring diffuse texels
# rather than leaving the new UV island black at occlusions and its border.
known=visible>0
for _ in range(80):
    if known.all():break
    old=known.copy()
    for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        hit=np.roll(old,(dy,dx),(0,1))&~known
        if dy<0:hit[-1,:]=False
        if dy>0:hit[0,:]=False
        if dx<0:hit[:,-1]=False
        if dx>0:hit[:,0]=False
        shifted=np.roll(face,(dy,dx),(0,1)); face[hit]=shifted[hit]; known|=hit
atlas.paste(base,(0,0)); atlas.paste(Image.fromarray(face),(1024,0)); atlas.save(here/'separate-face-atlas.png')
print('triangles',selected,'original vertices',count,'duplicated',len(duplicates),'total',len(raw))
