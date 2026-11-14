"""Retain facial features while reducing the surrounding bust geometry."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_trial.py';source=path.read_text()
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})"
assert source.count(needle)==1
assert source.count('cgmodelsnet-head-edge-collapse.STL')==1
source=source.replace('cgmodelsnet-head-edge-collapse.STL','cgmodelsnet-head-protected-100k.STL')
inject="""assert source.count('remaining<=25000')==2
source=source.replace('remaining<=25000','remaining<=100000')
needle='frozen[np.unique(edges[degree==1])]=True'
assert source.count(needle)==1
source=source.replace(needle,needle+\"\\nfrozen |= (vertices[:,0]>12)&(vertices[:,0]<25)&(vertices[:,2]>28)&(vertices[:,2]<39)&(vertices[:,1]<9)\")
"""+needle
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
