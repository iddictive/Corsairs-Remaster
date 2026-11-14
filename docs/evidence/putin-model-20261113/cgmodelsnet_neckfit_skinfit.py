"""Combine the experimental neck taper with fitted body boundary weights."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neckfit_trial.py';source=path.read_text()
old="data=bytearray((here/'cgmodelsnet-graft-clipped-prototype.gm').read_bytes())"
new="data=bytearray((here/'cgmodelsnet-graft-skinfit-prototype.gm').read_bytes())"
assert source.count(old)==1
source=source.replace(old,new)
assert source.count('cgmodelsnet-neckfit')>=3
source=source.replace('cgmodelsnet-neckfit','cgmodelsnet-neckfit-skinfit')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
