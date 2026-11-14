"""Test the pre-reprojection atlas locally around the nose tip."""
import ast
import math
import struct
import sys
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
current=np.asarray(Image.open(here/'cgmodelsnet-scarf-worlduv-bridge-atlas.png').convert('RGB'),dtype=float)
baseline=np.asarray(Image.open(here/'cgmodelsnet-radialfit-repaired-atlas.png').convert('RGB'),dtype=float)
assert current.shape==baseline.shape==(2048,2048,3)
mask=np.zeros((1024,2048),dtype=float)
for face in faces:
    if not np.all((face>=2106)&(face<14993)):continue
    q3=p[face]
    if q3[:,1].max()<1.668 or q3[:,1].min()>1.705 or q3[:,2].max()<.115 or q3[:,0].min()>.025 or q3[:,0].max()<-.025:continue
    q=uv[face]*2048;q[:,1]-=1024
    if np.any(q<0) or np.any(q[:,0]>=2048) or np.any(q[:,1]>=1024):continue
    result=raster(q,2048,1024)
    if result is None:continue
    x,y,w,inside=result;pt=w@q3
    fade=np.clip((.025-abs(pt[...,0]))/.010,0,1)*np.clip((pt[...,1]-1.668)/.007,0,1)*np.clip((1.705-pt[...,1])/.007,0,1)*np.clip((pt[...,2]-.115)/.010,0,1)
    mask[y,x]=np.maximum(mask[y,x],np.where(inside,fade,0))
atlas=current.copy();a=mask[...,None];atlas[1024:]=current[1024:]*(1-a)+baseline[1024:]*a
out=here/'cgmodelsnet-nose-baseline-atlas.png';Image.fromarray(np.clip(atlas,0,255).astype('uint8')).save(out)
print('changed head texels',int((mask>.01).sum()),'fully blended',int((mask>.99).sum()))
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-nose-baseline-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(out),str(angle),str(here/'cgmodelsnet-body-scarf-worlduv.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
