"""Check topology and render the reduced corrected head in three angles."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_recut_topology_check.py';source=path.read_text()
source=source.replace('cgmodelsnet-stockuv-retopology.gm\',694','cgmodelsnet-recut-winding-trial.gm\',344').replace('cgmodelsnet-recut-trial.gm\',344','cgmodelsnet-recut-decimated-trial.gm\',344')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'side'),(-1.,'other-side')]:
    source=renderer.read_text();needle='source = src_path.read_text()';assert source.count(needle)==1
    source=source.replace(needle,needle+"\nsource = source.replace('if abs(denominator) < 1e-8:', 'if denominator <= 1e-8:')")
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-recut-decimated-culled-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/'cgmodelsnet-recut-decimated-trial.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
