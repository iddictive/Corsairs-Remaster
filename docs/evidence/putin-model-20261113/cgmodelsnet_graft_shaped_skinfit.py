"""Fit physical-space stock boundary skinning on the shaped neck cut."""
import os
import sys
import types
from pathlib import Path

here=Path(__file__).resolve().parent
path=here/'clip_body_trial.py';source=path.read_text()
old="pair,weight=fitter.fit(a,b,t,pos,((keep[0][0],keep[1][0]),weight))"
new="""def physical(row):
                result=list(row)
                result[1]=row[1]-.25*(row[2]-.02)+.30*abs(row[0]-.01)
                return result
            pair,weight=fitter.fit(physical(a),physical(b),t,physical(pos),((keep[0][0],keep[1][0]),weight))"""
assert source.count(old)==1
source=source.replace(old,new)
module=types.ModuleType('clip_body_trial');exec(compile(source,str(path),'exec'),module.__dict__);sys.modules['clip_body_trial']=module
path=here/'cgmodelsnet_graft_shaped.py';source=path.read_text()
assert source.count('cgmodelsnet-graft-shaped-')==1
source=source.replace('cgmodelsnet-graft-shaped-','cgmodelsnet-graft-shaped-skinfit-')
needle="source=path.read_text()";assert source.count(needle)==1
inject=needle+"\nassert source.count(\"os.environ.pop('PUTIN_TRIAL_SKIN_FIT',None)\")==1\nsource=source.replace(\"os.environ.pop('PUTIN_TRIAL_SKIN_FIT',None)\",\"os.environ['PUTIN_TRIAL_SKIN_FIT']='1'\")"
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
