"""Measure and render the bounded fitted-weight trial."""
from pathlib import Path
here=Path(__file__).resolve().parent
jobs=[('cgmodelsnet_neck_weight_scan.py',[('cgmodelsnet-neck-weight-trial.gm','cgmodelsnet-neck-weight-fit-trial.gm'),('cgmodelsnet-neck-weight-scan.json','cgmodelsnet-neck-weight-fit-scan.json')]),('cgmodelsnet_neck_weight_delta_scan.py',[('cgmodelsnet-neck-weight-trial.gm','cgmodelsnet-neck-weight-fit-trial.gm'),('cgmodelsnet-neck-weight-delta.json','cgmodelsnet-neck-weight-fit-delta.json')]),('cgmodelsnet_animated_neck_weight.py',[('cgmodelsnet-neck-weight-trial.gm','cgmodelsnet-neck-weight-fit-trial.gm'),('animated-cgmodelsnet-neck-weight-','animated-cgmodelsnet-neck-weight-fit-')])]
for name,changes in jobs:
    path=here/name;source=path.read_text()
    for old,new in changes:
        assert source.count(old)>=1,(name,old)
        source=source.replace(old,new)
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
