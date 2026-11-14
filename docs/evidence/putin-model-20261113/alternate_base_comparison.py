"""Render stock one-material heads with their actual texture, offline."""
import struct
import sys
from pathlib import Path
from PIL import Image

here=Path(__file__).resolve().parent
root=here.parents[2]
base=root/'experiments/native-metal/.cache/runtime/RESOURCE'
renderer=here/'render_officer.py'
for model,texture in [('citiz_48','shipowner_13'),('citiz_50','Citiz50xxxx'),('citiz_51','Citiz51xxxxx'),('BigPirate','BigPirate'),('worker_49','Citiz49xxxx'),('trader_11','trader_11')]:
    data=(base/f'Textures/Characters/{texture}.tga.tx').read_bytes()
    _,w,h,_,fmt,size=struct.unpack_from('<6I',data)
    assert (w,h)==(1024,1024) and fmt==int.from_bytes(b'DXT1','little')
    atlas=Image.frombytes('RGBA',(w,h),data[24:24+size],'bcn',(1,'DXT1')).convert('RGB')
    texture_path=here/f'alternate-{model}-atlas.png'; atlas.save(texture_path)
    for angle,view in [(-.15,'front'),(-1.5707963268,'profile')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'", "f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1
            source=source.replace(old,repr(f'alternate-{model}-{view}.png'))
        sys.argv=[str(renderer),'officer_5',str(texture_path),str(angle),str(base/f'MODELS/Characters/{model}.gm')]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
