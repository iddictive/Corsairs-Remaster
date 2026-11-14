"""Locate visible profile-nose pixels on the actual GM and its atlas."""
import ast
import math
import struct
from pathlib import Path
import numpy as np
from PIL import Image
here=Path(__file__).resolve().parent
tree=ast.parse((here/'cgmodelsnet_face_reprojection.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='raster')
scope={'np':np};exec(compile(ast.Module(body=[fn],type_ignores=[]),'raster','exec'),scope);raster=scope['raster']
data=(here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);o=t+6*h[9];kind,size=struct.unpack_from('<2i',data,o);assert kind==4
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});v=np.frombuffer(data,dt,size//44,o+8);p=v['p'].astype(float);uv=v['uv'].astype(float)
image=np.asarray(Image.open(here/'cgmodelsnet-nose-uncompressed-profile.png').convert('RGB'))
angle=1.;c,s=math.cos(angle),math.sin(angle);scale=500/.41
screen=np.column_stack(((p[:,0]*c+p[:,2]*s+.004)*scale+256,490-(p[:,1]-1.465)*scale,-p[:,0]*s+p[:,2]*c))
depth=np.full((512,512),-np.inf); xyz=np.zeros((512,512,3)); tex=np.zeros((512,512,2))
for face in faces:
    q=screen[face]
    if q[:,0].max()<380 or q[:,0].min()>426 or q[:,1].max()<215 or q[:,1].min()>248:continue
    result=raster(q[:,:2],512,512)
    if result is None:continue
    x,y,w,inside=result;z=w@q[:,2];valid=inside&(z>depth[y,x]);depth[y,x]=np.where(valid,z,depth[y,x])
    xyz[y,x]=np.where(valid[...,None],w@p[face],xyz[y,x]);tex[y,x]=np.where(valid[...,None],w@uv[face],tex[y,x])
for y,x in [(229,389),(229,395),(229,400),(232,405),(235,400),(235,390),(224,395),(240,400)]:
    print('pixel',x,y,'rgb',image[y,x].tolist(),'xyz',np.round(xyz[y,x],5).tolist(),'uv',np.round(tex[y,x],5).tolist())
