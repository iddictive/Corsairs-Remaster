"""Mark candidate stock-body neck faces without changing their skinning."""
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
v=np.frombuffer(data,dt,len(rows),off+8);faces=np.frombuffer(data,'<u2',3*h[9],tri).reshape(-1,3).copy();pos=v['p'];uv=v['uv']
center=pos[faces].mean(1);tex=uv[faces].mean(1)%1
atlas=np.asarray(Image.open(here/'cgmodelsnet-scarf-clean-atlas.png').convert('RGB')).copy()
color=atlas[(tex[:,1]*2047).astype(int),(tex[:,0]*2047).astype(int)].astype(int)
mask=np.all(faces<2106,axis=1)&(center[:,1]>1.55)&(center[:,1]<1.62)&(abs(center[:,0]-.01)<.1)&(center[:,2]>-.1)&(center[:,2]<.16)&(color[:,0]>color[:,2]*1.3)&(color[:,0]>55)
remap={}
for f in faces[mask].copy():
    for idx in f:
        idx=int(idx)
        if idx not in remap:
            row=bytearray(rows[idx]);struct.pack_into('<2f',row,36,1984/2048,896/2048);remap[idx]=len(rows);rows.append(row)
faces[mask]=np.array([[remap[int(i)] for i in f] for f in faces[mask]],dtype='<u2')
assert len(rows)<65536 and mask.sum()>0
prefix=bytearray(data[:tri]);fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
result=prefix+faces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[off+8+size:]
(here/'cgmodelsnet-body-scarf-mask.gm').write_bytes(result)
atlas[880:912,1968:2000]=[255,0,255];Image.fromarray(atlas).save(here/'cgmodelsnet-body-scarf-mask-atlas.png')
print('selected body faces',int(mask.sum()),'duplicated vertices',len(remap))
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-body-scarf-mask-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-body-scarf-mask-atlas.png'),str(angle),str(here/'cgmodelsnet-body-scarf-mask.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
