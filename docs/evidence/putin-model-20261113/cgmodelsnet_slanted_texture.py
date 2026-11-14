"""Transfer the repaired head texture between two cylindrical height scales."""
import math
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
atlas=np.asarray(Image.open(here/'cgmodelsnet-bridge-slanted-atlas.png').convert('RGB')).copy()
previous=np.asarray(Image.open(here/'cgmodelsnet-atlas-imagegen-trial.png').convert('RGB'))
assert atlas.shape==previous.shape==(2048,2048,3)
# u is unchanged; convert the graft's new v to the prior cylindrical v.
ratio=(1.813-1.56)/(1.813-1.595)
row=np.clip(np.rint(np.arange(1024)*ratio).astype(int),0,1023)
atlas[1024:,:,:]=previous[1024+row,:,:]
texture=here/'cgmodelsnet-slanted-repaired-atlas.png';Image.fromarray(atlas).save(texture)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(needle)==1
        source=source.replace(needle,repr(f'cgmodelsnet-slanted-repaired-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(texture),str(angle),str(here/'cgmodelsnet-bridge-slanted-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
