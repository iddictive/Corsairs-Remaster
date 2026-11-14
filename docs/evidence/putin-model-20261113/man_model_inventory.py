"""Render locally available, explicitly man/man NPC meshes for head review."""
import re
import sys
from pathlib import Path
from PIL import Image

here=Path(__file__).resolve().parent
root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh
from render_unarmed_atlas import panel

names=set()
for p in (root/'src/gameplay/PROGRAM').rglob('*.c'):
    names.update(re.findall(r'NPC_GenerateCharacter\s*\(\s*"[^"]*"\s*,\s*"([^"]+)"\s*,\s*"man"\s*,\s*"man"',p.read_text(errors='ignore'),re.I))
folder=root/'experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Characters'
paths={p.stem.lower():p for p in folder.glob('*.gm')}
selected=sorted((paths[n.lower()] for n in names if n.lower() in paths),key=lambda p:p.stem.lower())
excluded=('barmen_', 'officer_')
selected=[p for p in selected if not p.stem.lower().startswith(excluded)]
print('candidate meshes',len(selected),flush=True)
for batch in range((len(selected)+23)//24):
    chunk=selected[batch*24:(batch+1)*24]
    canvas=Image.new('RGB',(180*6,220*4))
    for i,path in enumerate(chunk):
        try:
            vertices,faces=animated_mesh(path.read_bytes())
            canvas.paste(panel([v[0] for v in vertices],faces,0,(180,220),path.stem,(0,1.45,440)),((i%6)*180,(i//6)*220))
        except (OSError,ValueError,IndexError,AssertionError) as e:
            print(path.stem,type(e).__name__,str(e),flush=True)
    path=here/f'man-inventory-{batch+1}.png'
    canvas.save(path)
    print('saved',path.name,flush=True)
