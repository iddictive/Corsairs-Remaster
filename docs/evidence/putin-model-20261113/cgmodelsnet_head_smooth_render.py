"""Render the continuous field in ordinary and extreme sampled poses."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_head_taper_render.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-weight-taper-trial.gm','cgmodelsnet-head-weight-smooth-trial.gm'),('animated-cgmodelsnet-head-taper-','animated-cgmodelsnet-head-smooth-'),("[('run',163),('612',612),('5320',5320),('7236',7236)]","[('run',163),('612',612),('5320',5320),('7236',7236),('8423',8423)]")]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
