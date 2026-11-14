"""Color GM atlas regions to locate the rear-neck transition by surface."""
import sys
from pathlib import Path
from PIL import Image
import numpy as np
here=Path(__file__).resolve().parent
# Body uses the upper-left and upper-right tiled strip; bridge occupies the
# top-right strip; the complete lower half is the graft's head island.
atlas=np.zeros((2048,2048,3),dtype='uint8');atlas[:1024,:1024]=[32,100,240]
atlas[512:1024,1024:]=[32,100,240];atlas[:128,1024:]=[240,32,200]
atlas[1024:]=[40,225,70]
out=here/'cgmodelsnet-neck-source-mask-atlas.png';Image.fromarray(atlas).save(out)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'8423-front'),(1.,'8423-side'),(-1.,'8423-other-side'),(-.15,'bind')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1;source=source.replace(old,repr(f'cgmodelsnet-neck-source-mask-{label}.png'))
    gm='cgmodelsnet-neck-retopology-fit.gm' if label=='bind' else 'animated-cgmodelsnet-neck-retopology-fit-8423.gm'
    sys.argv=[str(renderer),'officer_5',str(out),str(angle),str(here/gm)]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
