# Two-child carrier regression probes

From the repository root run:

```sh
python3 docs/evidence/two-child-carrier-20261116/check.py
```

Requires Python 3 and `c++` with C++17/UBSan. Writes only to a temporary directory.
The script reads actual engine sources, compiles extracted release interpolation,
continuation curve, vertical-side fallback and collision/release arbitration.
It checks exact endpoint, timestep-independent release time, distinct vertical
child targets, launcher wiring, nonrecursive HE children and same shot-group ID.
An explicit event harness covers early collision, interception, repeated updates,
and two terminal impacts; its simplified lifecycle is not the production scheduler.

These are source/formula regressions, not full engine or gameplay acceptance.
Native engine compilation, script VM/menu, installed artifact checks, visual/audio
readability, moving targets, real damage/falloff and salvo/interception playtests
remain separate checks. Half damageScale per child preserves only the nominal
combined multiplier, not the realized total damage across two impact positions.
