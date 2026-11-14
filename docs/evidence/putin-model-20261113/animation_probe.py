"""Check the stock man bind and guard poses without running the game."""
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
root = here.parents[2]
sys.path.insert(0, str(root/'tools'))
from prepare_roundhouse import animated_mesh, bind_skin, skin_coordinates, source_rig
from prepare_port_rowboat import fk
from render_unarmed_atlas import panel

source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
candidate=Path(sys.argv[1]) if len(sys.argv)>1 else here/'morph-prototype.gm'
paths=[root/'src/assets/characters/putin-phonk/original.gm',candidate]
meshes=[animated_mesh(p.read_bytes()) for p in paths]
assert meshes[0][1] == meshes[1][1]
assert [x[1:] for x in meshes[0][0]] == [x[1:] for x in meshes[1][0]]
canvas=Image.new('RGB',(640,720))
for row,(label,(vertices,faces)) in enumerate(zip(('original','morph'),meshes)):
    bound=bind_skin(source,vertices); rig=source_rig(source)
    for col,(name,pose,origin) in enumerate((('bind',source['bind_pose'],source['bind_root']),('guard',source['guard_pose'],source['guard_root']))):
        p,q=fk(rig,pose,origin); points=skin_coordinates(p,q,bound)
        assert np.isfinite(np.asarray(points)).all()
        canvas.paste(panel(points,faces,-50,(320,360),label+' '+name),(col*320,row*360))
canvas.save(here/(candidate.stem+'-animation.png'))
print('bind and guard finite; topology and packed skin influences identical')
