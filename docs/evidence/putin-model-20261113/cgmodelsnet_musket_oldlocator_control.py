"""Negative control: attach the same musket to the original pistol locator."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_musket_overlay.py';source=path.read_text()
for old,new in [('putin-mush-locator-probe.gm','putin-stockuv-retopology-probe.gm'),("label=labels['gun_hand'];assert label[19]==65","label=labels['gun_hand'];gunbone=label[19];assert gunbone==66"),('1.,65|(65<<8),*n','1.,gunbone|(gunbone<<8),*n'),('musket-overlay-diagnostic','musket-oldlocator-control')]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_overlay_render.py';source=path.read_text()
for old,new in [('musket-overlay-diagnostic','musket-oldlocator-control'),('animated-cgmodelsnet-musket-overlay-','animated-cgmodelsnet-musket-oldlocator-'),('cgmodelsnet-musket-overlay-fullbody-','cgmodelsnet-musket-oldlocator-fullbody-')]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
