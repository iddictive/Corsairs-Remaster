"""Trial a half-strength fitted body transition and run the same checks."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_body_fit_trial.py';source=path.read_text()
needle="path=here/'cgmodelsnet_neck_body_follow_trial.py';source=path.read_text()"
assert source.count(needle)==1
source=source.replace(needle,needle+"\nassert source.count('t=t*t*(3-2*t)')==1;source=source.replace('t=t*t*(3-2*t)','t=.5*t*t*(3-2*t)')")
source=source.replace('cgmodelsnet-neck-body-fit-trial.gm','cgmodelsnet-neck-body-fit-half-trial.gm')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_neck_body_fit_checks.py';source=path.read_text()
needle="source=source.replace('cgmodelsnet-neck-body-follow-','cgmodelsnet-neck-body-fit-')"
assert source.count(needle)==1
source=source.replace(needle,"source=source.replace('cgmodelsnet-neck-body-follow-','cgmodelsnet-neck-body-fit-half-')")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
