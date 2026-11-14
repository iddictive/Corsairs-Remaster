import json, struct, pathlib
import numpy as np
from PIL import Image
p=pathlib.Path(__file__).with_name('putn.glb')
b=p.read_bytes(); size=struct.unpack_from('<I',b,12)[0]; j=json.loads(b[20:20+size]); start=20+size+8
assert len(b)==struct.unpack_from('<I',b,8)[0]
v=j['bufferViews'][1]; verts=np.frombuffer(b,dtype='<f4',count=1019802*3,offset=start+v['byteOffset']).reshape(-1,3)
canvas=Image.new('RGB',(1600,400),(235,235,235))
for n,(x,y,z) in enumerate(((0,1,2),(2,1,0),(0,2,1),(1,2,0))):
    a=verts[:,x]; c=verts[:,y]; dep=verts[:,z]
    px=np.clip(((a-a.min())/(a.max()-a.min())*360+20).astype(int),0,399)
    py=np.clip((380-(c-c.min())/(c.max()-c.min())*360).astype(int),0,399)
    idx=py*400+px; order=np.argsort(dep); depth=np.full(160000,np.nan); depth[idx[order]]=dep[order]; depth=depth.reshape(400,400)
    valid=np.isfinite(depth); fill=np.where(valid,depth,0); dx=np.gradient(fill,axis=1); dy=np.gradient(fill,axis=0)
    intensity=np.clip(0.65-dx*12+dy*12,0.15,1)*210; image=np.full((400,400,3),235,dtype=np.uint8)
    for k in range(3): image[:,:,k]=np.where(valid,intensity,235).astype('uint8')
    canvas.paste(Image.fromarray(image),(n*400,0))
canvas.save(pathlib.Path(__file__).with_name('geometry.png'))
print('min',verts.min(axis=0),'max',verts.max(axis=0),'finite',np.isfinite(verts).all())
