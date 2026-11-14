"""Offline masked photo projection on the no-earring GM; no game execution."""
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

here=Path(__file__).resolve().parent
def run(name,args,replacements):
    path=here/name
    source=path.read_text()
    for old,new,count in replacements:
        assert source.count(old)==count,old
        source=source.replace(old,new)
    sys.argv=[str(path),*map(str,args)]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

run('landmark_warp.py',['width-morph-baseline','reference-putin-2024'],[
    ('landmark-target.png','real-reference-target.png',1),
    ('landmark-coverage.png','real-reference-coverage.png',1),
])
photo=np.asarray(Image.open(here/'real-reference-target.png').convert('RGB'),dtype=float)
base=np.asarray(Image.open(here/'width-morph-baseline.png').convert('RGB'),dtype=float)
mask=Image.open(here/'real-reference-coverage.png').convert('L').filter(ImageFilter.MinFilter(11)).filter(ImageFilter.GaussianBlur(7))
weight=np.asarray(mask,dtype=float)[...,None]/255
Image.fromarray(np.clip(base+(photo-base)*weight,0,255).astype('uint8')).save(here/'photo-feathered-target.png')
run('full_face_projection_trial.py',[],[
    ('aligned-width-target.png','photo-feathered-target.png',1),
    ('if not central and np.any(normals@np.array([-s,0,c])<.55):continue','if (central and np.mean(normals@np.array([-s,0,c]))<.25) or (not central and np.any(normals@np.array([-s,0,c])<.55)):continue',1),
    ('full-face-','photo-face-',8),
])
