"""Retain more source geometry to test the reducer's visual error."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_trial.py';source=path.read_text()
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})"
assert source.count(needle)==1
source=source.replace('cgmodelsnet-head-edge-collapse.STL','cgmodelsnet-head-edge-collapse-100k.STL')
inject="""assert source.count('remaining<=25000')==2
source=source.replace('remaining<=25000','remaining<=100000')
"""+needle
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
