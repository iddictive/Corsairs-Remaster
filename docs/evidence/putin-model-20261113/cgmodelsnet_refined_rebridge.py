"""Rebake the neck bridge after the head texture has been refined."""
from pathlib import Path
from PIL import Image
here=Path(__file__).resolve().parent
old=Image.open(here/'cgmodelsnet-graft-shaped-radialfit-atlas.png').convert('RGB')
refined=Image.open(here/'cgmodelsnet-face-refined-atlas.png').convert('RGB')
assert old.size==refined.size==(2048,2048)
old.paste(refined.crop((0,1024,2048,2048)),(0,1024))
old.save(here/'cgmodelsnet-graft-shaped-rebridge-atlas.png')
(here/'cgmodelsnet-graft-shaped-rebridge-prototype.gm').write_bytes((here/'cgmodelsnet-graft-shaped-radialfit-prototype.gm').read_bytes())
path=here/'cgmodelsnet_bridge_shaped_skinfit.py';source=path.read_text()
for old,new in [('cgmodelsnet-graft-shaped-skinfit-','cgmodelsnet-graft-shaped-rebridge-'),('cgmodelsnet-bridge-shaped-skinfit','cgmodelsnet-refined-rebridge')]:
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
