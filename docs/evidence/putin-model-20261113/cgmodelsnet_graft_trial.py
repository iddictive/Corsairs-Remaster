"""Experimental multi-view bake and graft of the bounded reduced CC BY head."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'multiview_graft_trial.py';source=path.read_text()
changes=[
    ('bobnojio-head-reduced-1.5.STL','cgmodelsnet-head-bounded.STL'),
    ('source[:,:,2]>=83.', 'source[:,:,2]>=25.'),
    ('.010+points[:,0]*.0025,1.385+points[:,2]*.0028,.06-points[:,1]*.0025',
     '.010+(points[:,0]-18.67)*.00919,1.44855+points[:,2]*.007247,.152-points[:,1]*.0073'),
    ('photo-feathered-target.png','identity-geometry-face-front.png'),
    ('photo-face-profile.png','identity-geometry-face-profile.png'),
    ('photo-face-other-profile.png','identity-geometry-face-other-profile.png'),
    ('photo-face-atlas.png','identity-geometry-face-atlas.png'),
    ('multiview-graft-atlas.png','cgmodelsnet-graft-atlas.png'),
    ('multiview-graft-prototype.gm','cgmodelsnet-graft-prototype.gm'),
]
for old,new in changes:
    assert source.count(old)==1,(old,source.count(old))
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    src=renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(needle)==1
        src=src.replace(needle,repr(f'cgmodelsnet-graft-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-graft-atlas.png'),str(angle),str(here/'cgmodelsnet-graft-prototype.gm')]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
