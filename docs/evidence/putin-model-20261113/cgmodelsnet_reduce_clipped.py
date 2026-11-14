"""Clip the source head exactly at z=20, then reduce with bounded edges."""
import ast
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
clip_path=here/'exact_neck_cut_trial.py';tree=ast.parse(clip_path.read_text())
fn=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='clip_mesh')
module=ast.Module(body=[fn],type_ignores=[]);scope={'np':np};exec(compile(module,str(clip_path),'exec'),scope)
clip_mesh=scope['clip_mesh']
path=here/'cgmodelsnet_reduce_trial.py';source=path.read_text()
before='tri=input_triangles[np.all(input_triangles[:,:,2]>=25.,axis=1)]'
assert source.count(before)==1
source=source.replace(before,'tri=clip_mesh(input_triangles,20.)')
assert source.count('cgmodelsnet-head-edge-collapse.STL')==1
source=source.replace('cgmodelsnet-head-edge-collapse.STL','cgmodelsnet-head-clipped-bounded.STL')
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})"
assert source.count(needle)==1
new_exec="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles,'clip_mesh':clip_mesh})"
inject="""old='if np.any(norm<1e-12) or np.any(dot<.25*norm):continue'
new='''if np.any(norm<1e-12) or np.any(dot<.85*norm):continue
            lengths=np.stack([np.linalg.norm(after[:,a]-after[:,b],axis=1) for a,b in ((0,1),(1,2),(2,0))])
            if np.any(lengths>.8):continue'''
assert source.count(old)==1
source=source.replace(old,new)
"""+new_exec
source=source.replace(needle,inject)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','clip_mesh':clip_mesh})
