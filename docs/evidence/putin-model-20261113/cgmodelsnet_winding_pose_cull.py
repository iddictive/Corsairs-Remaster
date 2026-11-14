"""Cull the corrected GM in sampled man and musket poses, offline."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_recut_render.py';source=path.read_text();source=source[:source.index("renderer=here/'render_officer.py'")]
for old,new in [('cgmodelsnet-recut-trial.gm','cgmodelsnet-recut-winding-trial.gm'),('animated-cgmodelsnet-recut-','animated-cgmodelsnet-winding-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_recut_musket.py';source=path.read_text()
for old,new in [('cgmodelsnet-recut-trial.gm','cgmodelsnet-recut-winding-trial.gm'),('animated-cgmodelsnet-recut-musket-','animated-cgmodelsnet-winding-musket-'),('cgmodelsnet-recut-musket-scan.json','cgmodelsnet-winding-musket-scan.json')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for label in ['run','5320','8423','musket-shot','musket-idle']:
    for angle,side in [(-.15,'front'),(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text();needle='source = src_path.read_text()';assert source.count(needle)==1
        source=source.replace(needle,needle+"\nsource = source.replace('if abs(denominator) < 1e-8:', 'if denominator <= 1e-8:')")
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-winding-{label}-culled-{side}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/f'animated-cgmodelsnet-winding-{label}.gm')]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
