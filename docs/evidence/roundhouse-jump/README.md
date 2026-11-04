# Jumping heel kick: offline evidence

`phases.png` shows six retained GM meshes at seven phases; `motion.png` shows
the complete man sequence at four-frame intervals from two sides. Both use the
serialized AN, the existing native two-weight skinning model and retained mesh
inputs from `../roundhouse-kick/inputs`. They are offline renders, not the game.
`metrics.json` reports the sampled mesh floor and serialized heel-to-head error.
At contact the body root rises 0.205 m and the largest heel error is 4.8 mm.
The original K frames remain identical across all six hero rigs.

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

The measured native loop used 46.6 ms for K and 48.2 ms for L across 140 frames
including physical knockdown. These CPU totals include first-contact setup;
they are not a frame-rate or first-hit latency guarantee. Isolated controls
VM checks used stubbed native binding calls and their disposable inputs were
not retained. The migration preserves an occupied L and leaves the new action
remappable; full controls/gameplay acceptance remains the player's own replay.

The renderer requires Pillow. Normal staging verifies generated manifests,
delivers both runtime targets and signs the app; it does not play the scene.
