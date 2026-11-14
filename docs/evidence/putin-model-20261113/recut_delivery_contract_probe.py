"""Read-only execution of the real asset-manifest consumer with an in-memory state."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
import sync_metal_gameplay as sync

class ProbeState:
    def __init__(self):self.records={}
    def path(self,name):return here/'__nonexistent_delivery_probe__'/name
    def admit(self,name,original,data,legacy=(),create=False):
        assert original is None and data and create
    def track(self,name,data):self.records[name]=sync.digest(data)

original=root/'src/assets/characters/putin-phonk-candidate'
recut=root/'src/assets/characters/putin-phonk-recut-candidate'
prior=ProbeState();prior_claimed=set();prior_changes=sync.asset_changes(original,prior,claimed=prior_claimed)
trial=ProbeState();trial_claimed=set();trial_changes=sync.asset_changes(recut,trial,claimed=trial_claimed)
assert set(prior_changes)==set(trial_changes)==prior_claimed==trial_claimed and len(trial_claimed)==3
for name in sorted(trial_claimed):
    print(name,'old',prior.records[name],'recut',trial.records[name])
try:sync.asset_changes(recut,prior,claimed=prior_claimed)
except RuntimeError as exc:
    assert str(exc)=='Asset manifests overlap; no files changed';print('both registered: expected overlap rejection')
else:raise AssertionError('Duplicate manifests were accepted')
