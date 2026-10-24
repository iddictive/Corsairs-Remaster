# Prepared native dart contract fixture

Not executed yet. The native build/runtime owner must release the shared GPU
lane and signal that the final dart binary is ready before this command runs:

```sh
python3 docs/evidence/cannon-darts-20261021/native_workload.py dart-contract
```

The fixture uses the existing hidden `sea-fleet-benchmark` driver at 640×480 for
160 frames. Ordinary baseline/candidate comparison keeps its original fixture;
this separate functional fixture replaces only its root script. Both reuse the
retained ship-type, light and day includes in `native-fixture/`.

The task-local RESOURCE tree links unchanged installed resources. Only the fresh
`src/assets/ui/darts-projectile.tga.tx` sprite and native source `balls.fx` are
overlaid. The installed application is untouched. The producer records hashes
and uses the existing bounded diagnostic process/scratch lifecycle. No player,
focus, game SAVE, or filesystem save/load round trip is involved.

Four consecutive shots run through real `AIBalls::Execute`, actual mounted
Manowar model BSP collision, and `SHIP::Cannon_Trace`'s `Shp_HullHit` callback:

1. Ordinary Balls reports good 0, target 0, owner 999; no dart flight metadata.
2. A fresh `@dart:52` reports good 52, target 0, owner 999, and fresh native
   sequence/target/velocity/inFlight metadata followed by one native hit.
3. A deliberately unarmed `@airburst:51` Bombs shot hits under 35 m and retains
   ordinary good 3/no explosion/no dart flight metadata. This is the nearest
   existing direct-shell negative, not armed-airburst acceptance.
4. A dart is saved using the actual SEA_AI/CSaveLoad codec, cleared, and loaded
   through messages 51042/51043. The codec hex lives only in the script-owned
   `BenchSeaSave.save` attribute through `SeaLoad_GetPointer`. The loaded record
   retains good 52/native BSP collision while its transient camera-launch flag
   stays false; no stale dart flight metadata is admitted.

The callback also checks native receiver identity and bounded collision xyz.
The mounted player is Fleet[6], index 999. During fresh dart flight/hit frames
19–38, Sea.Phonk.Active is enabled with Weapon 0; two later frame predicates
require the real native ExecutePhonkOrdnance owner to retain Weapon 0. The player
is spatially separate from the collision receiver. No controls are fabricated.
Hull and mast damage callbacks return zero: these are collision/identity probes,
not combat damage or hull-destruction acceptance. At the end, three product-size
dart sprites and their real tracer lines cross the diagnostic camera foreground
at distinct directions. No geometry renderer substitutes for the sprite path.

All predicates must pass before the fixture removes Fleet[7]. The driver's raw
`ships=7 hidden=1` output acknowledges that the final assertion executed. A
predicate failure removes every Ship, causing the existing driver to exit 68.
An unexecuted final handler leaves eight ships and the runner rejects it. Exact
counter logging is absent from the current driver; the predicate source, its
hash, final ship acknowledgement, actual native events and rendered output are
the available evidence. A failed launch/validation retains its first raw log.

This does not create or activate a full SEA_CAMERAS stack and cannot accept the
impact-camera cut, timing, restoration or installed gameplay. The metadata
checks cover its actual native producer only. The analytical ~70% calibration
also remains separate: this fixture does not claim a volley hit-rate sample.

A native 20-volley sample at each calibrated range needs at least 454 normal
16 ms frames to cover the longest 7.26-game-second flight. A single bounded
512-frame fixture with two spatially separate receivers and 1020 shots/range is
feasible, but needs a new resource admission and a retained exact counter output
channel; the existing 160-frame baseline cannot prove that acceptance.
