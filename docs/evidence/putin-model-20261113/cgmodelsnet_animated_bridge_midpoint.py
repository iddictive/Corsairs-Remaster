"""Render the bridge subdivision in ordinary and extreme poses."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-bridge-midpoint-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-nose-baseline-atlas.png'),('animated-face-','animated-cgmodelsnet-bridge-midpoint-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('walk',10),('run',163),('fight-stand',8532),('8423',8423),('410',410),('1985',1985)]")]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
