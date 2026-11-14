"""Offline candidate-versus-stock skinning samples on the musket animation."""
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
root=here.parents[2];sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk
from render_unarmed_atlas import panel

source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
resource=root/'experiments/native-metal/.cache/runtime/RESOURCE'
animation=rig((resource/'animation/mushketer.an').read_bytes())
assert animation['parents']==source['parents']
meshes=[('stock musket',resource/'MODELS/Characters/Blaze_8_Mush.gm'),('candidate',here/'identity-geometry-face-prototype.gm')]
frames=[('idle',63),('shot',207),('run',318)]
canvas=Image.new('RGB',(960,720))
for row,(label,path) in enumerate(meshes):
    verts,faces=animated_mesh(path.read_bytes());bound=bind_skin(source,verts)
    for col,(name,frame) in enumerate(frames):
        p,q=fk(animation,animation['pose'](frame),animation['roots'][frame])
        points=np.asarray(skin_coordinates(p,q,bound));assert np.isfinite(points).all()
        canvas.paste(panel(points,faces,-50,(320,360),label+' '+name),(col*320,row*360))
canvas.save(here/'musket-pose-comparison.png')
print('finite positions: stock and candidate, idle/shot/run; no weapon or engine tested')
