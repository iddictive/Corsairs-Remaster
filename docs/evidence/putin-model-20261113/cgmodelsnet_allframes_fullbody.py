"""Render worst-frame candidate and stock with the same full-body camera."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_fullbody_trial.py';source=path.read_text()
start=source.index('cases=');end=source.index('\nfor label,gm,angle in cases:',start)
source=source[:start]+"cases=[('7236','animated-cgmodelsnet-allframes-7236.gm',-.15),('7249','animated-cgmodelsnet-allframes-7249.gm',-.15),('musket-976','animated-cgmodelsnet-allframes-musket-976.gm',-.15)]"+source[end:]
for old,new in [('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('cgmodelsnet-fullbody-','cgmodelsnet-allframes-fullbody-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_fullbody_trial.py';source=path.read_text();start=source.index('cases=');end=source.index('\nfor label,gm,angle in cases:',start)
source=source[:start]+"cases=[('7236','animated-stock-allframes-7236.gm',-.15),('7249','animated-stock-allframes-7249.gm',-.15)]"+source[end:]
for old,new in [('cgmodelsnet-nose-baseline-atlas.png','officer_5-atlas.png'),('cgmodelsnet-fullbody-','stock-allframes-fullbody-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
