# Jumping heel kick: offline evidence

Current circular revision: L sweeps the actual heel/shin across approximately
120 degrees while airborne. `circular-phases.png` covers six real GM rigs;
`circular-motion.png` covers the complete sampled gesture from two views.
These are offline component frames, not gameplay captures. All 121 serialized
K frames remain unchanged on the six kick-enabled rigs.

`crowd-native-result.json` records the real Character/VM/Jolt replay: three
targets at radius 1.5 m and headings -60/0/+60 degrees, ordinary native spacing,
one damage event and one physical fall per target. The previous installed
revision admitted one victim (`crowd-baseline-result.json`). Full 140-frame
CPU replay cost was 45.403 ms before and 46.612 ms after, including the added
two physical victims; these single samples do not establish interactive FPS.
K still hits one target. Existing wall, far, immortal and same-group negatives
pass on the new engine (`circular-boundary-result.json`). Targets are admitted by actual swept heel/shin contact
and visibility, with a per-kick entity ledger; there is no radial damage pulse.

Reproduce after the ordinary engine build, with a fresh temporary directory:

```sh
export CODEX_SCRATCH_DIR=$(mktemp -d /tmp/corsairs-crowd-check.XXXXXX)
export CORSAIRS_APP="/Applications/Corsairs Iddictive Remaster.app"
python3 docs/evidence/roundhouse-jump/crowd-native-replay.py
```

The runner overlays current canonical gameplay and generated animation banks
onto an isolated installed-resource reader. No game window is created. The
retained native cut-part lifecycle replay consumes its generated fixture too;
see the combat damage evidence. Installed visual timing remains the user's
interactive check.

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
