"""Render the body-follow trial and measure sampled bridge lengths."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_retopology_checks.py';source=path.read_text()
for old,new in [('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-neck-body-follow-trial.gm'),('cgmodelsnet-neck-retopology-scan.json','cgmodelsnet-neck-body-follow-scan.json'),('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-cgmodelsnet-neck-retopology-','animated-cgmodelsnet-neck-body-follow-'),("('410',410),('1985',1985)","('5320',5320),('7236',7236)")]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
