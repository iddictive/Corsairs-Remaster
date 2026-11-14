"""Run the existing conservative reducer on the corrected recut head only."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_head_decimate_trial.py';source=path.read_text()
old="""head = np.all((faces>=2106)&(faces<14993),axis=1)
assert head.sum()==25000 and not np.any((~head)&np.any((faces>=2106)&(faces<14993),axis=1))"""
new="""head=np.zeros(len(faces),dtype=bool);head[-24544:-344]=True
assert head.sum()==24200
headpool=np.r_[np.arange(2106,14993),np.arange(15755,16012)]
assert not np.any(np.isin(faces[~head],headpool))"""
assert source.count(old)==1;source=source.replace(old,new)
source=source.replace('cgmodelsnet-stockuv-retopology.gm','cgmodelsnet-recut-winding-trial.gm')
source=source.replace('len(faces)-25000+remaining','len(faces)-24200+remaining')
old='keep=np.concatenate((np.arange(2106),used[(used>=2106)&(used<14993)],np.arange(14993,len(rows))))'
new='keep=np.union1d(np.flatnonzero(~np.isin(np.arange(len(rows)),headpool)),np.intersect1d(used,headpool))'
assert source.count(old)==1;source=source.replace(old,new)
source=source.replace('cgmodelsnet-head-decimated-trial.gm','cgmodelsnet-recut-decimated-trial.gm')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
