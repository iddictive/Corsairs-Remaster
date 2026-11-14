"""CPU comparison with projected-winding culling; not an engine replay."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;renderer=here/'render_officer.py'
for model in ['recut-trial','recut-winding-trial']:
    for angle,label in [(-.15,'front'),(1.,'side'),(-1.,'other-side')]:
        source=renderer.read_text()
        needle="source = src_path.read_text()";assert source.count(needle)==1
        source=source.replace(needle,needle+"\nsource = source.replace('if abs(denominator) < 1e-8:', 'if denominator <= 1e-8:')")
        for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
            assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-{model}-culled-{label}.png'))
        sys.argv=[str(renderer),'officer_5',str(here/'cgmodelsnet-stockuv-scarf-atlas.png'),str(angle),str(here/f'cgmodelsnet-{model}.gm')]
        exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
