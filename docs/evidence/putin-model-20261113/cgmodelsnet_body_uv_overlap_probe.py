"""Measure overlap of the candidate scarf-body faces in their stock UV island."""
import ast
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
fn=next(n for n in ast.parse((here/'cgmodelsnet_face_reprojection.py').read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='raster')
scope={'np':np};exec(compile(ast.Module(body=[fn],type_ignores=[]),'raster','exec'),scope);raster=scope['raster']
data=(here/'cgmodelsnet-refined-rebridge-prototype.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);o=t+6*h[9];kind,size=struct.unpack_from('<2i',data,o);assert kind==4
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});v=np.frombuffer(data,dt,size//44,o+8);p=v['p'].astype(float);uv=v['uv'].astype(float)
center=p[faces].mean(1);field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
select=np.all(faces<2106,axis=1)&(field[faces].max(1)>-.055)&(center[:,1]>1.46)&(abs(center[:,0]-.01)<.12)&(abs(center[:,2])<.16)
selected=np.zeros((1024,1024),np.uint16);other=np.zeros_like(selected)
for i,face in enumerate(faces):
    if not np.all(face<2106):continue
    q=(uv[face]%1)*2048
    if np.any(q<0) or np.any(q>=1024):continue
    result=raster(q,1024,1024)
    if result is None:continue
    x,y,w,inside=result;buf=selected if select[i] else other;buf[y,x]+=inside.astype(np.uint16)
print('selected triangles',int(select.sum()),'covered texels',int((selected>0).sum()),'overlapping other body texels',int(((selected>0)&(other>0)).sum()),'multiple selected texels',int((selected>1).sum()))
