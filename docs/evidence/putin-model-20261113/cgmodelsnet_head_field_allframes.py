"""Evaluate a swept smooth field on every discrete frame of an animation bank."""
import argparse
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('strength',choices=['35','60']);parser.add_argument('bank',choices=['man','mushketer']);args=parser.parse_args()
hashes={'35':'0e2f6f0039551ef8e4e9bfa133b1b2f4be80999c7f17e897685a6b3ab2dae498','60':'5c11551ce185b87bd17989592333701e3500a0b0994b6b837dcaeae3fff13e6c'}
path=here/'cgmodelsnet_head_taper_allframes.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-weight-taper-trial.gm',f'cgmodelsnet-head-field-{args.strength}.gm'),('91508b2c56546acfaf052b170cfbe391e7bdecd6820a3c22eae78718ff92d99a',hashes[args.strength]),('cgmodelsnet-head-taper-allframes-',f'cgmodelsnet-head-field-{args.strength}-allframes-')]:
    assert source.count(old)==1;source=source.replace(old,new)
sys.argv=[str(path),args.bank]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
