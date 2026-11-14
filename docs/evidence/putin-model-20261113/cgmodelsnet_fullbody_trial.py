"""Render existing actual GM poses with an offline full-body camera."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
renderer=here/'render_officer.py'
cases=[('bind-front','cgmodelsnet-body-scarf-worlduv.gm',-.15),('bind-profile','cgmodelsnet-body-scarf-worlduv.gm',math.pi/2),('walk','animated-cgmodelsnet-nose-baseline-walk.gm',-.15),('run','animated-cgmodelsnet-nose-baseline-run.gm',-.15),('fight-stand','animated-cgmodelsnet-nose-baseline-fight-stand.gm',-.15),('8423','animated-cgmodelsnet-neck-extreme-8423.gm',-.15),('410','animated-cgmodelsnet-neck-extreme-410.gm',-.15),('1985','animated-cgmodelsnet-neck-extreme-1985.gm',-.15)]
for label,gm,angle in cases:
    source=renderer.read_text()
    old="original_tx = (base / f'Textures/Characters/{textures[model]}.tga.tx').read_bytes()"
    assert source.count(old)==1
    source=source.replace(old,"original_tx = (root / 'src/assets/characters/putin-phonk/original.tga.tx').read_bytes()")
    needle="module = {'__file__': str(src_path), '__name__': 'diagnostic'}"
    assert source.count(needle)==1
    inject="""old_scale = 'scale = 512, '+repr(angle)+', 500 / .41'
old_height = '490 - (y - 1.465) * scale'
assert source.count(old_scale) == 1 and source.count(old_height) == 1
source = source.replace(old_scale, 'scale = 512, '+repr(angle)+', 500 / 2.15')
source = source.replace(old_height, '480 - y * scale')
"""
    source=source.replace(needle,inject+needle)
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(f'cgmodelsnet-fullbody-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-nose-baseline-atlas.png'),str(angle),str(here/gm)]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
