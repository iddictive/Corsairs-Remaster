# Recorded boxing retarget evidence

The replay passes unchanged retained ACCAD BVHs through `tools/boxing_mocap.py`,
native AN packing and reading, then the real `man` and coarse `Jessika` GM skins.
The PNGs contain complete frame sequences at −55° and five lifecycle samples
from front, quarter and side. This is offline geometry evidence, not gameplay.

`measurements.json` retains exact source/helper/mesh/AN hashes and every cross
and uppercut frame's source sample, elbow angle, wrist trajectory, sole-floor
error and source-driven foot-contact error. Float32 AN round-trip errors are
below 0.2 micrometres for both floor support and horizontal ankle targets.
Cross contact extends to 96.7% of the available native arm length. Raw source
and native contact elbow angles are both 150.21°; calibration adds no bend.
Uppercut travels from pelvis height to the chin-interception part of the
recorded path (output 13 / source 26), retaining a bent elbow. Source 27–29 is
high follow-through, not impact; it passes in one output frame before recovery.
The 90° torso drive is present in the capture. Native/source wrist-to-head
paths and elbow angles are retained per frame so anatomy and timing are not
inferred from isolated contact pictures.

The source shoulder axis is angled in its boxing guard. Rotating the whole
capture to flatten that axis incorrectly turns the recorded +Z strike sideways.
Pelvis up uses world Y: Hips→ToSpine is an almost horizontal source adapter.
The selected E8 right uppercut was southpaw and incompatible with the orthodox
jab/cross guard; the retained E7 left uppercut is mirrored with semantic side
swap to provide a rear-right orthodox uppercut. These are source-driven
calibrations shared by all rigs, with no model-specific bone constants.

Motion root/ankle displacement is scaled by supporting-leg lengths. Recorded
knee planes and ankle displacement drive the native leg solve; necessary
pelvis-height adaptation prevents unreachable targets from becoming foot
sliding. Native mesh sole height is restored after retargeting. No hand IK
or generated punch targets are used. ACCAD has no fingers: native fist
articulation must be supplied through the optional `hand_pose` argument.

`make_guard(source, hand_pose=None)` returns the common orthodox native pose.
`make_strikes(source, hand_pose=None, guard=None)` returns 15/20/22 native frames.
Every strike starts and ends at that exact guard; the small guard differences
blend over two entrance and three exit intervals, without changing load/contact.
The bank author must derive idle/block/dodge from the same guard. Sampling and
the source/archive/license identity are owned by the source recipe under
`src/assets/unarmed/mocap/`, not this evidence note.

Reproduce with the project's Pillow/NumPy interpreter:

```sh
OPENBLAS_NUM_THREADS=1 python3 docs/evidence/unarmed-boxing/mocap/replay.py --bank-root /path/to/disposable/banks
```

The 24 captured humanoid rigs also passed disposable finite-quaternion and
exact-guard-boundary checks. Full GM visual acceptance here covers the two
named rigs only. Native gameplay timing and finger closure remain consuming
runtime/integration checks. No game was launched or controlled for this replay.
