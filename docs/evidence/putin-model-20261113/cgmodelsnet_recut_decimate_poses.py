"""Render representative poses of the reduced recut mesh."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_recut_render.py';source=path.read_text()
for old,new in [('cgmodelsnet-recut-trial.gm','cgmodelsnet-recut-decimated-trial.gm'),('animated-cgmodelsnet-recut-','animated-cgmodelsnet-recut-decimated-'),('cgmodelsnet-recut-{label}-{side}.png','cgmodelsnet-recut-decimated-{label}-{side}.png')]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
