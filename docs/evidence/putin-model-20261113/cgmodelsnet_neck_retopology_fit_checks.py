"""Repeat the contour scan and pose renders for fitted weights."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_retopology_checks.py';source=path.read_text()
for old,new in [('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-neck-retopology-fit.gm'),('cgmodelsnet-neck-retopology-scan.json','cgmodelsnet-neck-retopology-fit-scan.json'),('animated-cgmodelsnet-neck-retopology-','animated-cgmodelsnet-neck-retopology-fit-')]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
