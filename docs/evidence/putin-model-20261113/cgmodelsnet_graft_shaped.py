"""Apply the same shaped neck cut to the stock player body."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_graft_slanted.py';source=path.read_text()
changes=[
    ("transformed[:,1]+=.25*(transformed[:,2]-.02)","transformed[:,1]+=.25*(transformed[:,2]-.02)-.30*abs(transformed[:,0]-.01)"),
    ("y+.25*(z-.02),z", "y+.25*(z-.02)-.30*abs(x-.01),z"),
    ("y-.25*(z-.02),z", "y-.25*(z-.02)+.30*abs(x-.01),z"),
    ('cgmodelsnet-head-slanted.STL','cgmodelsnet-head-shaped.STL'),
    ('cgmodelsnet-graft-slanted-','cgmodelsnet-graft-shaped-'),
]
for before,after in changes:
    assert source.count(before)==1,(before,source.count(before))
    source=source.replace(before,after)
needle="path=here/'cgmodelsnet_graft_trial.py'"
assert source.count(needle)==1
source=source.replace(needle,"import __main__; __main__.clip_body_slanted=clip_body_slanted\n"+needle)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
