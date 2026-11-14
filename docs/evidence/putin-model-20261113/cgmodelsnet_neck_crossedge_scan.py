"""Measure only body-to-head bridge edges, excluding the two boundary loops."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_animation_scan.py';original=path.read_text()
needle='rows=[]'
assert original.count(needle)==1
inject="""labels=indices[edges]>=15016
edges=edges[np.sum(labels,axis=1)==1]
assert len(edges)==629
print('body-head edges',len(edges))
rows=[]"""
for model,label in [('cgmodelsnet-body-scarf-worlduv.gm','original'),('cgmodelsnet-neck-weight-fit-trial.gm','fitted'),('cgmodelsnet-neck-weight-trial.gm','rigid')]:
    source=original.replace(needle,inject)
    assert source.count('cgmodelsnet-body-scarf-worlduv.gm')==1
    source=source.replace('cgmodelsnet-body-scarf-worlduv.gm',model)
    source=source.replace('cgmodelsnet-neck-animation-scan.json',f'cgmodelsnet-neck-crossedge-{label}.json')
    print('variant',label)
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
