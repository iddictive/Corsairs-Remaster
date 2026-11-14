"""Fit the clipped stock-body weights against sampled man animations."""
import os
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_graft_clipped.py';source=path.read_text()
assert source.count('cgmodelsnet-graft-clipped-')==1
source=source.replace('cgmodelsnet-graft-clipped-','cgmodelsnet-graft-skinfit-')
os.environ['PUTIN_TRIAL_SKIN_FIT']='1'
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
