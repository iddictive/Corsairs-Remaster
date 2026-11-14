"""Bake a narrow scarf transition on separate stock-body UV triangles."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent;data=(here/'cgmodelsnet-refined-rebridge-prototype.gm').read_bytes()
h=list(struct.unpack_from('<11i7f',data));obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7];tri=obj+104*h[8];off=tri+6*h[9]
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
rows=[bytearray(data[off+8+44*i:off+8+44*(i+1)]) for i in range(size//44)]
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44})
v=np.frombuffer(data,dt,len(rows),off+8);faces=np.frombuffer(data,'<u2',3*h[9],tri).reshape(-1,3).copy();p=v['p'].astype(float);uv=v['uv'].astype(float)
original=np.asarray(Image.open(here/'cgmodelsnet-scarf-clean-atlas.png').convert('RGB'),dtype=float);atlas=original.copy()
center=p[faces].mean(1);tex=(uv[faces].mean(1)%1);color=original[(tex[:,1]*2047).astype(int),(tex[:,0]*2047).astype(int)].astype(int)
select=np.all(faces<2106,axis=1)&(center[:,1]>1.55)&(center[:,1]<1.62)&(abs(center[:,0]-.01)<.1)&(center[:,2]>-.1)&(center[:,2]<.16)&(color[:,0]>color[:,2]*1.3)&(color[:,0]>55)
ids=np.flatnonzero(select);assert 0<len(ids)<=180
# The current GM uses top-left for body, y<128 on the right for the bridge,
# and the lower half for the head. Verify our tile area is not referenced.
all_uv=uv[faces].reshape(-1,2)%1
assert not np.any((all_uv[:,0]>=.5)&(all_uv[:,1]>=.25)&(all_uv[:,1]<.5))
stock=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS),dtype=float)
fabric=stock[716:736,665:735];assert fabric.shape==(20,70,3)
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
tile=48;pad=4;side=tile-2*pad;cols=21
yy,xx=np.mgrid[0:tile,0:tile];u=(xx+.5-pad)/side;w=(yy+.5-pad)/side
weights=np.stack((1-u-w,u,w),axis=-1);weights=np.maximum(weights,0);weights/=np.maximum(weights.sum(axis=-1,keepdims=True),1e-9)
for j,fid in enumerate(ids):
    f=faces[fid].copy();x0=1024+(j%cols)*tile;y0=512+(j//cols)*tile
    assert x0+tile<=2048 and y0+tile<=1024
    src=weights@uv[f];ix=np.clip(((src[...,0]%1)*2048).astype(int),0,2047);iy=np.clip(((src[...,1]%1)*2048).astype(int),0,2047)
    original_color=original[iy,ix];height=weights@field[f]
    t=np.clip((height+.020)/.012,0,1);alpha=t*t*(3-2*t)
    pattern=fabric[(yy//2)%20,(xx//2)%70]
    atlas[y0:y0+tile,x0:x0+tile]=original_color*(1-alpha[...,None])+pattern*alpha[...,None]
    coords=[(x0+pad,y0+pad),(x0+pad+side,y0+pad),(x0+pad,y0+pad+side)]
    for k,index in enumerate(f):
        row=bytearray(rows[int(index)]);struct.pack_into('<2f',row,36,coords[k][0]/2048,coords[k][1]/2048)
        faces[fid,k]=len(rows);rows.append(row)
assert len(rows)<65536
prefix=bytearray(data[:tri]);fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
result=prefix+faces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[off+8+size:]
(here/'cgmodelsnet-body-scarf-bake.gm').write_bytes(result)
Image.fromarray(np.clip(atlas,0,255).astype('uint8')).save(here/'cgmodelsnet-body-scarf-bake-atlas.png')
print('body triangles',len(ids),'duplicated vertices',len(ids)*3)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-body-scarf-bake-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-body-scarf-bake-atlas.png'),str(angle),str(here/'cgmodelsnet-body-scarf-bake.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
