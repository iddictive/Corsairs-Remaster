"""Keep original stock scarf faces while clipping the graft body."""
import os
import sys
import types
from pathlib import Path
from PIL import Image
import numpy as np

here=Path(__file__).resolve().parent
clip=here/'clip_body_trial.py';source=clip.read_text()
needle='    result=[]; cache={}; removed=0; crossing=0; max_discarded=0.'
insert=needle+"""
    uv=np.array([struct.unpack_from('<2f',r,36) for r in rows])*2
    uv=(uv[faces].mean(axis=1)%1)
    image=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024)),dtype=int)
    col=image[np.clip((uv[:,1]*1024).astype(int),0,1023),np.clip((uv[:,0]*1024).astype(int),0,1023)]
    center=points[faces].mean(axis=1)
    scarf_flags=(center[:,1]>plane)&(center[:,1]<1.67)&(col[:,2]>col[:,0]*1.1)&(col.max(axis=1)<120)
    print('stock scarf faces retained above plane',int(scarf_flags.sum()))
"""
for old,new in [(needle,insert),('for face in faces:', 'for fid,face in enumerate(faces):'),
                ('if not local or np.all(xyz[:,1]<=plane):', 'if scarf_flags[fid] or not local or np.all(xyz[:,1]<=plane):')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
module=types.ModuleType('clip_body_trial');module.__dict__.update({'Image':Image,'here':here})
exec(compile(source,str(clip),'exec'),module.__dict__);sys.modules['clip_body_trial']=module
os.environ['PUTIN_TRIAL_SKIN_FIT']='1'
path=here/'cgmodelsnet_graft_clipped.py';source=path.read_text()
assert source.count('cgmodelsnet-graft-clipped-')==1
source=source.replace('cgmodelsnet-graft-clipped-','cgmodelsnet-graft-restored-scarf-')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
