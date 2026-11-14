"""Apply repaired head texture to the contour-fitted head."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_shaped_texture.py';source=path.read_text()
for before,after in [('cgmodelsnet-bridge-shaped-skinfit','cgmodelsnet-bridge-shaped-contourfit'),('cgmodelsnet-shaped-skinfit-repaired','cgmodelsnet-contourfit-repaired')]:
    assert source.count(before)==1,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
