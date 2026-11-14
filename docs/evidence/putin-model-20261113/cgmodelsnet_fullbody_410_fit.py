"""Fit both frame-410 poses to one common offline camera without cropping."""
import math
import struct
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
models=[('candidate','animated-cgmodelsnet-neck-extreme-410.gm','cgmodelsnet-nose-baseline-atlas.png'),('stock','animated-stock-officer-410.gm','officer_5-atlas.png')]
rot=[];angle=-.15;c,s=math.cos(angle),math.sin(angle)
for _,name,_ in models:
    data=(here/name).read_bytes();h=struct.unpack_from('<11i7f',data)
    off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
    kind,size=struct.unpack_from('<2i',data,off);assert kind==4
    dtype=np.dtype({'names':['p'],'formats':[('<f4',3)],'offsets':[0],'itemsize':44})
    pos=np.frombuffer(data,dtype,size//44,off+8)['p'].astype(float)
    rot.append(np.column_stack((pos[:,0]*c+pos[:,2]*s,pos[:,1])))
both=np.vstack(rot);lo=both.min(0);hi=both.max(0)
scale=float(min(476/(hi[0]-lo[0]),476/(hi[1]-lo[1])));cx=float((lo[0]+hi[0])/2);cy=float((lo[1]+hi[1])/2)
print('shared frame bounds',lo,hi,'scale',scale)
renderer=here/'render_officer.py'
for label,gm,atlas in models:
    source=renderer.read_text();needle="module = {'__file__': str(src_path), '__name__': 'diagnostic'}";assert source.count(needle)==1
    old="original_tx = (base / f'Textures/Characters/{textures[model]}.tga.tx').read_bytes()"
    assert source.count(old)==1
    source=source.replace(old,"original_tx = (root / 'src/assets/characters/putin-phonk/original.tga.tx').read_bytes()")
    injection=f"""old_scale = 'side, angle, scale = 512, '+repr(angle)+', 500 / .41'
assert source.count(old_scale) == 1
source = source.replace(old_scale, 'side, angle, scale = 512, '+repr(angle)+', '+repr({scale!r}))
old_xy = '(x + .004) * scale + side / 2, 490 - (y - 1.465) * scale'
assert source.count(old_xy) == 1
source = source.replace(old_xy, '(x - '+repr({cx!r})+') * scale + side / 2, side / 2 - (y - '+repr({cy!r})+') * scale')
"""
    source=source.replace(needle,injection+needle)
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-fullbody-410-fit-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/atlas),str(angle),str(here/gm)]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
