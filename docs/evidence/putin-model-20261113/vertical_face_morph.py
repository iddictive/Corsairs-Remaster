"""Trial smooth vertical face alignment while preserving stock skin weights."""
import hashlib
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
root=here.parents[2]
gm=bytearray((root/'src/assets/characters/putin-phonk/original.gm').read_bytes())
assert hashlib.sha256(gm).hexdigest()=='74b36c17871a497d7eaf25a7c5ab372bc9e1abb9addd806611b3ad1a407eccd1'
h=struct.unpack_from('<11i7f',gm); off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
typ,size=struct.unpack_from('<2i',gm,off); assert typ==4 and size%44==0
dtype=np.dtype({'names':['p','normal'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(gm,dtype=dtype,count=size//44,offset=off+8); p=v['p']; normal=v['normal']; old=p.copy()
zoom=500/.41; screen_y=490-(old[:,1]-1.465)*zoom
center=260.; sigma=35.; amp=14.
g=np.exp(-.5*((screen_y-center)/sigma)**2)
mask=(abs(old[:,0])<.15)&(old[:,1]>1.53)&(old[:,1]<1.9)
p[mask,1]-=amp*g[mask]/zoom
j=1-amp*(screen_y-center)/(sigma*sigma)*g
n=normal.copy(); n[mask,1]/=j[mask]; n/=np.maximum(np.linalg.norm(n,axis=1,keepdims=True),1e-8)
normal[mask]=n[mask]
assert np.min(j[mask])>.7 and np.max(abs(p[:,1]-old[:,1]))<.015
(here/'vertical-face-prototype.gm').write_bytes(gm)
print('moved',int(mask.sum()),'max metres',float(np.max(abs(p[:,1]-old[:,1]))))
