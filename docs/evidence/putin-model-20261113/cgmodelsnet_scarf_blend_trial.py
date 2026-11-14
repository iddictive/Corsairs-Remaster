"""Keep the repaired upper head and retain the prior lower scarf texture."""
import math
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
original=np.asarray(Image.open(here/'cgmodelsnet-neckfit-atlas.png').convert('RGB'),dtype=float)
fixed=np.asarray(Image.open(here/'cgmodelsnet-atlas-imagegen-target.png').convert('RGB').resize((2048,2048),Image.Resampling.LANCZOS),dtype=float)
assert original.shape==fixed.shape==(2048,2048,3)
# Keep original top and progressively restore the original scarf in the lower head.
y=np.arange(2048);t=np.clip((y-1700)/220,0,1);t=t*t*(3-2*t)
weight=np.where(y<1024,0,1-t)[:,None,None]
out=np.clip(original*(1-weight)+fixed*weight,0,255).astype('uint8')
texture=here/'cgmodelsnet-scarf-blend-atlas.png';Image.fromarray(out).save(texture)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr('cgmodelsnet-scarf-blend-'+label+'.png'))
    sys.argv=[str(renderer),'officer_5',str(texture),str(angle),str(here/'cgmodelsnet-neckfit-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
