"""Render the radial neck-fit GM in stock animation poses."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_animated_shaped.py';source=path.read_text()
for old,new in [('cgmodelsnet-bridge-shaped-skinfit-prototype.gm','cgmodelsnet-bridge-shaped-radialfit-prototype.gm'),
                ('cgmodelsnet-shaped-skinfit-repaired-atlas.png','cgmodelsnet-radialfit-repaired-atlas.png'),
                ('animated-cgmodelsnet-shaped-','animated-cgmodelsnet-radialfit-')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
