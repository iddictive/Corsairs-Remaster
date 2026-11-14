"""Encode a non-registered candidate TX and verify actual Storm mip layout."""
import hashlib
import io
import struct
import sys
from pathlib import Path
from PIL import Image
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'experiments/native-metal'))
from texture_loader_mip_probe import plan

gm=(here/'cgmodelsnet-neck-retopology-fit.gm').read_bytes()
atlas_bytes=(here/'cgmodelsnet-nose-baseline-atlas.png').read_bytes()
assert hashlib.sha256(gm).hexdigest()=='e73916910134f6d9182f837fb46bc22b64ab17c98bf3dd85fb5897554fda9f05'
assert hashlib.sha256(atlas_bytes).hexdigest()=='4b8c225f922a04d279c1e8477ef4fd20d9b68651bd167110982e924d68ede776'
old,new=b'mr05x.tga\0',b'Putin.tga\0';assert len(old)==len(new) and gm.count(old)==1
candidate=gm.replace(old,new);assert candidate.count(new)==1
image=Image.open(io.BytesIO(atlas_bytes)).convert('RGBA');assert image.size==(2048,2048)
chunks=[];mip=image
for _ in range(6):
    output=io.BytesIO();mip.save(output,format='DDS',pixel_format='DXT1');raw=output.getvalue();assert raw[84:88]==b'DXT1'
    chunk=raw[128:];assert len(chunk)==((mip.width+3)//4)*((mip.height+3)//4)*8
    chunks.append(chunk);mip=mip.resize((mip.width//2,mip.height//2),Image.Resampling.LANCZOS)
texture=struct.pack('<6I',0,2048,2048,len(chunks),int.from_bytes(b'DXT1','little'),len(chunks[0]))+b''.join(chunks)
levels,required=plan(texture[:24],len(texture)-24);assert required==len(texture)-24 and levels==[len(c) for c in chunks]
for level,chunk in enumerate(chunks):
    side=2048>>level
    decoded=Image.frombytes('RGBA',(side,side),chunk,'bcn',(1,'DXT1'));assert decoded.size==(side,side)
(here/'putin-format-probe.gm').write_bytes(candidate)
(here/'putin-format-probe.tga.tx').write_bytes(texture)
print('unregistered candidate GM sha256',hashlib.sha256(candidate).hexdigest())
print('unregistered candidate TX sha256',hashlib.sha256(texture).hexdigest(),'dimensions 2048x2048 mips',len(chunks),'payload sizes',levels,'bytes',len(texture))
