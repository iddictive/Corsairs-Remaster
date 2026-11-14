"""Replay new neck mesh under the musket bank's own bind pose."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_musket_activebind_probe.py';source=path.read_text()
for old,new in [('cgmodelsnet-stockuv-retopology.gm','cgmodelsnet-recut-trial.gm'),('faces[-694:]','faces[-344:]'),('bridge.max()<15983','bridge.max()<16271'),('(indices[edges]>=15016)&(indices[edges]<15623)','(indices[edges]>=16012)&(indices[edges]!=16270)'),('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-recut-musket-'),('cgmodelsnet-musket-activebind-scan.json','cgmodelsnet-recut-musket-scan.json')]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
