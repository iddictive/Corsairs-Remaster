"""Render the refined actual GM in three stock animation poses."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_animated_radialfit.py';source=path.read_text()
for before,after in [('cgmodelsnet-radialfit-repaired-atlas.png','cgmodelsnet-face-refined-atlas.png'),('animated-cgmodelsnet-radialfit-','animated-cgmodelsnet-face-refined-')]:
    assert source.count(before)==1,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
