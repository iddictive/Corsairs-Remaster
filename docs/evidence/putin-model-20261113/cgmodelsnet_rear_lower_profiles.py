"""Compare rear-lowering trials and their unchanged-geometry control."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;renderer=here/'render_officer.py'
for label,gm in [('control','animated-cgmodelsnet-stockuv-retopology-8423.gm'),('3','animated-cgmodelsnet-rear-lower-3-8423.gm'),('6','animated-cgmodelsnet-rear-lower-6-8423.gm')]:
    for angle,side in [(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-rear-lower-{label}-8423-{side}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/gm)]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
