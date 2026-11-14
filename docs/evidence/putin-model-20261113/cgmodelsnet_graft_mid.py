"""Test a higher matching body/head plane on the recognizable bust."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_graft_clipped.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-clipped-bounded.STL','cgmodelsnet-head-clipped-mid.STL'),
                ('cgmodelsnet-graft-clipped-', 'cgmodelsnet-graft-mid-'),
                ("'1.59349'", "'1.6101581'")]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
needle="source=path.read_text()"
assert source.count(needle)==1
source=source.replace(needle,needle+"\nassert source.count('source[:,:,2]>=20.')==1\nsource=source.replace('source[:,:,2]>=20.','source[:,:,2]>=22.2999')")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
