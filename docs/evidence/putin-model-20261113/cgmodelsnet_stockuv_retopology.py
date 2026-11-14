"""Apply the bounded weights and retopology to the continuous body UV model."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_weight_fit_trial.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-refined-rebridge-prototype.gm'),('cgmodelsnet-neck-weight-fit-trial.gm','cgmodelsnet-stockuv-weight-fit.gm')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_neck_retopology_trial.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-stockuv-weight-fit.gm'),('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-stockuv-retopology.gm')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
