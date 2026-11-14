"""Extend the scarf onto the sculpture's exposed rear-neck collar."""
import ast
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image
here=Path(__file__).resolve().parent
fn=next(n for n in ast.parse((here/'cgmodelsnet_face_reprojection.py').read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='raster')
scope={'np':np};exec(compile(ast.Module(body=[fn],type_ignores=[]),'raster','exec'),scope);raster=scope['raster']
data=(here/'cgmodelsnet-neck-retopology-fit.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);o=t+6*h[9];kind,size=struct.unpack_from('<2i',data,o);assert kind==4
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});v=np.frombuffer(data,dt,size//44,o+8);p=v['p'].astype(float);uv=v['uv'].astype(float)
base=np.asarray(Image.open(here/'cgmodelsnet-nose-baseline-atlas.png').convert('RGB'),dtype=float);atlas=base.copy()
stock=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS),dtype=float);fabric=stock[716:736,665:735]
mask=np.zeros((1024,2048));depth=np.full((1024,2048),-np.inf)
for face in faces:
    if not np.all((face>=2106)&(face<14993)):continue
    xyz=p[face];field=xyz[:,1]+.25*(xyz[:,2]-.02)-.30*abs(xyz[:,0]-.01)-1.595
    if field.min()>.052 or xyz[:,2].min()>.025:continue
    q=uv[face]*2048;q[:,1]-=1024
    if np.any(q<0) or np.any(q[:,0]>=2048) or np.any(q[:,1]>=1024):continue
    result=raster(q,2048,1024)
    if result is None:continue
    x,y,w,inside=result;pt=w@xyz
    f=pt[...,1]+.25*(pt[...,2]-.02)-.30*abs(pt[...,0]-.01)-1.595
    a=np.clip((.050-f)/.015,0,1)*np.clip((.025-pt[...,2])/.025,0,1);a=a*a*(3-2*a)
    valid=inside&(a>mask[y,x])
    theta=np.mod(np.arctan2(pt[...,2],pt[...,0]-.01)+np.pi,2*np.pi)/(2*np.pi)
    mirror=np.mod(theta*8*69,2*69);fx=np.minimum(mirror,2*69-mirror);fy=np.clip((f+.055)*19/.105,0,19)
    x0=np.floor(fx).astype(int);x1=np.minimum(x0+1,69);y0=np.floor(fy).astype(int);y1=np.minimum(y0+1,19);dx=(fx-x0)[...,None];dy=(fy-y0)[...,None]
    pattern=(1-dy)*((1-dx)*fabric[y0,x0]+dx*fabric[y0,x1])+dy*((1-dx)*fabric[y1,x0]+dx*fabric[y1,x1])
    patch=atlas[y+1024,x];orig=base[y+1024,x];mix=orig*(1-a[...,None])+pattern*a[...,None];patch[valid]=mix[valid];atlas[y+1024,x]=patch;mask[y,x]=np.where(valid,a,mask[y,x])
out=here/'cgmodelsnet-rear-collar-atlas.png';Image.fromarray(np.clip(atlas,0,255).astype('uint8')).save(out)
print('head texels changed mask',int((mask>.01).sum()))
renderer=here/'render_officer.py'
cases=[('bind-front','cgmodelsnet-neck-retopology-fit.gm',-.15),('bind-profile','cgmodelsnet-neck-retopology-fit.gm',1.),('bind-other-profile','cgmodelsnet-neck-retopology-fit.gm',-1.),('8423-front','animated-cgmodelsnet-neck-retopology-fit-8423.gm',-.15),('8423-side','animated-cgmodelsnet-neck-retopology-fit-8423.gm',1.),('8423-other-side','animated-cgmodelsnet-neck-retopology-fit-8423.gm',-1.),('1985','animated-cgmodelsnet-neck-retopology-fit-1985.gm',-.15),('run','animated-cgmodelsnet-neck-retopology-fit-run.gm',-.15)]
for label,gm,angle in cases:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-rear-collar-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(out),str(angle),str(here/gm)]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
