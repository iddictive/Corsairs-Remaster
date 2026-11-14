"""Bridge the measured body/head loops on the shaped neck cut."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_bridge_slanted.py';source=path.read_text()
changes=[
    ('cgmodelsnet-graft-slanted-', 'cgmodelsnet-graft-shaped-'),
    ('cgmodelsnet-bridge-slanted', 'cgmodelsnet-bridge-shaped'),
    ("'split=2108'", "'split=2106'"),
    ('abs(pos[edges,1]+.25*(pos[edges,2]-.02)-plane)', 'abs(pos[edges,1]+.25*(pos[edges,2]-.02)-.30*abs(pos[edges,0]-.01)-plane)'),
]
for before,after in changes:
    assert source.count(before)>0,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
