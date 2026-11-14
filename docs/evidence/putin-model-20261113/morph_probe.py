"""Bounded head-width GM prototype; retains topology, UV and bone weights."""
import hashlib
import math
import struct
from pathlib import Path

here = Path(__file__).resolve().parent
root = here.parents[2]
data = bytearray((root/'src/assets/characters/putin-phonk/original.gm').read_bytes())
assert hashlib.sha256(data).hexdigest() == '74b36c17871a497d7eaf25a7c5ab372bc9e1abb9addd806611b3ad1a407eccd1'
h = struct.unpack_from('<11i7f',data)
o = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
typ,size = struct.unpack_from('<2i',data,o); assert typ==4 and size%44==0
start=o+8; count=0
for i in range(size//44):
    at=start+i*44
    x,y,z=struct.unpack_from('<3f',data,at)
    if abs(x)>.16 or y<=1.52: continue
    t=min(1,max(0,(y-1.52)/.08)); smooth=t*t*(3-2*t)
    a=1-.10*smooth
    da=-.10*(6*t-6*t*t)/.08 if 0<t<1 else 0
    nx,ny,nz=struct.unpack_from('<3f',data,at+20)
    nx1=nx/a; ny1=ny-nx*x*da/a
    length=math.sqrt(nx1*nx1+ny1*ny1+nz*nz)
    if length<1e-8: continue
    struct.pack_into('<f',data,at,x*a)
    struct.pack_into('<3f',data,at+20,nx1/length,ny1/length,nz/length)
    count+=1
assert 400<count<800,count
target=here/'morph-prototype.gm'; target.write_bytes(data)
print('changed vertices',count,'sha256',hashlib.sha256(data).hexdigest())
