# Crime actor release and registered fort positions

`baseline-cases.c.txt` is the pre-edit source-backed fixture. An existing target
at index 1 remains tracked after that slot receives a replacement identity:
`CrimeReleaseBaseline=101`, `script_errors=0`, exit 1. `baseline-results.json`
retains the actual output. `candidate-cases.c.txt` freezes the implemented
canonical helpers and fixture; `candidate-results.json` retains all four raw
successful invocations. `manifest.json` binds source and snapshot hashes.

The release candidate proves that active target/witness bindings are detached,
old actor/fleet/ordinal and reporter fields survive under unique same-list keys,
repeated release is inert, replacement tracking is false, replacement death
cannot silence old evidence, and old consequences resolve exactly once. Holds
cover survivor States 0–3, survivor callback indices, custody captor index,
prisoner state and player passengers; unrelated indices remain allowed. Scalar
legacy reporter State survives without a generic migration.

Registered-position cases prove a translated fort can witness a nearby victim,
raw canonical locator coordinates cannot grant observation, and an absent
registered position cannot grant observation. The previous `FortCrimeCases`
acceptance/negative set also passes against the new canonical functions.

`CrimeReleaseOldSave` saves a prior pending scalar reporter with no archive
counter, then releases and resolves it after a real codec load.
`CrimeReleaseNewSave` saves already archived reporters, replaces the actor after
load and resolves the preserved reports once. Both execute one normal
`COMPILER::SaveState` / `LoadState` round, without user-save access.

The fixture retains the original isolated dependencies: actual script compiler
and crime source, stubbed character/nation lookup and effect writers, no native
scene. It proves script state and codec behavior; actual borrower acknowledgement,
whole-group detachment, native fort registration and the installed game remain
integration requirements.

Use an owned temporary directory as `$fort_crime_fixture` and the existing built
diagnostic, from the repository root:

```sh
cp docs/evidence/fort-crime-20261021/release/baseline-cases.c.txt "$fort_crime_fixture/cases.c"
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" CrimeReleaseBaseline
# Expected: result 101, zero script errors, exit 1.

cp docs/evidence/fort-crime-20261021/release/candidate-cases.c.txt "$fort_crime_fixture/cases.c"
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" CrimeReleaseCases
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" FortCrimeCases
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" CrimeReleaseOldSave 1 CrimeReleaseAfterOldSave
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" CrimeReleaseNewSave 1 CrimeReleaseAfterNewSave
# Expected: all results zero, zero script errors, exit 0.
```

Preserve any new decision-bearing output before releasing that temporary
directory through the environment's approved cleanup helper. These frozen text
snapshots are evidence, not a maintained test suite.
