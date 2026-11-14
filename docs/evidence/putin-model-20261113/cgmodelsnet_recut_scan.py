"""Measure the new bridge on the same man-bank sample as the candidate."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_animation_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-recut-trial.gm'),('faces[-628:]','faces[-344:]'),('bridge.max()<15623','bridge.max()<16271'),('cgmodelsnet-neck-animation-scan.json','cgmodelsnet-recut-scan.json')]:
    assert source.count(old)==1;source=source.replace(old,new)
needle='rows=[]';assert source.count(needle)==1
source=source.replace(needle,"labels=(indices[edges]>=16012)&(indices[edges]!=16270)\nedges=edges[np.sum(labels,axis=1)==1]\nprint('body-head edges',len(edges))\nrows=[]")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
