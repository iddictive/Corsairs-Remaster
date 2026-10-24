Final30 native acceptance input. The isolated bridge compiled/linked against
the frozen fc87 engine cut; its measured compiler peak was416104448bytes,
below the granted512MiB cap. Linked inputs were hashed before compilation and
verified before/after linking. The isolated probe hash is
`25d7e2d531074dcecd989bffedfe6aac864359ffdb7e21ef38511a636775217a`.

The first functional pilot stopped on reject(exit68,3.85seconds) and released
the GPU. Its raw log identifies a fixture registration failure: the functional
script declared only DartNativeLegacy, while bridge Init registers DartNativeQueue
first and returned false on its missing declaration. The resulting invalid
Legacy call left ordinaryHits0; dart/airburst/loadedDart each hit once,
explosions0, fresh/loaded metadata1, PhonkChecks2, BenchBad1. This does not show a
codec or engine regression. Raw evidence is in`native-angle30-functional/`.
All three fixtures now declare all three bridge natives before library load;
the unchanged binary needs no recompilation. Corrected functional replay passed:
all four identity hits1, BenchBad0, metadata1/1 and PhonkChecks2. The first angle
attempt then stopped at a fixture compile error before its first frame. The
user ended agent gameplay QA and requested installation; angle/range/visual
acceptance remains with the user, not an automatically pending GPU run.

`prepare_native_angle30.py` extracts the current ammo helpers, cannon parameter
recalculation and scatter; verifies the real Manowar GM hash; and generates
three bounded fixtures. It preserves all earlier13.9degree inputs/results.

`build_native_angle30.py` compiles one task-only adapter translation unit using
the actual current Ninja sea_ai compiler arguments, then links it explicitly
with the existing diagnostic driver and current archives. It modifies no
production source, API, driver, shared object, cache or installed resource.
The granted compiler bound is512MiB/120seconds; source, argv, archive/object hashes,
raw logs and executable hash are retained. Run only after the compile grant.

The native workload commands, run serially only after the GPU grant, are:

```
python3 docs/evidence/cannon-darts-20261021/native_workload.py angle30-functional-declarations
python3 docs/evidence/cannon-darts-20261021/native_workload.py angle30-angle
python3 docs/evidence/cannon-darts-20261021/native_workload.py angle30-range
```

Each hidden native run is640×480 and capped at90seconds; expected native RSS
is3GiB, retaining the root's12GiB reserve. Functional uses160frames; the other
two use512frames at the normal16ms step and TimeScale1. There is no player
launch, installed modification, build concurrency or player SAVE write.

The functional fixture retains real BSP ordinary/dart/unarmed-airburst identity,
in-memory dart codec and PhonkWeapon0 predicates. Its final scene is not sprite
proof: the previously selected crop shows a ship's bow, and an all-hits frame
does not establish a living projectile. A future visual claim needs a frame
bound to a known in-flight projectile position, not an unidentified bright shape.
Its ordinary shot is seeded from independently authored pre-edit four-lane
codec bytes. The frozen old and current Save functions are verified identical;
the provenance is a supported prior-format seed, not a player save or historical
native save capture. The real current AIBalls loader and BSP hit must accept it.

The angle fixture initializes an actual AICannon on a real mounted Manowar GM
cannonr label, recharges, queues and executes20/30/40/60/80degree shots. A sixth
80degree shot saves and reloads its pending cannon through real CSaveLoad before
Execute. The actual CannFire callback checks raw launch beta and RawAng0, and
loaded transient TargetValidfalse. Public production AIBalls::Save snapshots
actual kernel positions to CSV without private member access. The evaluator
checks every actual position, peak, complete-flight disappearance and30degree
range maximum against independent closed-form physics. Sea-contact range is a
native last-pre-water/next-step bracket. The42-pounder80degree flight fits the
512frame bound at normal timing.

The range fixture uses51actual GM labels on each of two real Manowar shooters,
20volleys/1020shots per receiver, the actual native low-root solver and current
source scatter at master skills/perks. Targets2109.50/4225.52m are derived from
the lowest real muzzle reaching the upper aim band with2m margin; nominal
master ranges are2116/4232m. Acceptance is663–765 native BSP hits per receiver,
zero wrong receiver/good identity, and2040initial active shots. Existing
SaveEngineState exports exact counts to a task-owned snapshot before rejection.
The extracted parameter recalculation adapts its single Characters[index]
reference to this fixture's real Fleet[index] owner; Storm #define supports
scalar constants and cannot provide a C identifier alias. No physics expression
is changed by that fixture adapter.

Direct AICannon::Fire exercises the queue and projectile consumer; it does not
exercise manualcontroller BuildManualAimSolution, reticle selection, player
camera choreography or installed combat. Those were deferred to the user,
including native70percent acceptance. No mathematical result substitutes for
actual BSP hit counts.

## Cheap syntax preflight before native fixtures

New Storm fixtures should first use the existing CPU-only
`experiments/native-metal/.cache/build/bin/script-vm-probe`, not a render run
as their language parser. In a disposable fixture copy, rename the scene's
`Main`, add a no-op `int ParseOnly() { return 0; }`, and replace only unavailable
task-library declarations with ordinary typed no-op functions. Keep their
argument and return types; remove the corresponding task-library directive.
Run `script-vm-probe <fixture-directory> ParseOnly` and require both
`ParseOnly=0` and `script_errors=0`. Keep the adapter explicit: it proves syntax,
not bridge registration, native calls, physics or rendering.

The retained `native-angle30-parse-seed/` input demonstrates a rejected nested
statement form; `native-angle30-parse-braces/` passes after explicit braces.
Other observed fixture hazards were initialized comma-separated locals and
using scalar `#define` as an array-name alias. Inspect actual Storm errors
instead of assuming C/C++ syntax. Valid unchanged fixtures may reuse their
matching parse result; a new fixture or changed body needs a new preflight.
Before the real native run, restore the unmodified native declarations and
library directive and match every registered native's name, argument count
and return type against that fixture. This separate readback caught the
missing declaration that prevented the legacy-shot registration.
