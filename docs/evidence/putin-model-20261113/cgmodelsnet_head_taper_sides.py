"""Compare the tapered and previous head weights from both sides."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;renderer=here/'render_officer.py'
cases=[('taper-5320','animated-cgmodelsnet-head-taper-5320.gm'),('prior-5320','animated-cgmodelsnet-declared-5320.gm'),('taper-7236','animated-cgmodelsnet-head-taper-7236.gm'),('prior-7236','animated-cgmodelsnet-allframes-7236.gm')]
for label,gm in cases:
    for angle,side in [(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-head-{label}-{side}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/gm)]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
