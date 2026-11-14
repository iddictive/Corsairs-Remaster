"""Render the actual encoded candidate TX with the renamed candidate GM."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    changes=[("atlas = sys.argv[2] if len(sys.argv) > 2 else None","atlas = None"),
             ("original_tx = (base / f'Textures/Characters/{textures[model]}.tga.tx').read_bytes()","original_tx = (here / 'putin-format-probe.tga.tx').read_bytes()"),
             ("h[1:3] == (1024, 1024)","h[1:3] == (2048, 2048)"),
             ("Image.frombytes('RGBA', (1024, 1024), original_tx","Image.frombytes('RGBA', (2048, 2048), original_tx"),
             ("original.save(here / f'{model}-atlas.png')","pass"),
             ("output = here / f'{model}-original-preview.png'",f"output = here / 'putin-format-probe-{label}.png'")]
    for old,new in changes:
        assert source.count(old)==1,(old,source.count(old));source=source.replace(old,new)
    sys.argv=[str(renderer),'officer_5','unused',str(angle),str(here/'putin-format-probe.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
