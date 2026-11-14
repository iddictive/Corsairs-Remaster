"""Measure how much the lower-head taper moves compared with the candidate."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_weight_delta_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-stockuv-retopology.gm'),('cgmodelsnet-neck-weight-trial.gm','cgmodelsnet-head-weight-taper-trial.gm'),('cgmodelsnet-neck-weight-delta.json','cgmodelsnet-head-taper-delta.json')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
