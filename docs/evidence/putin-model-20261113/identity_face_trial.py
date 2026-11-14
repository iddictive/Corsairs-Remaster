"""Reproduce the stock-GM projection from the retained imagegen target."""
import sys
from pathlib import Path

here=Path(__file__).resolve().parent
def run(name,args,replacements):
    path=here/name;source=path.read_text()
    for old,new in replacements:
        assert source.count(old)==1,old
        source=source.replace(old,new)
    sys.argv=[str(path),*args]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

run('landmark_warp.py',['width-morph-baseline','identity-refinement-target'],[
    ('landmark-target.png','identity-aligned-target.png'),
    ('landmark-coverage.png','identity-aligned-coverage.png'),
])
run('nose_uv_trial.py',[],[
    ('photo-feathered-target.png','identity-aligned-target.png'),
    ('nose-face-','identity-face-'),
])
