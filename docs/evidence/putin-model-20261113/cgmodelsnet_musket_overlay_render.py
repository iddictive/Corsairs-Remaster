"""Render the diagnostic mesh at idle, shot and run, then full-body."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_musket_activebind_probe.py';source=path.read_text()
source=source[:source.index("path=here/'cgmodelsnet_neck_animation_scan.py'")]
for old,new in [('cgmodelsnet-stockuv-retopology.gm','musket-overlay-diagnostic.gm'),('cgmodelsnet-stockuv-scarf-atlas.png','musket-overlay-diagnostic-atlas.png'),('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-musket-overlay-')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_fullbody.py';source=path.read_text()
for old,new in [('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-musket-overlay-'),('cgmodelsnet-stockuv-scarf-atlas.png','musket-overlay-diagnostic-atlas.png'),('cgmodelsnet-musket-fullbody-','cgmodelsnet-musket-overlay-fullbody-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
