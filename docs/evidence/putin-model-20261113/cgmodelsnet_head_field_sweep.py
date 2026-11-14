"""Compare two smoothed falloffs on neck and adjacent-head deformation."""
import contextlib
import io
import json
from pathlib import Path
here=Path(__file__).resolve().parent
for strength,label in [(.35,'35'),(.60,'60')]:
    path=here/'cgmodelsnet_head_weight_smooth_trial.py';source=path.read_text()
    for old,new in [('field[ids]<.025','field[ids]<.045'),('field[ids]/.025','field[ids]/.045'),('blend=.85*',f'blend={strength}*'),('cgmodelsnet-head-weight-smooth-trial.gm',f'cgmodelsnet-head-field-{label}.gm')]:
        assert source.count(old)==1,(old,source.count(old));source=source.replace(old,new)
    old='target=np.interp(theta,angles,weights)'
    new="""grid=np.arange(256)*2*np.pi/256
base=np.interp(grid,angles,weights)
kernel=np.exp(-.5*(np.arange(-15,16)/6.5)**2);kernel/=kernel.sum()
smooth=sum(np.roll(base,k)*weight for k,weight in zip(range(-15,16),kernel))
target=np.interp(theta,np.r_[grid,2*np.pi],np.r_[smooth,smooth[0]])"""
    assert source.count(old)==1;source=source.replace(old,new)
    with contextlib.redirect_stdout(io.StringIO()):exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
    path=here/'cgmodelsnet_head_taper_edge_probe.py';source=path.read_text()
    for old,new in [('cgmodelsnet-head-weight-taper-trial.gm',f'cgmodelsnet-head-field-{label}.gm'),('cgmodelsnet-head-taper-edge-probe.json',f'cgmodelsnet-head-field-{label}-edges.json')]:
        assert source.count(old)==1;source=source.replace(old,new)
    with contextlib.redirect_stdout(io.StringIO()):exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
    path=here/'cgmodelsnet_neck_retopology_checks.py';source=path.read_text();source=source[:source.index("path=here/'animated_face_render_trial.py'")]
    for old,new in [('cgmodelsnet-neck-retopology-trial.gm',f'cgmodelsnet-head-field-{label}.gm'),('cgmodelsnet-neck-retopology-scan.json',f'cgmodelsnet-head-field-{label}-bridge.json')]:
        assert source.count(old)==1;source=source.replace(old,new)
    with contextlib.redirect_stdout(io.StringIO()):exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
    edges=json.loads((here/f'cgmodelsnet-head-field-{label}-edges.json').read_text())['rows'];bridge=json.loads((here/f'cgmodelsnet-head-field-{label}-bridge.json').read_text())['rows']
    print(label,'sampled worst affected-edge increase',max(x['max_increase'] for x in edges),'relative',max(x['max_relative'] for x in edges),'worst body-head edge',max(x[1] for x in bridge))
