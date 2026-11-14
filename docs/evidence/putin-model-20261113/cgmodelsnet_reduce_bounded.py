"""Limit accumulated edge growth and angular change in each local collapse."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_trial.py';source=path.read_text()
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})"
assert source.count(needle)==1
source=source.replace('cgmodelsnet-head-edge-collapse.STL','cgmodelsnet-head-bounded.STL')
inject="""old='if np.any(norm<1e-12) or np.any(dot<.25*norm):continue'
new='''if np.any(norm<1e-12) or np.any(dot<.85*norm):continue
            lengths=np.stack([np.linalg.norm(after[:,a]-after[:,b],axis=1) for a,b in ((0,1),(1,2),(2,0))])
            if np.any(lengths>.8):continue'''
assert source.count(old)==1
source=source.replace(old,new)
"""+needle
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
