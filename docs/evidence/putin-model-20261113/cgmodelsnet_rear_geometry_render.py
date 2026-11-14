"""Render the rear-collar geometry experiment in four sampled poses."""
import argparse
from pathlib import Path
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('strength',choices=['3','6']);args=parser.parse_args()
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm',f'cgmodelsnet-rear-geometry-{args.strength}.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-',f'animated-cgmodelsnet-rear-geometry-{args.strength}-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('run',163),('5320',5320),('7236',7236),('8423',8423)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
