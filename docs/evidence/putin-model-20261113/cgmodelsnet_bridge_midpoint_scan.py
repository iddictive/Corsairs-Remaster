"""Measure the subdivided bridge over the same animation sample."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_animation_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-bridge-midpoint-trial.gm'),('faces[-628:]','faces[-1884:]'),('bridge.max()<15623','bridge.max()<16480'),('cgmodelsnet-neck-animation-scan.json','cgmodelsnet-bridge-midpoint-scan.json')]:
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
