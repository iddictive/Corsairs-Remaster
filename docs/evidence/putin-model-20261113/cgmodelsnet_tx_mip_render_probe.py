"""Render selected encoded mip levels; this does not infer engine LOD selection."""
import struct
from pathlib import Path
here=Path(__file__).resolve().parent
texture=(here/'putin-format-probe.tga.tx').read_bytes();flags,w,h,mips,fmt,size=struct.unpack_from('<6I',texture)
assert (flags,w,h,mips,size)==(0,2048,2048,6,2097152)
offset=24
for level in range(mips):
    side=2048>>level;length=((side+3)//4)**2*8;chunk=texture[offset:offset+length];assert len(chunk)==length;offset+=length
    if level not in (2,3,4,5):continue
    tmp=here/f'putin-format-mip-{side}-probe.tga.tx';tmp.write_bytes(struct.pack('<6I',0,side,side,1,fmt,length)+chunk)
    path=here/'cgmodelsnet_tx_render_probe.py';source=path.read_text()
    for old,new in [('putin-format-probe.tga.tx',tmp.name),('(2048, 2048)',f'({side}, {side})'),('putin-format-probe-{label}.png',f'putin-format-mip-{side}-{{label}}.png')]:
        assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
    source=source.replace("[(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]","[(-.15,'front'),(1.,'profile')]")
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
assert offset==len(texture)
