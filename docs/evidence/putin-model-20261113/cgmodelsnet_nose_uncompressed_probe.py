"""Compare the GM atlas before and after the runtime-like DXT1 encoding."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    needle="source = src_path.read_text()"
    assert source.count(needle)==1
    inject="""\nold = \"texture = Image.frombytes('RGBA', (header[1], header[2]), tx[24:24 + header[5]], 'bcn', (1, 'DXT1')).convert('RGB')\"
assert source.count(old) == 1
source = source.replace(old, \"texture = Image.open(atlas).convert('RGB')\")"""
    source=source.replace(needle,needle+inject)
    old="module = {'__file__': str(src_path), '__name__': 'diagnostic'}"
    assert source.count(old)==1
    source=source.replace(old,"module = {'__file__': str(src_path), '__name__': 'diagnostic', 'atlas': atlas}")
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr(f'cgmodelsnet-nose-uncompressed-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-scarf-worlduv-bridge-atlas.png'),str(angle),str(here/'cgmodelsnet-body-scarf-worlduv.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
