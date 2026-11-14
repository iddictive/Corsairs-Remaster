"""Locate exposed rear-collar pixels on posed and bind-space head triangles."""
import ast
import math
import struct
from pathlib import Path
import numpy as np
from PIL import Image
here=Path(__file__).resolve().parent
fn=next(n for n in ast.parse((here/'cgmodelsnet_face_reprojection.py').read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='raster')
scope={'np':np};exec(compile(ast.Module(body=[fn],type_ignores=[]),'raster','exec'),scope);raster=scope['raster']
def read(name):
    data=(here/name).read_bytes();h=struct.unpack_from('<11i7f',data)
    t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);o=t+6*h[9];kind,size=struct.unpack_from('<2i',data,o);assert kind==4
    dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});return faces,np.frombuffer(data,dt,size//44,o+8)
faces,posed=read('animated-cgmodelsnet-neck-retopology-fit-8423.gm');other,bind=read('cgmodelsnet-neck-retopology-fit.gm');assert np.array_equal(faces,other)
p=posed['p'].astype(float);ref=bind['p'].astype(float);angle=-.15;c,s=math.cos(angle),math.sin(angle);scale=500/.41
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*scale+256,490-(p[:,1]-1.465)*scale,-p[:,0]*s+p[:,2]*c))
depth=np.full((512,512),-np.inf); xyz=np.zeros((512,512,3));owner=np.full((512,512),-1)
for face in faces:
    q=screen[face]
    if q[:,0].max()<105 or q[:,0].min()>180 or q[:,1].max()<250 or q[:,1].min()>305:continue
    result=raster(q[:,:2],512,512)
    if result is None:continue
    x,y,w,inside=result;z=w@q[:,2];valid=inside&(z>depth[y,x]);depth[y,x]=np.where(valid,z,depth[y,x])
    xyz[y,x]=np.where(valid[...,None],w@ref[face],xyz[y,x]);owner[y,x]=np.where(valid,int(np.min(face)),owner[y,x])
rgb=np.asarray(Image.open(here/'animated-cgmodelsnet-neck-retopology-fit-8423.png').convert('RGB'),dtype=int)
y,x=np.mgrid[0:512,0:512]
chosen=(x>=110)&(x<=170)&(y>=260)&(y<=295)&(rgb[:,:,0]>rgb[:,:,2]*1.3)&(rgb[:,:,0]>65)&(owner>=2106)&(owner<14993)
q=xyz[chosen];print('selected skin-looking head pixels',len(q))
if len(q):
    field=q[:,1]+.25*(q[:,2]-.02)-.30*abs(q[:,0]-.01)-1.595
    print('bind xyz ranges',q.min(0),q.max(0),'field percentiles',np.percentile(field,[0,25,50,75,100]))
    print('screen bounds',x[chosen].min(),x[chosen].max(),y[chosen].min(),y[chosen].max())
