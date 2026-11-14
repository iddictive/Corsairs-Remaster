"""Render the contour-fitted graft in three stock animation poses."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_animated_shaped.py';source=path.read_text()
for before,after in [('cgmodelsnet-bridge-shaped-skinfit-prototype.gm','cgmodelsnet-bridge-shaped-contourfit-prototype.gm'),
                     ('cgmodelsnet-shaped-skinfit-repaired-atlas.png','cgmodelsnet-contourfit-repaired-atlas.png'),
                     ('animated-cgmodelsnet-shaped-', 'animated-cgmodelsnet-contourfit-')]:
    assert source.count(before)==1,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
