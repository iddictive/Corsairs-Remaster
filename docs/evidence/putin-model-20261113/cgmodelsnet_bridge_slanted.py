"""Bridge the single measured body loop to the obliquely cut head."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'seam_bridge_trial.py';source=path.read_text()
changes=[
    ("data=(here/f'{input_prefix}-graft-prototype.gm').read_bytes()", "data=(here/'cgmodelsnet-graft-slanted-prototype.gm').read_bytes()"),
    ("Image.open(here/f'{input_prefix}-graft-atlas.png')", "Image.open(here/'cgmodelsnet-graft-slanted-atlas.png')"),
    ("output_prefix='inset-bridge' if inset else 'seam-bridge'", "output_prefix='cgmodelsnet-bridge-slanted'"),
    ('split=2152','split=2108'),
    ('abs(pos[edges,1]-plane)', 'abs(pos[edges,1]+.25*(pos[edges,2]-.02)-plane)'),
    ('loop(np.all(faces>=split,axis=1),1.600)', 'loop(np.all(faces>=split,axis=1),1.595)'),
]
for old,new in changes:
    assert source.count(old)==1,(old,source.count(old))
    source=source.replace(old,new)
sys.argv=[str(path)]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    src=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(old)==1
        src=src.replace(old,repr(f'cgmodelsnet-bridge-slanted-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-bridge-slanted-atlas.png'),str(angle),str(here/'cgmodelsnet-bridge-slanted-prototype.gm')]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
