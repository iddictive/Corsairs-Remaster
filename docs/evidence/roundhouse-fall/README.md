# Ragdoll fall recovery

The original native replay reproduced the reported teleport: after a 3 m
drop the pelvis stayed near −2.82 m, but at five seconds the character entered
get-up at the launch floor, +0.023 m. [Baseline output](baseline.txt) and the
[exact scenario](baseline.cpp.inc) retain that observation; the roundhouse
implementation was source revision `fb4ae910`.

The current [native scenario](replay.cpp.inc) uses the existing CPU scene
adapter, real character/animation/Jolt code, and the canonical gameplay VM.
Only camera/graphics startup and blood presentation are omitted. Its two-level
Jolt collision fixture isolates a dock-like ledge; it is not a captured dock.

[Results](result.json) cover nine seconds per case: a 3 m drop kills the
ordinary victim and leaves the corpse below; a flat-floor knockdown recovers
with HP unchanged; an immortal victim survives and uses protected recovery;
a fall beyond collision geometry kills and releases its physics body; a 1 m
drop stays nonlethal without teleporting to the launch anchor. Script damage
still owns minimum-HP quest callbacks, immortality and credited death handling.

The threshold is 2 m below the launch floor, measured from the lowest physical
body endpoint minus its radius. No new event runs for a normal flat-floor
fall. Each fall sends at most one fatal attempt and one recovery classification.
The extra active-fall work is a bounded scan of the existing 15 bodies;
full-scene FPS is unmeasured. A dead body more than 10 m below launch retires
from physics while its final pose remains until normal entity/location cleanup.

Reproduce after the normal native build and roundhouse asset preparation:

```sh
scratch="$(mktemp -d /tmp/corsairs-fall-replay-XXXXXX)"
CODEX_SCRATCH_DIR="$scratch" CORSAIRS_REPLAY_OUTPUT="$scratch" CORSAIRS_APP="/path/to/Corsairs.app" python3 docs/evidence/roundhouse-fall/replay.py
```

The runner writes only to the caller's scratch directory and reads the app.
It overlays current canonical gameplay there; it never starts the game or
reads saves. [Input identities](inputs.json) bind the recorded run to its
source/assets and cached libraries. The caller owns scratch cleanup.
