# Jumping heel kick: offline evidence

The first implementation only lifted K's poses and was rejected as lacking a
unique animation. L now has independent body/torso/striking-leg curves, an
in-flight folded launch leg, tucked/opened counterbalance arms and its own
landing. It lasts 1.4 seconds; K retains its two-second sequence unchanged.

`phases.png` shows six retained real GM meshes; `motion.png` shows the complete
man sequence at four-frame intervals from two views. Both read serialized AN
and use the native two-weight skinning model. They are offline renders, not
the game. The authored flight arc reaches 0.75 m before metric contact
adaptation; the heel reaches the head with maximum sampled error 3.2 mm.
Current [metrics](metrics.json), [source/unchanged-K proof](unique-motion-inputs.json),
[exact native scenario](unique-native.cpp.txt) and [native results](unique-native-result.json)
retain the new run. Alternating K/L/K preserves durations and contact phases;
L hits once and rejects walls, range, immortality and same-group targets.
Both actions retain their existing energy, perk and damage contracts.

The candidate adds 973,542 bytes to the combined generated animation resources
(7,825,631 to 8,799,173 bytes). It reuses the existing bank and contact path;
this avoids a second bank lifecycle at the cost of loading the extra action with
the supported hero bank. Native loading cost and played-scene motion are not
measured by this offline evidence.

Reproduce from the current recipe, rigs, and generator with the normal generated
bank preparation, then pass its output directory and an existing output directory:

```sh
python3 tools/prepare_roundhouse.py
OUTPUT_DIR="$(mktemp -d /tmp/roundhouse-evidence-XXXXXX)"
python3 docs/evidence/roundhouse-jump/probe.py experiments/native-metal/.cache/roundhouse "$OUTPUT_DIR"
python3 docs/evidence/roundhouse-jump/motion.py experiments/native-metal/.cache/roundhouse "$OUTPUT_DIR"
```

The [native contact replay](native-replay.py), [results](native-result.json)
and [rejection results](native-negative-result.json) use real character,
animation, Jolt and gameplay VM code without a game window. K and L each
produce exactly one damage event; the VM checks an exact 2× base coefficient.
L rejects a wall, excessive range, an immortal or same-group target, a missing
perk and insufficient energy. Input identities and the exact compiled fixtures
are retained in [native-inputs.json](native-inputs.json). The companion runner
recreates the positive K/L scene; the exact negative fixture retains its
additional rejection setup.

The earlier hop candidate native loop used 46.6 ms for K and 48.2 ms for L across 140 frames
including physical knockdown. These CPU totals include first-contact setup;
they are not a frame-rate or first-hit latency guarantee. Isolated controls
VM checks used stubbed native binding calls and their disposable inputs were
not retained. The migration preserves an occupied L and leaves the new action
remappable; full controls/gameplay acceptance remains the player's own replay.

The renderer requires Pillow. Normal staging verifies generated manifests,
delivers both runtime targets and signs the app; it does not play the scene.
