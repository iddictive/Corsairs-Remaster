"""Render the new GM against the candidate's actual encoded texture mips."""
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
path=here/'cgmodelsnet_tx_mip_render_probe.py';source=path.read_text()
old="texture=(here/'putin-format-probe.tga.tx').read_bytes()"
new="texture=(root/'src/assets/characters/putin-phonk-candidate/Putin.tga.tx').read_bytes()"
assert source.count(old)==1;source=source.replace(old,new)
source=source.replace("here=Path(__file__).resolve().parent", "here=Path(__file__).resolve().parent;root=here.parents[2]")
source=source.replace('putin-format-mip-','putin-recut-mip-')
needle="path=here/'cgmodelsnet_tx_render_probe.py';source=path.read_text()"
assert source.count(needle)==1
source=source.replace(needle,needle+"\n    source=source.replace('putin-format-probe.gm','cgmodelsnet-recut-trial.gm')")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
