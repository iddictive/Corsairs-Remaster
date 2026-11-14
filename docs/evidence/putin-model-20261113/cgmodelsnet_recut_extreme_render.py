"""Render the two worst declared man actions and musket death sample."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-recut-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-recut-declared-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('402',402),('6206',6206)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_recut_musket.py';source=path.read_text()
needle="'animated-cgmodelsnet-recut-musket-'";assert source.count(needle)==1
source=source.replace(needle,"'animated-cgmodelsnet-recut-musket-extreme-'")
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})"
assert source.count(needle)==1
source=source.replace(needle,"source=source.replace(\"[('idle',63),('shot',207),('run',318)]\",\"[('death',976),('guard',481),('attack',174)]\")\n"+needle)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
