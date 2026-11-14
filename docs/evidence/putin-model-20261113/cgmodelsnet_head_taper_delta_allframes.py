"""Measure candidate-to-taper head displacement in every bank frame."""
import argparse
from pathlib import Path
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('bank',choices=['man','mushketer']);args=parser.parse_args()
path=here/'cgmodelsnet_neck_weight_delta_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-stockuv-retopology.gm'),('cgmodelsnet-neck-weight-trial.gm','cgmodelsnet-head-weight-taper-trial.gm'),('cgmodelsnet-neck-weight-delta.json',f'cgmodelsnet-head-taper-delta-all-{args.bank}.json'),("np.unique(np.r_[np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532])","np.arange(anim['frames'],dtype=int)")]:
    assert source.count(old)==1;source=source.replace(old,new)
if args.bank=='mushketer':
    line="source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']"
    assert source.count(line)==1
    source=source.replace(line,line+"\nactive=rig((here/'mushketer-source.an').read_bytes());assert active['parents']==source['parents']\nsource=dict(source);source.update(rest=active['rest'],bind_pose=active['pose'](0),bind_root=active['roots'][0])")
    source=source.replace("(root/source['source_path']).read_bytes()","(here/'mushketer-source.an').read_bytes()")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
