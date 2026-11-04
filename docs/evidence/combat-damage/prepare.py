import json,sys
from pathlib import Path
sys.path.insert(0,'tools')
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates,source_rig
from prepare_port_rowboat import fk
import numpy as np
root=Path(__file__).resolve().parent
sources=json.loads(Path('src/assets/roundhouse/rigs.json').read_text())['rigs']
inputs=json.loads(Path('docs/evidence/roundhouse-kick/inputs/meshes.json').read_text())
for name in ['man','Danielle','Jessika','YokoDias','man1','man2']:
 if name not in inputs:continue
 source=sources[name]
 vertices,faces=animated_mesh(Path('docs/evidence/roundhouse-kick/inputs',inputs[name]['model']+'.gm').read_bytes())
 bound=bind_skin(source,vertices);rig=source_rig(source)
 p,q=fk(rig,source['bind_pose'],source['bind_root']);head=source['body']['joints'][2];parent=source['parents'][head]
 chest=source['body']['joints'][1]
 while source['parents'][head]!=chest and source['parents'][head]>=0:head=source['parents'][head]
 parent=source['parents'][head]
 axis=np.array(p[head])-p[parent];axis/=np.linalg.norm(axis);seam=np.array(p[head])
 guard=skin_coordinates(*fk(rig,source['guard_pose'],source['guard_root']),bound)
 moved=skin_coordinates(p,q,bound)
 with (root/(name+'.txt')).open('w') as out:
  out.write(str(len(faces))+'\n'+' '.join(map(str,[*axis,float(axis@seam)]))+'\n')
  for face in faces:
   for v in face:
    def desc(b):
     while b>=0:
      if b==head:return 1
      b=source['parents'][b]
     return 0
    _,weight,a,b=vertices[v];membership=weight*desc(a)+(1-weight)*desc(b)
    out.write(' '.join(map(str,[*vertices[v][0],*guard[v],*moved[v],membership]))+'\n')
 print(name,len(vertices),len(faces),'joint',head,'parent',parent,'cut',seam.tolist())
