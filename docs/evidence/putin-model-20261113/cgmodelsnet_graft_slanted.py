"""Clip the stock body and the bust on a shared oblique plane."""
import os
import struct
from pathlib import Path
import numpy as np
from clip_body_trial import clip_body

here=Path(__file__).resolve().parent
def clip_body_slanted(rows,faces,points,plane):
    transformed=np.asarray(points,dtype=float).copy()
    transformed[:,1]+=.25*(transformed[:,2]-.02)
    adjusted=[]
    for row in rows:
        r=bytearray(row);x,y,z=struct.unpack_from('<3f',r,0)
        struct.pack_into('<3f',r,0,x,y+.25*(z-.02),z);adjusted.append(r)
    adjusted,kept=clip_body(adjusted,faces,transformed,plane)
    adjusted=[bytearray(row) for row in adjusted]
    for r in adjusted:
        x,y,z=struct.unpack_from('<3f',r,0)
        struct.pack_into('<3f',r,0,x,y-.25*(z-.02),z)
    return adjusted,kept

path=here/'cgmodelsnet_graft_trial.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-bounded.STL','cgmodelsnet-head-slanted.STL'),
                ('source[:,:,2]>=25.','source[:,:,2]>=-1000.'),
                ('cgmodelsnet-graft-','cgmodelsnet-graft-slanted-')]:
    assert source.count(old)>0,old
    source=source.replace(old,new)
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})"
assert source.count(needle)==1
inject="""old='from clip_body_trial import clip_body'
new='from __main__ import clip_body_slanted as clip_body'
assert source.count(old)==1
source=source.replace(old,new)
assert source.count('(1.813-1.6174)')==1
source=source.replace('(1.813-1.6174)','(1.813-1.56)')
"""+needle
source=source.replace(needle,inject)
os.environ['PUTIN_TRIAL_BODY_CLIP']='1';os.environ['PUTIN_TRIAL_BODY_PLANE']='1.595'
os.environ.pop('PUTIN_TRIAL_SKIN_FIT',None)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
