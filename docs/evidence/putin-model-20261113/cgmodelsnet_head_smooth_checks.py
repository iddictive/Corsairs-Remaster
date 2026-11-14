"""Run edge and two-bank scans for the continuous head-weight field."""
import argparse
from pathlib import Path
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('kind',choices=['edges','man','mushketer']);args=parser.parse_args()
if args.kind=='edges':
    path=here/'cgmodelsnet_head_taper_edge_probe.py';source=path.read_text()
    changes=[('cgmodelsnet-head-weight-taper-trial.gm','cgmodelsnet-head-weight-smooth-trial.gm'),('cgmodelsnet-head-taper-edge-probe.json','cgmodelsnet-head-smooth-edge-probe.json')]
else:
    path=here/'cgmodelsnet_head_taper_allframes.py';source=path.read_text()
    changes=[('cgmodelsnet-head-weight-taper-trial.gm','cgmodelsnet-head-weight-smooth-trial.gm'),('91508b2c56546acfaf052b170cfbe391e7bdecd6820a3c22eae78718ff92d99a','dbe1c44a8f0d195f7279e2565be1b2b7301b400bcc74c5389c7a545d5f174972'),('cgmodelsnet-head-taper-allframes-','cgmodelsnet-head-smooth-allframes-')]
for old,new in changes:
    assert source.count(old)==1,(old,source.count(old));source=source.replace(old,new)
if args.kind!='edges':
    import sys
    sys.argv=[str(path),args.kind]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
