"""Attach the stock musket to the packaged recut musket GM for offline QA."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_musket_overlay.py';source=path.read_text()
changes=[("here/'putin-mush-locator-probe.gm'","root/'src/assets/characters/putin-phonk-recut-candidate/Putin_mush.gm'"),('musket-overlay-diagnostic','recut-musket-overlay-diagnostic')]
for old,new in changes:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_overlay_render.py';source=path.read_text()
changes=[('musket-overlay-diagnostic','recut-musket-overlay-diagnostic'),('animated-cgmodelsnet-musket-overlay-','animated-cgmodelsnet-recut-musket-overlay-'),('cgmodelsnet-musket-overlay-fullbody-','cgmodelsnet-recut-musket-overlay-fullbody-')]
for old,new in changes:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
