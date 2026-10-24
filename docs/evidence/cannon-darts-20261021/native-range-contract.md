# Prepared native range sample

Superseded by the later 30-degree flight request. Preserve this fixture and its
failed/corrected-VM evidence; do not run it as acceptance for the new physics.
The root owns binding the new range fixture after the next native source cut.

Prepared, not executed. The root must signal the final binary and exclusive GPU
window before the single 512-frame run:

```sh
python3 docs/evidence/cannon-darts-20261021/prepare_native_range.py
python3 docs/evidence/cannon-darts-20261021/native_workload.py range-contract
```

Admission: 640×480, at most 90 seconds, approximately 2 GiB additional peak,
12 GiB current host reserve, no concurrent renderer/build. The earlier native
1024-projectile pilot measured approximately 1.16 GiB RSS. This fixture has 2040
simultaneous shots, but the same eight mounted ships. The existing diagnostic
producer owns its process tree, deadline, output budget and private scratch.

Two source profiles use native mounted Manowar receivers at 2098.377758 m and
4214.511469 m; the second source/receiver pair is translated 200 m along z to
avoid the nearer target intercepting its shots. Actual Manowar cannonr locators
provide all 51 muzzle points. Each profile generates 20×51 shots. The first four
real Ship entities are removed from the execute layer after their initial pose
has been published, retaining their native BSP/model collision and rendering.
No dummy ships, replacement collision mesh, or fake controls are created.

`prepare_native_range.py` extracts the current three ammo helper functions,
actual perk lookup, and actual Darts goods initializer. Cannon 42/48 data and
speed initialization come from the installed stock cannon source. The launch
transform and three random dispersion assignments are taken verbatim from
current `Ball_AddBall`; its misfire branch and postlaunch smoke/audio are
omitted. The explicit master fixture uses Accuracy/Cannons 1.0, GunProfessional,
LongRangeShoot and effective TmpPerks.LongRangeShoot; the resulting dispersion
uses the actual script `frnd()` RNG. The source-to-world low ballistic root and
ordered RakingFire target axis follow the calibration/native AICannon equations.
This is not execution of the complete AICannon/bort controller, reload timing,
or a live AI aiming decision.

Native AIBalls owns flight, ordinary native tracing order, actual hull BSP
intersection, wave/water removal, and projectile removal. `Shp_HullHit` counts a
hit only for the matching owner/receiver and CurrentBallType 52. The existing
hull/mast callbacks return zero to preserve receiver geometry across the sample;
damage/degradation is outside this precision measurement.

The root-approved bounded acceptance is 663–765 native hits inclusive per 1020
shots (65–75%). Both counters must pass, with zero wrong receiver/identity events,
before Fleet[7] is removed. The driver's raw `ships=7 hidden=1` acknowledges that
the final predicate executed. A failing predicate removes all Ship entities and
the existing driver exits 68; an unexecuted final handler leaves eight ships and
the runner rejects it. Exact counts are exported before the final predicate
through the existing SaveEngineState VM serializer into task-owned
`range-state.bin`; the reader validates its native header/zlib and scalar record
layout. No player SAVE, native driver instrumentation or permanent tests are
added. The first silent-counter run failed opaquely; the next snapshot identified
a fixture launch setup error, documented in `native-verification-status.md`.

The generated fixture/source plus `native-range-inputs.json` retain selected
source boundaries, raw source/model hashes, distances, muzzle inputs, command,
seed and acceptance predicates. A run retains its binary/input hashes, first
raw log, timing CSVs, native exit, final image and validation. Installed gameplay,
moving targets, AI common offset, damage, reload sequencing and impact-camera
visuals remain separate acceptance requirements.
