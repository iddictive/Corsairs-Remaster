# Mounted neutral fleet lifecycle

Consumer: ordinary world traffic already admitted as physical sea actors. The
reported failure was neutral ships reaching a waypoint and remaining in drift.
The waypoint mover intentionally stops at its target; the missing transition
was that `WdmTrafficVoyageUpdate` skipped every `trafficInSea` descriptor, while
the original virtual map ship had already retired. The admission task therefore
never advanced from its first destination.

The minimum repair shares the existing arrival, paid service and departure
policy with a physical-sea adapter. Current actor position decides arrival;
the adapter never moves/teleports the actor to an offscreen projection. Generated
harbour guards, military missions, battle/attack/runaway tasks, surrender,
explicit stopped ships, player companions and quest-owned actors retain their
existing authority. Neutral patrols reverse at their existing route endpoints.

State ownership:

- Mounted actors own current crew, cargo, HP, damage, identity, motion and reload.
  A non-releasing snapshot observes them before service. It retains admission
  receipts and does not refund charges still loaded in native cannons.
- The existing roster service plan owns paid resources, recruit watermarks and
  elapsed service time. Its policy performs purchases and recruits once. Only
  service-owned deltas update existing actors; full `RestoreShip` is not used on
  an active entity.
- Native sail/mast repair reuses the existing emergency repair/geometry owners.
  `trafficService.nativeSails` records the unapplied paid percentage and
  `nativeMasts` the corresponding full-mast repair. Geometry failure restores
  attributes and keeps the delta for retry, save/reload or new/unmounted hull
  restoration. It never charges another service plan for that delta. A failure
  on one hull does not discard already paid updates on another hull.
- `SeaLocator` is a read-only `WdmStaticResources` message resolving authored
  `WdmIslands::GetQuestLocator` coordinates. Missing resources/locator retains
  `trafficNextLocator` plus `trafficRouteBlocked` for retry. A failed rigging
  application retains `trafficServiceBlocked`; neither silently departs.

Evidence and limits:

- `baseline.json`: real full-PROGRAM VM, actual old `VoyageUpdate`, otherwise
  identical service descriptors. Counted service entry: offscreen=1, mounted=0.
  The repository HEAD was b325c241; runtime source hashes are authoritative
  because the parallel crew owner had active edits in this checkout.
- `candidate.json`: real full-PROGRAM VM and actual save codec, `Run=0`,
  `AfterLoad=0`, `script_errors=0`. Covers mounted arrival; pending20→100 recruits
  staying20 until24h; no duplicate stock charge; save/reload; blocked locator
  retry; departure retask; unchanged actor identity/pose/reload; admitted-state
  retention; exactly one final loaded-charge normalization; battle, guard,
  mission, surrender and explicit-stop exclusions; depleted port wait; paid
  rigging failure rollback/retry/recovery; two-hull partial native failure.
- Entity presence, commander identity-table lookup, native task messages,
  locator lookup and two-port market choice are explicit fixture seams. These
  checks do not claim physics, rendered geometry, or full-world trading proof.
  The accelerated native simulation owner must close the actual actor/task and
  locator seam; player visual review remains with the user.
- `perf-before.json` / `perf-after.json`: same VM consumer and fixture,10000
  ordinary mounted voyage refreshes,0.4108s versus1.7512s including compile/
  process startup (about0.134ms additional per refresh). No new timer exists;
  work runs in existing phased traffic refresh. In transit the work is bounded
  group/member validation and coordinate math; service additionally snapshots
  existing descriptors/ships and performs existing port policy. This is a
  component cost observation, not measured whole-frame FPS or a no-cost claim.

Reproduce from the repository root with Python3.12+ and an empty temporary
directory (allow512MiB and180s; no player state or game UI):

```sh
CODEX_SCRATCH_DIR="$(mktemp -d -t corsairs-neutral-replay)" python3 docs/evidence/mounted-neutral-lifecycle-20261029/candidate-replay.py.txt
```

Append `perf-before` or `perf-after` for the bounded process comparison.
`baseline-replay.py.txt` plus `baseline-fixture.c.txt` records the pre-edit probe;
use the recorded baseline source revision/hashes when reproducing old behavior.
The probe binary, fixture and projected gameplay hashes are in each raw record.
No permanent test framework or test-suite assertions were added.
