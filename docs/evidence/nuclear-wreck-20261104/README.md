# Nuclear wreck lifecycle fixture

The hidden native fleet fixture uses real `SHIP`, its fire model, and
`CSaveLoad`. It does not run the game executable or load a player save. The
probe calls `SetDead` and `PreservePhonkNuclearWreck` directly; its script
callbacks are synthetic. It checks a surface ship, grounded and submerged
rejection, normal sinking, fresh living ship fire-state reset, and a native
save/load of the dead ship. Its successful output is in `run.log.txt`:

```text
native lifecycle afloat=1 grounded=0 submerged=0 ordinary_sink=0.350 char=20 restored=20 fresh_char=0
```

This is not proof of live nuclear death/quest callbacks, actual coastline
geometry, visible char coverage, ship interaction in the game, or a player
save upgrade. The ground state is synthetic, and the underwater position is
set directly. The restored fire cells do not prove their visible area.

The run used Apple clang 21.0.0, macOS 27.2, Apple M5 Max, 640×480, a 16 ms
native frame and the installed game's read-only `RESOURCE` tree. Source was
working tree based on `348a7beb174f9ad1af1b82987002f545da78845a`, including the
task's uncommitted nuclear-wreck changes. SHA-256 at the run: `ship.cpp`
`e2f62bc2f9153314b73f3083307312530cef1e613ea29189595c467a4a0c5218`;
`ship_phonk_nuclear.cpp`
`bac87ad3f413bbd055d9731564f298a366dbfbcc3c31309a9d694e70e520fd00`.

To replay, set `REPO` to this repository and `RESOURCE_ROOT` to a compatible
game's `RESOURCE` directory. With the game closed, prepare the configured
native build and build its `sea-fleet-benchmark` target. Copy `probe.cpp.txt`
and `fixture.c.txt` from this directory as `probe.cpp` and `fixture.c` into a
task-owned temporary directory. Copy `engine.ini`, `day.c`, `ship-lights.c`
and `ship-type.c` from `docs/evidence/cannon-darts-20261021/native-fixture/`
into that directory and symlink its `RESOURCE` to `RESOURCE_ROOT`. Then run:

```sh
python3 "$REPO/docs/evidence/nuclear-wreck-20261104/build-probe.py.txt" "$REPO" "$WORK"
STORM_METAL_PROFILE=1 STORM_METAL_MODERN_EFFECTS=1 STORM_QUIET_CLIENT=1 \
  STORM_METAL_FRAME_TRACE="$WORK/renderer-frames.csv" "$WORK/probe" "$WORK" 2 640 480
```

`WORK` is the task-owned temporary directory above. The probe exits after its
assertions; exit code zero and the result line are the pass condition.
