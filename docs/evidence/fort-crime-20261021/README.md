# Immediate coastal-fort crime reporting — 2026-10-21

The source-backed VM baseline left an existing, identified severity-2 sea crime
unreported beside a fort: reputation stayed 50, with zero public-consequence or
relation-refresh calls. The candidate applies that report immediately, once.

The canonical owner is `src/gameplay/PROGRAM/sea_ai/AIShip.c`. A loaded, living
`FORT_NORMAL` fort witnesses an unlawful attack against its own or an allied
nation inside the existing recognition radius, `1400 * 1.5 = 2100`, measured
from the victim to the fort's registered position. Local fort hostility refreshes
immediately. Public consequences still require an established identity.
`Crime.Sea.<nation>.ReportedSeverity` records the already-applied severity;
missing means zero for prior saves. Later escalation applies only the difference,
and exit from sea does not charge it again. Distant reporting keeps its timing.

## Retained evidence

| File | Purpose |
| --- | --- |
| `baseline-cases.c.txt` | Frozen pre-edit source functions and pending-crime falsifier |
| `baseline-run.txt`, `baseline-trace.txt` | Actual VM result 101 / exit 1, zero script errors, and the captured pre-edit trace |
| `candidate-cases.c.txt` | Frozen candidate functions, dependency stubs, acceptance and negative cases |
| `candidate-results.json` | Raw stdout, stderr, error log and exit status for all three successful invocations |
| `manifest.json` | Canonical source/blob identity, fixture and diagnostic hashes, build conditions |

The direct candidate run checks existing pending severity 2; attack → death and
attack → capture escalation; repeated hit/death/exit deduplication; incident-centered
observation; 2099/2100 boundary; allied versus neutral fort; dead/absent fort;
quest and protected victims; lawful war; false-flag anonymity; distant and
distant-to-coastal outcomes; the existing hunter threshold; and unchanged
severity-3 public consequences. The negative branches are assertions in the
retained fixture, not separate permanent tests.

Both save scenarios use the diagnostic's actual `COMPILER::SaveState` /
`LoadState` path for one round. `FortCrimeSeedSave` restores a prior pending
record without `ReportedSeverity`, then reports exactly once.
`FortCrimeImmediateSave` restores an already reported attack, changes flag,
preserves known identity, escalates to death and exits without a second full
penalty. Every candidate invocation returned zero with `script_errors=0`.

The real compiler/VM executes copied canonical crime functions. The fixture
stubs character/nation lookup, reputation/hunter writers, patent, journal,
quest scheduling and native relation refresh. Its empty `OnLoad` supplies the
normal codec callback. These observations prove the script branching, amounts,
deduplication and serialized attributes; they do not prove native cannon fire,
the complete PROGRAM load, the installed runtime, player saves or visible UI.
Those integration checks belong to the root delivery batch.

## Replay

From the repository root, build the existing `script-vm-probe` target with the
configured toolchain and `-j1`, after acquiring the shared build owner. Use one
task-owned temporary directory as `$fort_crime_fixture`; no game assets or user
save are needed. Copy one frozen snapshot to its `cases.c` before each group:

```sh
cp docs/evidence/fort-crime-20261021/baseline-cases.c.txt "$fort_crime_fixture/cases.c"
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" FortCrimeCases
# Expected: FortCrimeCases=101, script_errors=0, exit 1.

cp docs/evidence/fort-crime-20261021/candidate-cases.c.txt "$fort_crime_fixture/cases.c"
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" FortCrimeCases
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" FortCrimeSeedSave 1 FortCrimeAfterSeedSave
experiments/native-metal/.cache/build/bin/script-vm-probe "$fort_crime_fixture" FortCrimeImmediateSave 1 FortCrimeAfterImmediateSave
# Expected: every named result=0, script_errors=0, exit 0.
```

The probe keeps generated drivers, codec state and logs inside that directory;
retain any new decision-bearing result before releasing the directory through
the environment's approved temporary-directory cleanup helper. The source
snapshots remain evidence and are not an installed test suite.

## Dynamic phantom-slot release integration

The current crime owner keys targets as `t<character.index>`, groups as
`g<commander.index>`, witnesses as `w<index>` and survivors as `s<index>`.
`CrimeSea_IsTrackedShip` / `CrimeSea_IsTrackedAmbient` test key existence;
`CrimeSea_MarkShipUnavailable` changes reporter state but retains the keys until
`CrimeSea_ResolveDirect` deletes the nation incident at sea exit. The original
implementation let a replacement actor at the same index match that old crime;
the release baseline reproduces it with result 101 and zero script errors.

The independent sea-traffic owner binds `trafficFleetID` and stable
`trafficRosterSlot`; `WdmFleetSeaActorOwned` also checks the current group's
`quest.id_<index>` character-id receipt. `WdmFleetSeaClearCaptain` removes only
traffic tags. Resetting a character cannot remove its records stored on `pchar`.

The subsequent source/VM candidate adds `CrimeSea_CanReleaseActor` and
`CrimeSea_ReleaseActor`. After native borrowers acknowledge whole-group detach,
before character reset, the caller rechecks eligibility and archives active
target/witness nodes into the same lists under unique `rN` keys. State, identity
and attack flag survive, with the old actor id, fleet id and roster ordinal.
Removing active `tN/wN/gN` bindings prevents a replacement's tracking or death
from consuming old evidence. The existing resolver sees the archived reporters
and preserves deferred consequences and `ReportedSeverity` deduplication.
Legacy scalar reporters normalize only at release; no bulk save migration runs.

Release is held for any matching survivor receipt, regardless of State, and
matching survivor callback, custody, prisoner or player-passenger index. A State
value does not acknowledge native boat removal. The traffic owner must keep its
independent native-borrower checks; group binding removal assumes whole-group
release. These helpers do not remove boats, custody actors or native entities.

`ObserveFort` now consumes only `Forts[slot].position.x/z`; missing positions
cannot fall back to canonical island-local locators. The fort registration owner
must fill that receipt from the same translated label passed to native `AI_FORT`.

[Release evidence and replay](release/README.md) retains the separate baseline,
new candidate, negative cases and both old/new save-codec replays. The earlier
snapshots above remain the original coastal-reporting evidence. Native lifecycle
and fort registration integration remain the root delivery batch's acceptance.
