"""Bridge the shaped graft with fitted body-boundary skin weights."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_bridge_shaped.py';source=path.read_text()
for old,new in [('cgmodelsnet-graft-shaped-','cgmodelsnet-graft-shaped-skinfit-'),
                ('cgmodelsnet-bridge-shaped','cgmodelsnet-bridge-shaped-skinfit')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
