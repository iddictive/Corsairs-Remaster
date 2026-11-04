# Combat damage geometry evidence

Follow-up: ordinary world blood stains previously never aged while the ring
buffer was below 75% capacity. They now use the existing 5.5-second lifetime
and fade regardless of occupancy. The per-frame scan is bounded by the existing
10,000-triangle buffer; its full-scene frame-time cost has not been measured.
This corrects a proven stale-world-stain path, but does not establish that it
was the source of the user's particular floating mark. Ship stains retain their
separate attached path.

Integrated gameplay contract: substantial, actual unblocked blade/gun damage
starts 90 seconds of nonlethal bleeding at 2% of **current** HP per second.
Both at least 8 HP and at least 8% of maximum HP must have been lost. Fists,
kicks, blocks, misses, immortality and trivial wounds do not start it. A new
qualifying wound refreshes duration without stacking the rate. Healing keeps
the timer; bleeding alone clamps to 1 HP. Existing AI updates, saved character
attributes and portrait updates own the state; no startup prerequisite or
new save codec was introduced. `bleeding-result.json` records the actual VM
probe of cadence, threshold, healing, refresh, expiry, floor and dead/immortal
cleanup. Run `bleeding-probe.py` with `CODEX_SCRATCH_DIR` set to a fresh temp.

Accepted weapon damage sends its source entity/kind and post-death-policy
fatal result to Character. The existing engine resolves melee arcs/hitscan
targets, not triangle hitpoints; the model intersects that accepted combat
line with its actual skinned receiver. A miss creates no guessed surface mark.
Blade fatalities may detach the neck/head; bullet hits retain attached wound
patches. The canonical rig's head ancestry identifies the neck seam.

`detached-native-result.json` proves the real gameplay-VM wound helper's message
transport into Character, the model API seam and the location's actual Jolt
world: one body despite repeated
impact, 2.74 m of physical movement published back to the model, and cleanup
to zero bodies. It intentionally substitutes model graphics; it does not prove
MODELR Capture/Draw. After the crowd runner, reproduce with
`python3 docs/evidence/combat-damage/detached-native.py` in the same scratch.

The installed candidate engine hash is
`7ddb0228d3a7aa5dfd9ae6b4c39a52184cb0d8e7538bb0c5ab269d2090a69b47`.
Normal staging delivered source gameplay and generated banks alongside it;
installed visual materials, HUD placement and full-scene FPS remain uninspected.

Integration owner: the combat upgrade task. This evidence covers real skinned
receiver geometry; it does not establish native materials, HUD visibility,
frame time, physics settling, installed delivery, or interactive acceptance.

`man-component.png` contains complete guard, bind, and detached-head component
frames above corresponding chest details. It uses the actual retained GM mesh
and skeleton poses, neutral gray material, source wound colors/alpha, and a
depth-tested offline raster. The head's displaced position is an inspection
pose, not a simulated physics result. `man.json` preserves exact expanded
triangles/colors/alpha. No game was started, inspected, or captured.

The native `probe.cpp` includes the runtime geometry owner
`engine/src/libs/model/src/combat_damage_geometry.h`. It exercises the same
influence-field split, cap triangulator and finite surface projection, then
reconstructs skin anchors under two actual poses. `summary.json` retains event
costs, source/derived triangle counts, anchor errors and source hashes.

Five retained skins pass: man, man1, man2, Danielle, YokoDias. Maximum source
anchor reconstruction error is below 1.3e-7m. A wound references 6–20 source
triangles in these samples; a projection ten metres off the receiver yields no
triangles. Every admitted seam edge has exactly one cap boundary edge on both
complementary pieces; internal cap edges have incidence two. Timings are single
geometry-event samples (cut 0.18–0.40ms; wound 0.44–1.00ms), **not** comparative
full-path performance measurements.

`initial-cap-failure.json` preserves the falsifier that reopened initial cap
acceptance: exact 0.5-influence source edges were admitted twice, leaving eight
unmatched seam edges on man and sixteen on YokoDias. The corrected collector
assigns them only to their kept-side triangle. The runtime now rejects any
incomplete cap, and all five supported samples have zero unmatched edges.
`initial-summary.json` is the earlier, superseded geometry-only result.

Jessika is rejected safely: 46 seam segments form closed loops (46 welded
vertices, all degree two), but the existing cap triangulator returns no cap.
The model retains the complete original skin and its wound. A reliable
triangulation of this non-planar/projected contour is required before enabling
that cut; no guessed cap is emitted.

Reproduce from repository root with the project-selected Python and clang:

```sh
python3 docs/evidence/combat-damage/prepare.py
combat_probe_dir=$(mktemp -d /tmp/corsairs-combat-check.XXXXXX)
clang++ -O3 -std=c++20 -Iexperiments/native-metal -Iexperiments/native-metal/engine/src/libs/model/src docs/evidence/combat-damage/probe.cpp -o "$combat_probe_dir/probe"
"$combat_probe_dir/probe" docs/evidence/combat-damage/man.txt docs/evidence/combat-damage/man.json
python3 docs/evidence/combat-damage/render.py
```

The preparation script reads the canonical retained real-GM inputs under
`docs/evidence/roundhouse-kick/inputs/` and the versioned rig source. It can
recreate each named skin input; invoke the probe with that skin's `.txt`/`.json`
for the same algorithm check. The source compiler check used the current model
and battle-interface target's definitions/includes, Apple clang 21.0.0,
arm64 C++20, macOS SDK 27.2, deployment target 15.0.

Source/runtime integration remains owned by `MODELR`, `CombatDamage`, the
Character accepted-weapon-hit/Jolt lifecycle hooks, and the existing BIManSign
portrait owner. Blood-loss percentage and remaining duration come from the
gameplay bridge, not from diagnostic or graphics constants.
The font owner decodes UTF-8 (`font.cpp`, `Utf8ToCodepoint`); both prepared font
tables contain `interface_normal` entries for Cyrillic `с`, `%` and `/` used by
the compact rate. This proves glyph support, not the uninspected rendered HUD.
