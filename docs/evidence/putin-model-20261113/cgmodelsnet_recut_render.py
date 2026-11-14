"""Render the rebuilt-neck trial in static and sampled posed views."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-recut-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-recut-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('run',163),('5320',5320),('7236',7236),('8423',8423)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for label,gm in [('bind','cgmodelsnet-recut-trial.gm'),('8423','animated-cgmodelsnet-recut-8423.gm'),('5320','animated-cgmodelsnet-recut-5320.gm')]:
    for angle,side in [(-.15,'front'),(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-recut-{label}-{side}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/gm)]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
