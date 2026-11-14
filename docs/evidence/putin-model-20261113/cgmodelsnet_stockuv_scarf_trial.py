"""Paint the narrow scarf onto its nonoverlapping original body UVs."""
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
data=(here/'cgmodelsnet-refined-rebridge-prototype.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);o=t+6*h[9];kind,size=struct.unpack_from('<2i',data,o);assert kind==4
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44});v=np.frombuffer(data,dt,size//44,o+8);p=v['p'].astype(float);uv=v['uv'].astype(float)
center=p[faces].mean(1);field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
select=np.all(faces<2106,axis=1)&(field[faces].max(1)>-.055)&(center[:,1]>1.46)&(abs(center[:,0]-.01)<.12)&(abs(center[:,2])<.16)
assert select.sum()==76
base=np.asarray(Image.open(here/'cgmodelsnet-nose-baseline-atlas.png').convert('RGB'),dtype=float);atlas=base.copy()
stock=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS),dtype=float);fabric=stock[716:736,665:735];visited=np.zeros((1024,1024),bool)
for face in faces[select]:
    q=(uv[face]%1)*2048;assert np.all((q>=0)&(q<1024))
    result=raster(q,1024,1024)
    if result is None:continue
    x,y,w,inside=result;xyz=w@p[face];height=w@field[face]
    t=np.clip((height+.055)/.025,0,1);alpha=t*t*(3-2*t);alpha=np.where(inside,alpha,0)
    theta=np.mod(np.arctan2(xyz[...,2],xyz[...,0]-.01)+np.pi,2*np.pi)/(2*np.pi)
    mirror=np.mod(theta*8*69,2*69);fx=np.minimum(mirror,2*69-mirror);fy=np.clip((height+.055)*19/.055,0,19)
    x0=np.floor(fx).astype(int);x1=np.minimum(x0+1,69);y0=np.floor(fy).astype(int);y1=np.minimum(y0+1,19);dx=(fx-x0)[...,None];dy=(fy-y0)[...,None]
    pattern=(1-dy)*((1-dx)*fabric[y0,x0]+dx*fabric[y0,x1])+dy*((1-dx)*fabric[y1,x0]+dx*fabric[y1,x1])
    assert not np.any(inside&visited[y,x])
    patch=atlas[y,x];original=base[y,x];patch[inside]=(original*(1-alpha[...,None])+pattern*alpha[...,None])[inside];atlas[y,x]=patch;visited[y,x]|=inside
out=here/'cgmodelsnet-stockuv-scarf-atlas.png';Image.fromarray(np.clip(atlas,0,255).astype('uint8')).save(out)
assert np.array_equal(atlas[1024:],base[1024:])
print('selected faces',int(select.sum()),'painted UV pixels',int(visited.sum()))
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-stockuv-scarf-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(out),str(angle),str(here/'cgmodelsnet-refined-rebridge-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
