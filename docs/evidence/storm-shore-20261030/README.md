# Storm shore motion diagnosis

The user reports this at ordinary game speed, not only during time acceleration.
The reported coastal water jerk remains unresolved. No sea, foam or
shader repair was applied. The still image cannot establish temporal motion;
no game/UI launch, gameplay automation, GPU component or private SAVE was used.

The source diagnosis found one adjacent contact discrepancy: SEA displaces
render vertices horizontally by `normal * PosShift * 3`, while both CoastFoam
types retain their authored horizontal coordinates and sample scalar `WaveXZ`
plus the declared0.2m offset. Under the authored storm profile this can make
foam float above the represented surface. It does not prove a water jump.

The frozen [replay](contact-replay.py) evaluates original128×128 TGA heights,
packed normal construction, two-layer spatial/temporal interpolation, authored
storm amplitudes64/9, scales0.075/0.5, phase speeds4.25/8, movement8/12, starting
phases0/10 and PosShift4.25. It follows the surface vertex at parameter84,100
for8 seconds at30Hz. The scalar foam query at its displaced location has
clearance0.267784–1.484837m; a hypothetical matched native surface point stays
at0.2m. There are zero surface crossings or submerged samples in this replay.
The hypothetical matched point is an arithmetic comparison, not shipped code.

[Original aggregate stdout](contact-result.json) is retained exactly. The241
individual rows existed only in the original process memory and were not
retained; no rerun was performed while archiving this evidence. This is a
Python double-precision reconstruction of native formulas, not an engine-linked
or rasterized triangle test. It omits actual camera/LOD topology, foam anchor
animation, weather transitions, land occlusion and the player's scene. It is
therefore evidence of continuous contact drift only, not proof of the reported
jerk or of a production repair. No timing or FPS claim is made.

Reproduce from the repository root with the64 runtime inputs available:

```sh
python3 docs/evidence/storm-shore-20261030/contact-replay.py
```

[Input/source identities](identities.json) bind the original inspected clean
sources to commit`a7f9f9ca85c3e3543854ea69518e527bb9fb61a3` and the local TGA
inputs. TGA bytes remain in the project's existing runtime resource owner;
this evidence adds no duplicate asset bundle. These assets are ignored inputs,
so Git alone does not reconstruct them; matching hashes are required for a
comparable replay. The replay and aggregate are retained for the root's
unresolved-defect disposition and any later source-bound investigation.
