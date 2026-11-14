"""Evaluate a larger, still bounded deformation from the licensed bust."""
from pathlib import Path

here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_fit_trial.py';source=path.read_text()
assert source.count('cgmodelsnet-fit-')==5
source=source.replace('cgmodelsnet-fit-','cgmodelsnet-strong-fit-')
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','points':points})"
assert source.count(needle)==1
injected="""for old, new in [('response-offset,-.012,.012','response-offset,-.020,.020'),
                 ('np.clip(.6*f*fade,-.006,.006)','np.clip(1.8*f*fade,-.018,.018)'),
                 ('change.max()<=.00601','change.max()<=.01801')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
"""+needle
source=source.replace(needle,injected)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
