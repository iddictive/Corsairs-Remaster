"""Render and scan the stock-UV model after skin and contour changes."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_retopology_checks.py';source=path.read_text()
for old,new in [('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-stockuv-retopology.gm'),('cgmodelsnet-neck-retopology-scan.json','cgmodelsnet-stockuv-retopology-scan.json'),('animated-cgmodelsnet-neck-retopology-','animated-cgmodelsnet-stockuv-retopology-'),('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
