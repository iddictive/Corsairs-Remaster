"""Check changed body edges and both profiles in extreme poses."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_head_taper_edge_probe.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-weight-taper-trial.gm','cgmodelsnet-neck-body-follow-trial.gm'),('cgmodelsnet-head-taper-edge-probe.json','cgmodelsnet-neck-body-follow-edges.json')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for frame in [5320,7236,8423]:
    for angle,label in [(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text()
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-neck-body-follow-{frame}-{label}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/f'animated-cgmodelsnet-neck-body-follow-{frame}.gm')]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
