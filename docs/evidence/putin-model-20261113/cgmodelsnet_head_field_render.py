"""Render a swept head-weight candidate in representative poses."""
import argparse
from pathlib import Path
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('strength',choices=['35','60']);args=parser.parse_args()
path=here/'cgmodelsnet_head_smooth_render.py';source=path.read_text()
for old,new in [('cgmodelsnet-head-weight-smooth-trial.gm',f'cgmodelsnet-head-field-{args.strength}.gm'),('animated-cgmodelsnet-head-smooth-',f'animated-cgmodelsnet-head-field-{args.strength}-')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
