"""Apply only the generated head half to the existing diagnostic atlas."""
import math
import sys
from pathlib import Path
from PIL import Image

here=Path(__file__).resolve().parent
atlas=Image.open(here/'cgmodelsnet-neckfit-atlas.png').convert('RGB')
assert atlas.size==(2048,2048)
generated=Image.open(here/'cgmodelsnet-atlas-imagegen-target.png').convert('RGB').resize(atlas.size,Image.Resampling.LANCZOS)
atlas.paste(generated.crop((0,1024,2048,2048)),(0,1024))
texture=here/'cgmodelsnet-atlas-imagegen-trial.png';atlas.save(texture)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    source=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old)==1
        source=source.replace(old,repr('cgmodelsnet-atlas-imagegen-'+label+'.png'))
    sys.argv=[str(renderer),'officer_5',str(texture),str(angle),str(here/'cgmodelsnet-neckfit-prototype.gm')]
    exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
