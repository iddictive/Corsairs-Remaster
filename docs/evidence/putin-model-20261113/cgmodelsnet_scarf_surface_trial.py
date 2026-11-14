"""Test stock scarf material on the retained lower graft surface."""
import ast
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_face_reprojection.py';tree=ast.parse(path.read_text())
fn=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='raster')
scope={'np':np};exec(compile(ast.Module(body=[fn],type_ignores=[]),str(path),'exec'),scope);raster=scope['raster']
data=(here/'cgmodelsnet-refined-rebridge-prototype.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3);off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44})
v=np.frombuffer(data,dt,size//44,off+8);p=v['p'].astype(float);uv=v['uv'].astype(float)
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
base=np.asarray(Image.open(here/'cgmodelsnet-refined-rebridge-atlas.png').convert('RGB'),dtype=float);atlas=base.copy()
# The rectangular dark-blue fabric patch is from the already used stock atlas.
stock=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS),dtype=float)
fabric=stock[710:745,665:800];assert fabric.shape==(35,135,3)
maskmap=np.zeros((1024,2048),dtype=float)
for face in faces:
    if not np.all(face>=2106) or field[face].min()>.021:continue
    q=uv[face]*2048;q[:,1]-=1024
    if np.any(q[:,0]<0) or np.any(q[:,0]>=2048) or np.any(q[:,1]<0) or np.any(q[:,1]>=1024):continue
    result=raster(q,2048,1024)
    if result is None:continue
    x,y,w,inside=result;f=w@field[face]
    t=np.clip((.020-f)/.012,0,1);alpha=t*t*(3-2*t)
    alpha=np.where(inside,alpha,0)
    maskmap[y,x]=np.maximum(maskmap[y,x],alpha)
y,x=np.mgrid[0:1024,0:2048]
Image.fromarray(np.clip(maskmap*255,0,255).astype('uint8')).save(here/'cgmodelsnet-scarf-surface-mask.png')
pattern=fabric[(y//3)%fabric.shape[0],(x//3)%fabric.shape[1]]
a=maskmap[...,None];atlas[1024:]=base[1024:]*(1-a)+pattern*a
Image.fromarray(np.clip(atlas,0,255).astype('uint8')).save(here/'cgmodelsnet-scarf-surface-atlas.png')
print('scarf head texels',int((maskmap>.01).sum()))
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-scarf-surface-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-scarf-surface-atlas.png'),str(angle),str(here/'cgmodelsnet-refined-rebridge-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
