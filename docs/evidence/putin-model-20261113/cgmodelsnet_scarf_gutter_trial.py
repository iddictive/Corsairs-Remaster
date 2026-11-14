"""Try larger UV tiles and gutters while retaining the corrected face atlas."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_scarf_worlduv_trial.py';source=path.read_text()
needle='source = path.read_text()'
if source.count(needle)!=1:
    needle='source=path.read_text()'
assert source.count(needle)==1
injection="""
for old,new in [('tile=48;pad=4;side=tile-2*pad;cols=21','tile=72;pad=12;side=tile-2*pad;cols=14'),('cgmodelsnet-scarf-clean-atlas.png','cgmodelsnet-nose-baseline-atlas.png')]:
    assert source.count(old)==1
    source=source.replace(old,new)
"""
source=source.replace(needle,needle+injection)
assert source.count('cgmodelsnet-body-scarf-worlduv')==1
source=source.replace('cgmodelsnet-body-scarf-worlduv','cgmodelsnet-body-scarf-gutter')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
