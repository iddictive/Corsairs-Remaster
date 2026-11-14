"""Run the same stock-animation bridge scan on the weight experiment."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_animation_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-neck-weight-trial.gm'),('cgmodelsnet-neck-animation-scan.json','cgmodelsnet-neck-weight-scan.json')]:
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
