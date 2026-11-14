"""Decode two stock texture formats and render candidate GMs offline."""
import io
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
root=here.parents[2]
base=root/'experiments/native-metal/.cache/runtime/RESOURCE'
for model,texture in [('trader_5','trader_5'),('shipowner_2','shipowner_1')]:
    data=(base/f'Textures/Characters/{texture}.tga.tx').read_bytes()
    _,w,h,_,fmt,size=struct.unpack_from('<6I',data)
    assert (w,h)==(1024,1024)
    if fmt==23:  # D3DFMT_R5G6B5
        pixel=np.frombuffer(data,dtype='<u2',count=w*h,offset=24).reshape(h,w).astype('uint32')
        color=np.stack(((pixel>>11)*255//31,((pixel>>5)&63)*255//63,(pixel&31)*255//31),axis=2).astype('uint8')
        atlas=Image.fromarray(color,'RGB')
    elif fmt==int.from_bytes(b'DXT3','little'):
        atlas=Image.frombytes('RGBA',(w,h),data[24:24+size],'bcn',(2,'DXT3')).convert('RGB')
    else: raise ValueError((model,fmt))
    atlas_path=here/f'{model}-comparison-atlas.png'
    atlas.save(atlas_path)
    render=here/'render_officer.py'
    source=render.read_text().replace("f'{prefix}-candidate-preview.png'",repr(f'{model}-comparison-front.png'))
    sys.argv=[str(render),'officer_5',str(atlas_path),'-.15',str(base/f'MODELS/Characters/{model}.gm')]
    exec(compile(source,str(render),'exec'),{'__file__':str(render),'__name__':'__main__'})
