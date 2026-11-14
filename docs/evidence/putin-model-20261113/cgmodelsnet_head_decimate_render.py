"""Render decimated GM in static views and representative animation frames."""
import math
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr('cgmodelsnet-decimated-'+label+'.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/'cgmodelsnet-head-decimated-trial.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-head-decimated-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-decimated-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('run',163),('5320',5320),('7236',7236),('8423',8423)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
