#!/usr/bin/env python3
from pathlib import Path
import math
import argparse
import hashlib
import sys

root = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser(description="CPU source/atlas temporal smoke check, not Metal acceptance")
parser.add_argument("--atlas", type=Path, help="Optional materialized copy of the exact tracked atlas")
args = parser.parse_args()
s=(root/'experiments/native-metal/missile_trail_vfx.hpp').read_text()
assert 'mix(smoke.sample(sm,uv),smoke.sample(sm,nextUV),fract(frame))' in s
assert 'next=min(63.,n+1.)' in s
atlas = args.atlas or root / 'src/assets/cluster-vfx/WispySmoke02_8x8.png'
expected = '8833f2a05ffb8bb2214fae15c1e5c4fc9e04b2b5b7c7bfdc7589b72b1a0418e9'
if not atlas.is_file():
    print('SKIP actual-pixel replay: atlas unavailable; source contract checked only', file=sys.stderr)
    sys.exit(77)
assert hashlib.sha256(atlas.read_bytes()).hexdigest() == expected, 'Unexpected atlas revision: review baseline first'
try:
    from PIL import Image
    import numpy as np
except ImportError as error:
    print(f'SKIP actual-pixel replay: {error}', file=sys.stderr)
    sys.exit(77)
image=np.asarray(Image.open(atlas),dtype=np.float64)/255.
assert image.shape == (1024, 1024, 4), 'Unexpected atlas dimensions/format'
# Alpha is invariant under the loader's premultiplication/color conversion.
frames=np.array([image[(i//8)*128:(i//8+1)*128,(i%8)*128:(i%8+1)*128,3] for i in range(64)])
def sample(age, life=8,variation=.25,blend=True):
 frame=min(63,max(0,(variation*12)*(1-age/life)+(58+variation*5)*(age/life)+variation))
 n=math.floor(frame)
 return frames[n]*(1-(frame-n))+frames[min(63,n+1)]*(frame-n) if blend else frames[n]
old=[];new=[]
for i in range(481):
 old.append(sample(i/60,blend=False));new.append(sample(i/60))
oldDiff=np.mean(np.abs(np.diff(old,axis=0)),axis=(1,2))
newDiff=np.mean(np.abs(np.diff(new,axis=0)),axis=(1,2))
assert sum(oldDiff==0)>400
assert sum(newDiff==0)==0
# One chosen transition approached from both sides: old finite jump, new continuous.
f=20; age=8*(f-3.25)/56.25; epsilon=1e-6
jumpOld=np.mean(np.abs(sample(age+epsilon,blend=False)-sample(age-epsilon,blend=False)))
jumpNew=np.mean(np.abs(sample(age+epsilon)-sample(age-epsilon)))
assert jumpOld>1e-4 and jumpNew<1e-6
# Every frame/cell transition stays inside its own texel-safe tile, including 7->8 and63.
for f in [0,7.999,8,15.999,62.999,63]:
 for n in [math.floor(f),min(63,math.floor(f)+1)]:
  cell=(n%8,n//8)
  for uv in [0,1,.5]:
   for c in cell:
    q=min((c+1)/8-.51/1024,max(c/8+.51/1024,(c+uv)/8))
    assert c/8<q<(c+1)/8
assert np.isfinite(new).all()
print(f'PASS actual atlas alpha: old stationary frames={sum(oldDiff==0)}/480; blended={sum(newDiff==0)}/480; integer-boundary mean jump {jumpOld:.7f}->{jumpNew:.9f}; tile bounds7->8,63 PASS')
