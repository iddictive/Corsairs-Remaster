"""Apply the existing repaired cylindrical texture to the shaped graft."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_slanted_texture.py';source=path.read_text()
for before,after in [('cgmodelsnet-bridge-slanted','cgmodelsnet-bridge-shaped-skinfit'),('cgmodelsnet-slanted-repaired','cgmodelsnet-shaped-skinfit-repaired')]:
    assert source.count(before)>0
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
