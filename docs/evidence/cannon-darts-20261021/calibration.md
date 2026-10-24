# Dart calibration, 2026-10-21

Historical low-angle candidate, superseded by the requested30-degree launch law.
The figures below retain that earlier experiment and do not describe the current
dart arc. Current native precision acceptance remains separate; the target is
still about70% of a master Manowar broadside at maximum range.

Candidate: good `SpeedV0=2`, `RangeMultiply=2`, absolute
`HeightMultiply=.4`, `DispersionMultiply=.30`,
`AccuracyReferenceRange=2116`. Apply the effective dispersion to **all three**
launch perturbations and the native preview:

```
effective = .30 * 2116 / (cannon.FireRange * 2 * LongRangeShootSpeedFactor^2)
```

This is mathematical calibration against the shipped Manowar root-hull mesh,
**not native hit-count or player-scene acceptance**. The source baseline was read
before production native edits; no engine build, game launch, or installed-file
mutation was performed by this experiment. No native AIBalls timing baseline was
claimed: the existing fleet diagnostic does not fire a projectile workload.

## Inputs and mechanism

- Stock 42lb FireRange800 and quest 48lb FireRange1600; cannon cap `.60rad`,
  cannon HeightMultiply1, gravity9.81. LongRangeShoot speed factor1.15.
- Maximum Accuracy skill and relevant perks reach the existing `.05` scatter
  floor. `Bring2Range(.5,1.2,.2,1.2,.05)` clamps to `.5`.
- Full crew/cannoneers/Cannons skill and RakingFire give rake factor1. The real
  51 starboard gun locators are sorted by hull axis and aim across its full span.
- Cannon_GetFireHeight uses the Manowar's Balls band8.6, DY4.3; at master
  Cannoners/skill the common broadside aim height is8.6±.645m.
- The low cap is `.5*asin(sin(2*.60)/2)=.242393290836rad`,13.88811255°.
  Speed doubles and equal-height maximum range doubles. The existing raw-angle
  rotation/height scaling/rotation-back trajectory preserves its nominal aim
  endpoint; this is retained by the calibration formula.
- The real root mesh has82,759 triangles and hull bounds x±8.2705002,
  y[-5.3555050,13.7394934], z[-35.7909775,33.6990242]. Gun local y spans
  3.3399999..10.5900002m. The JSON contains all51 source muzzle positions and
  the source asset SHA256.

## Results

51,000 shots per case,1,000 complete broadsides, seed20261021. The hull oracle is
a2cm rasterization of the **actual triangles**, projected along the mean terminal
trajectory. Launches use the full warped analytic trajectory, independent
uniform azimuth/elevation/speed samples, and one common target-height draw per
broadside. Flat-water misses before the target are excluded from hits.

| Cannon | Advertised equal-height range | Full broadside target-band range | Effective scatter | Nominal/manual hits | Automatic hits |
| --- | ---: | ---: | ---: | ---: | ---: |
| 42lb | 2116m | 2098.3778m | .30 | 70.0314% | 69.3745% |
| 48lb quest | 4232m | 4214.5115m | .15 | 69.8667% | 69.1608% |

The target-band range is the shortest of the51 actual muzzle-to-rake-station
maximum distances at target Y9.245, minus1cm. All51 nominal solutions pass the
derived cap. The correction from the equal-height maximum is below1%; evaluating
at the sea-level advertised distance instead would silently include guns whose
raised target is just beyond their elevation cap.

The automatic branch includes the existing common4.4m-radius azimuth-random
`Ship_GetBortFireDelta` offset (master floor.08 ×55). Manual crosshair fire skips
that event. The `nominal_reachable_guns` field describes unshifted nominal aim;
it does not claim all automatic offset samples remain within the cap.

42lb nominal flight:10.20669 ball-seconds,5.10335 game-seconds with current
`AIBalls.SpeedMultiply=2`; world apex58.52696m from mean muzzle Y6.24294m.
48lb:14.52058 ball-seconds,7.26029 game-seconds, apex110.84540m.
For comparison, stock balls at their own equal-height maximum take6.07386 and
8.58974 game-seconds respectively; their `.7` height scaling gives apex rises
126.66793 and253.33586m above the muzzle. Darts cross twice the distance sooner
and with a much lower arc.

Sensitivity: canonical.28 gives approximately71.5% nominal/71.1% automatic;
canonical.32 gives68.3% nominal/67.6% automatic. A fixed unnormalized angular
multiplier cannot hold the same hit fraction at both cannon ranges. Zero
dispersion still gives about89.2% nominal because full RakingFire sends end guns
to tapered/high hull stations; it does not imply a100% hit oracle.

## Reproduction and limitations

From the repository root, with the installed asset pack present and Pillow:

```
python3 docs/evidence/cannon-darts-20261021/calibrate.py --shots 51000 > docs/evidence/cannon-darts-20261021/calibration.json
```

Input loader is the repository's existing `experiments/native-metal/island_geometry.py`.
No synthetic width/height box substitutes for the mesh. Still, the mesh oracle
uses one mean terminal tangent, so small per-gun/per-shot direction changes can
move the projected contact by decimetres. Curvature over the16.54m hull depth is
centimetre-scale. Raster resolution adds2cm contact uncertainty. It uses root
render triangles rather than native BSP collision, omits rigging/secondary nodes,
waves, ship roll/sway, waterline-dependent loading, damage, moving targets and
firing delays. Those effects require an engine/runtime replay before claiming
observed combat accuracy. The range-normalized .30 candidate is supported by
this approximation, not proven as an exact engine hit rate.

Source owners: `PROGRAM/sea_ai/AIBalls.c` (scatter/flight inputs),
`PROGRAM/sea_ai/AICannon.c` (speed/perks/target band), `PROGRAM/sea_ai/AIShip.c`
(common automatic offset), native `ai_cannon.cpp` (height/elevation solver),
`ai_balls.cpp` (warped trajectory), and `ai_ship_cannon_controller.cpp`
(live preview, muzzle enumeration and RakingFire station distribution).
