"""Test a vertically corrected sculpture-to-stock-head depth registration."""
import math
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
path=here/'smooth_face_fit_trial.py'; source=path.read_text()
changes=[('y=1.385+ref[:,2]*.0028','y=1.399+ref[:,2]*.0028'),
         ("photo-face-prototype.gm","nose-subdivision-prototype.gm"),
         ("smooth-face-fit-prototype.gm","registered-sculpture-fit-prototype.gm")]
for old,new in changes:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

# Keep the projected front texture registered to the moved vertices.
base=(here/'nose-subdivision-prototype.gm').read_bytes()
out=bytearray((here/'registered-sculpture-fit-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',out);off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',out,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44})
old=np.frombuffer(base,dtype=dt,count=size//44,offset=off+8)
new=np.frombuffer(out,dtype=dt,count=size//44,offset=off+8)
uv=new['uv'].astype(float).copy();mask=(uv[:,0]>.5)&(uv[:,0]<.75)&(uv[:,1]>=0)&(uv[:,1]<.25)
d=new['p'].astype(float)-old['p'].astype(float);scale=500/.41;c,s=math.cos(-.15),math.sin(-.15)
uv[mask,0]+=(d[mask,0]*c+d[mask,2]*s)*scale/2048;uv[mask,1]-=d[mask,1]*scale/2048
new['uv'][:]=uv
(here/'registered-sculpture-fit-prototype.gm').write_bytes(out)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    src=renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(needle)==1
        src=src.replace(needle,repr(f'registered-sculpture-fit-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'identity-geometry-face-atlas.png'),str(angle),str(here/'registered-sculpture-fit-prototype.gm')]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
