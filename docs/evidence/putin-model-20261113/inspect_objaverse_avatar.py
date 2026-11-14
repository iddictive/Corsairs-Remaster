"""Inspect the downloaded public GLB geometry and rasterize its head offline."""
import io
import json
import math
import struct
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
data=(here/'objaverse-lasquetispice.glb').read_bytes()
n=struct.unpack_from('<I',data,12)[0];meta=json.loads(data[20:20+n]);bin_offset=28+n
def read(i):
    a=meta['accessors'][i];v=meta['bufferViews'][a['bufferView']]
    dtypes={5123:'<u2',5125:'<u4',5126:'<f4'};dims={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}
    dtype=np.dtype(dtypes[a['componentType']]); width=dims[a['type']]
    offset=bin_offset+v.get('byteOffset',0)+a.get('byteOffset',0)
    stride=v.get('byteStride',width*dtype.itemsize)
    assert offset+(a['count']-1)*stride+width*dtype.itemsize<=len(data)
    return np.ndarray((a['count'],width),dtype=dtype,buffer=data,offset=offset,strides=(stride,dtype.itemsize)).copy()
prim=meta['meshes'][0]['primitives'][0];a=prim['attributes']
p=read(a['POSITION']).astype(float);n=read(a['NORMAL']).astype(float);uv=read(a['TEXCOORD_0']).astype(float)
faces=read(prim['indices']).reshape(-1,3);assert np.isfinite(p).all() and int(faces.max())<len(p)
tex=np.asarray(Image.open(here/'lasquetispice-texture-0.png').convert('RGB'));height,width=tex.shape[:2]
print('vertices',len(p),'triangles',len(faces),'texture',width,height,'skins',len(meta.get('skins',[])),'animations',len(meta.get('animations',[])))
for angle,label in [(0.,'front'),(math.pi,'back'),(-math.pi/2,'profile')]:
    c,s=math.cos(angle),math.sin(angle)
    x=(p[:,0]*c+p[:,2]*s)*1000+256;y=490-(p[:,1]-1.43)*1000;z=-p[:,0]*s+p[:,2]*c
    q=np.column_stack((x,y,z));nn=np.column_stack((n[:,0]*c+n[:,2]*s,n[:,1],-n[:,0]*s+n[:,2]*c))
    img=np.full((512,512,3),[18,22,30],dtype=np.uint8);depth=np.full((512,512),-np.inf)
    for f in faces:
        v=q[f];lo=np.maximum(np.floor(v[:,:2].min(0)).astype(int),0);hi=np.minimum(np.ceil(v[:,:2].max(0)).astype(int),511)
        if np.any(lo>hi):continue
        aa,bb,dd=v[:,:2];det=(bb[1]-dd[1])*(aa[0]-dd[0])+(dd[0]-bb[0])*(aa[1]-dd[1])
        if abs(det)<1e-8:continue
        yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];xx=xx+.5;yy=yy+.5
        w0=((bb[1]-dd[1])*(xx-dd[0])+(dd[0]-bb[0])*(yy-dd[1]))/det
        w1=((dd[1]-aa[1])*(xx-dd[0])+(aa[0]-dd[0])*(yy-dd[1]))/det;w2=1-w0-w1
        znow=w0*v[0,2]+w1*v[1,2]+w2*v[2,2];old=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
        mask=(w0>=0)&(w1>=0)&(w2>=0)&(znow>old)
        if not mask.any():continue
        old[mask]=znow[mask]
        u=w0*uv[f[0],0]+w1*uv[f[1],0]+w2*uv[f[2],0];vv=w0*uv[f[0],1]+w1*uv[f[1],1]+w2*uv[f[2],1]
        ix=np.clip((u*width).astype(int),0,width-1);iy=np.clip((vv*height).astype(int),0,height-1)
        normal=w0[...,None]*nn[f[0]]+w1[...,None]*nn[f[1]]+w2[...,None]*nn[f[2]]
        normal/=np.maximum(np.linalg.norm(normal,axis=2,keepdims=True),1e-8)
        light=.72+.28*np.maximum(0,normal@np.array([-.35,.25,.9028]))
        dst=img[lo[1]:hi[1]+1,lo[0]:hi[0]+1];dst[mask]=np.clip(tex[iy,ix]*light[...,None],0,255)[mask]
    Image.fromarray(img).save(here/f'lasquetispice-actual-{label}.png')
