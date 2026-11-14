"""Match experimental head/body cut heights before testing a seam."""
import os
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_graft_low_trial.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-bounded-low.STL','cgmodelsnet-head-clipped-bounded.STL'),('cgmodelsnet-graft-low-','cgmodelsnet-graft-clipped-')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
os.environ['PUTIN_TRIAL_BODY_CLIP']='1'
os.environ['PUTIN_TRIAL_BODY_PLANE']='1.59349'
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
