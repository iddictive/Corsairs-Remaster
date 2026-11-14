"""Show whole model for three active-musket-bind CPU samples, without weapon."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_fullbody_trial.py';source=path.read_text()
start=source.index('cases=');end=source.index('\nfor label,gm,angle in cases:',start)
source=source[:start]+"cases=[('idle','animated-cgmodelsnet-musket-activebind-idle.gm',-.15),('shot','animated-cgmodelsnet-musket-activebind-shot.gm',-.15),('run','animated-cgmodelsnet-musket-activebind-run.gm',-.15)]"+source[end:]
for old,new in [('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('cgmodelsnet-fullbody-','cgmodelsnet-musket-fullbody-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
