"""Apply the same contour retopology to the bounded fitted-weight candidate."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_retopology_trial.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-neck-weight-fit-trial.gm'),('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-neck-retopology-fit.gm')]:
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
