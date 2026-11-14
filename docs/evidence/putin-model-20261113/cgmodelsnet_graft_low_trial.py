"""Graft the lower-cut reduced bust, retaining the whole chin."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_graft_trial.py';source=path.read_text()
for old,new in [
    ('cgmodelsnet-head-bounded.STL','cgmodelsnet-head-bounded-low.STL'),
    ('source[:,:,2]>=25.', 'source[:,:,2]>=20.'),
    ('cgmodelsnet-graft-', 'cgmodelsnet-graft-low-')]:
    assert source.count(old)>0,old
    source=source.replace(old,new)
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})"
assert source.count(needle)==1
inject="""for old,new in [('centers[:,1]>1.6174','centers[:,1]>1.595'),('(1.813-1.6174)','(1.813-1.595)')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
"""+needle
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
