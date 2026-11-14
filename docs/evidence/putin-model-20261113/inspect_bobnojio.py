"""Read the freely downloaded STL and make non-game geometry projections."""
import hashlib
import struct
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

here=Path(__file__).resolve().parent
path=here/'bobnojio-vvp.STL'
raw=path.read_bytes()
count=struct.unpack_from('<I',raw,80)[0]
assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
faces=np.frombuffer(raw,dtype=dt,count=count,offset=84)
verts=faces['vertices'].astype(float)
assert np.isfinite(verts).all()
norm=np.cross(verts[:,1]-verts[:,0],verts[:,2]-verts[:,0])
norm/=np.maximum(np.linalg.norm(norm,axis=1,keepdims=True),1e-12)
points=np.concatenate([verts[:,i] for i in range(3)]+[verts.mean(axis=1)])
normals=np.concatenate([norm]*4)
center=(verts.reshape(-1,3).min(0)+verts.reshape(-1,3).max(0))/2
canvas=Image.new('RGB',(2048,512),(28,32,39))
for i,(horizontal,depth,label) in enumerate(((0,1,'+Y'),(0,-1,'-Y'),(1,1,'+X'),(1,-1,'-X'))):
    side=1-horizontal
    a=(points[:,horizontal]-center[horizontal])*(1 if i<2 else -1)
    d=points[:,side]*depth
    z=points[:,2]
    scale=455/(z.max()-z.min())
    x=np.clip((256+a*scale).astype(int),0,511)
    y=np.clip((490-(z-z.min())*scale).astype(int),0,511)
    index=y*512+x
    order=np.argsort(d)
    selected=np.full(512*512,-1,dtype='int64')
    selected[index[order]]=order
    ok=selected>=0
    light=np.array([.28,-.5,.82]); light/=np.linalg.norm(light)
    intensity=.35+.65*np.abs(normals[selected[ok]]@light)
    img=np.zeros((512*512,3),dtype='uint8'); img[:]=[28,32,39]
    img[ok]=np.clip(intensity[:,None]*np.array([210,213,218]),0,255).astype('uint8')
    image=Image.fromarray(img.reshape(512,512,3))
    ImageDraw.Draw(image).text((12,12),label,fill=(255,255,255))
    canvas.paste(image,(512*i,0))
canvas.save(here/'bobnojio-geometry-projections.png')
print('triangles',count,'bounds',verts.reshape(-1,3).min(0),verts.reshape(-1,3).max(0),'sha256',hashlib.sha256(raw).hexdigest())
