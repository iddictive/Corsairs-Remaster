"""Include nose-side polygons in the stock-head photo projection trial."""
import sys
from pathlib import Path

here=Path(__file__).resolve().parent
path=here/'full_face_projection_trial.py'
source=path.read_text()
replacements=[
    ('aligned-width-target.png','photo-feathered-target.png',1),
    ('central = 200 < center[0] < 310 and 145 < center[1] < 320',
     'central = 200 < center[0] < 310 and 145 < center[1] < 320\n    nose = 225 < center[0] < 275 and 205 < center[1] < 255',1),
    ('if not central and np.any(normals@np.array([-s,0,c])<.55):continue',
     'if (central and not nose and np.mean(normals@np.array([-s,0,c]))<.25) or (not central and np.any(normals@np.array([-s,0,c])<.55)):continue',1),
    ('full-face-','nose-face-',8),
]
for old,new,count in replacements:
    assert source.count(old)==count,old
    source=source.replace(old,new)
sys.argv=[str(path)]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
