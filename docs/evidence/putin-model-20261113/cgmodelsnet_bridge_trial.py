"""Connect main measured cut loops as a diagnostic, not delivery output."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'seam_bridge_trial.py';source=path.read_text()
changes=[
    ("data=(here/f'{input_prefix}-graft-prototype.gm').read_bytes()", "data=(here/'cgmodelsnet-graft-clipped-prototype.gm').read_bytes()"),
    ("Image.open(here/f'{input_prefix}-graft-atlas.png')", "Image.open(here/'cgmodelsnet-graft-clipped-atlas.png')"),
    ("output_prefix='inset-bridge' if inset else 'seam-bridge'", "output_prefix='cgmodelsnet-bridge'"),
    ('split=2152','split=2132'),
    ('loop(np.all(faces<split,axis=1),1.595)', 'loop(np.all(faces<split,axis=1),1.59349)'),
    ('loop(np.all(faces>=split,axis=1),1.600)', 'loop(np.all(faces>=split,axis=1),1.59349)'),
]
for old,new in changes:
    assert source.count(old)==1,(old,source.count(old))
    source=source.replace(old,new)
sys.argv=[str(path)]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    src=renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(needle)==1
        src=src.replace(needle,repr(f'cgmodelsnet-bridge-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-bridge-atlas.png'),str(angle),str(here/'cgmodelsnet-bridge-prototype.gm')]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
