"""Parse and render actual CGModelsNet GLB geometry without a game or DCC."""
import json
import struct
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw

here=Path(__file__).resolve().parent;data=(here/'objaverse-cgmodelsnet.glb').read_bytes()
n=struct.unpack_from('<I',data,12)[0];j=json.loads(data[20:20+n]);base=28+n
def read(i):
    a=j['accessors'][i];v=j['bufferViews'][a['bufferView']];types={5125:'<u4',5126:'<f4'};dims={'SCALAR':1,'VEC3':3}
    dt=np.dtype(types[a['componentType']]);width=dims[a['type']];off=base+v.get('byteOffset',0)+a.get('byteOffset',0)
    stride=v.get('byteStride',width*dt.itemsize)
    assert off+(a['count']-1)*stride+width*dt.itemsize<=len(data)
    return np.ndarray((a['count'],width),dtype=dt,buffer=data,offset=off,strides=(stride,dt.itemsize)).copy()
points=[];normals=[];total=0
for m in j['meshes']:
    a=m['primitives'][0]['attributes'];p=read(a['POSITION']);normal=read(a['NORMAL']);idx=read(m['primitives'][0]['indices']).reshape(-1,3)
    assert np.isfinite(p).all() and int(idx.max())<len(p)
    points.extend((p,p[idx].mean(axis=1)));normals.extend((normal,normal[idx].mean(axis=1)));total+=len(idx)
p=np.concatenate(points).astype(float);normal=np.concatenate(normals).astype(float)
length=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-10);normal/=length
lo=p.min(axis=0);hi=p.max(axis=0);center=(lo+hi)/2;scale=455/(hi[2]-lo[2])
canvas=Image.new('RGB',(2048,512),(28,32,39))
for i,(axis,sgn,label) in enumerate([(0,1,'front/back A'),(0,-1,'front/back B'),(1,1,'side A'),(1,-1,'side B')]):
    side=1-axis;x=np.clip((256+(p[:,axis]-center[axis])*scale).astype(int),0,511)
    y=np.clip((490-(p[:,2]-lo[2])*scale).astype(int),0,511);d=p[:,side]*sgn
    order=np.argsort(d);index=y*512+x;selected=np.full(512*512,-1,dtype=int);selected[index[order]]=order;visible=selected>=0
    light=np.array([.28,-.5,.82]);light/=np.linalg.norm(light)
    value=.35+.65*np.abs(normal[selected[visible]]@light);img=np.full((512*512,3),[28,32,39],dtype=np.uint8)
    img[visible]=np.clip(value[:,None]*[210,213,218],0,255).astype(np.uint8)
    out=Image.fromarray(img.reshape(512,512,3));ImageDraw.Draw(out).text((12,12),label,fill=(255,255,255));canvas.paste(out,(512*i,0))
canvas.save(here/'cgmodelsnet-actual-geometry.png')
print('vertices',sum(m['primitives'][0]['attributes']['POSITION'] is not None and j['accessors'][m['primitives'][0]['attributes']['POSITION']]['count'] for m in j['meshes']),'triangles',total,'skins',len(j.get('skins',[])))
