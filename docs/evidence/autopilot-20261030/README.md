# Player fleet observer policy

The policy controls the original player actor and reads the current companion
slots. It does not create traffic descriptors, native actors, cargo, cash,
replacement crews or a second food/wage clock. Native motion, menu/callback
wiring and installed activation retain their separate evidence boundaries.

`candidate.json` records actual full-PROGRAM execution and four rounds of the
engine VM serializer: `Run=0`, every `AfterLoad=0`, `script_errors=0`.
The fixture covers:

- Two stores and successive real trade legs. The first export debits 850 sugar
  from its store and 12,750 cash from the player; the destination receives the same
  850 units. Per-hull sale revenue and next-leg purchase costs match independent
  price/unit arithmetic. Seven units of pre-existing personal cargo remain.
- Repeated callbacks/arrivals and serializer rebinds without duplicate purchases
  or sales; original character/RealShip indices, loaded charges and reload value.
- Companion removal, finite cash/reserve, capacity, empty stock, blocked locator,
  hostile port, boarding, task locks, interfaces, reload and manual handoff.
- Existing hostile actor selection, retained combat task and route continuation
  after its target dies. Unrelated player and companion orders are preserved.
- Invalid saved navigation stays inactive and records a bounded reason.
- Normal sea teardown's retained `Ship.Stopped=true` and physical task/lock
  attributes do not block chart control; returning to the physical scene still
  honors those guards without clearing the attributes.
- Forced chart encounters enqueue one deferred handoff rather than reload inside
  the native map update. Duplicate callbacks do not enqueue again; the latest
  specific index is revalidated at execution. Manual mode, stale generation,
  locked reload, expired index, map loss, interfaces and save rebind cannot admit
  stale work. Stop/mode changes invalidate pending intent.
- Paid provisions conserve store plus physical holds. The shared daily-food
  quantity matches the original expression across 175 crew/passenger/slave cases;
  20 passengers and 80 slaves produce the corresponding 112 unit fortnight purchase.

`baseline.json` runs the original treasurer ledger from source revision
`60984eef27ca52717cdba9cac88a4ebd848d0b47`; `treasurer.json` runs its shared-helper
replacement against the same buy/sell/time oracle. Both pass with zero errors.
The original caller's policy gates, explicit excess sale, reserve, ceil purchase
payment, floor sale revenue, XP and batched service time remain with the caller.
Baseline-only link stubs satisfy newly integrated, unexecuted observer calls;
the baseline executes the original treasurer code, never these stubs.

`control.json`, `idle.json`, and `decisions.json` retain bounded process CPU
measurements including compilation/startup. Control is 0.230336 s; 100,000 disabled
callbacks take 0.264108 s, a residual of about 0.34 microseconds/callback. Two thousand
actual two-port route decisions take 1.197939 s, roughly 0.48 ms/decision after the
control subtraction. Process variance limits the small residuals; two fixture
ports do not establish full-world route or frame time. Route choice runs at
transitions or a dated blocked-market retry, never every rendered frame.

Native entity presence, engine task-message dispatch, authored locator
resolution, PostEvent scheduling, native encounter-index validation and sea
reload entry are explicit fixture seams. The actual queue/deferred policy,
task setters, roster, store,
prices, cash, per-hull cargo, market selection and serializer execute unchanged.
Commerce skill/perks are fixed inputs; product log and XP presentation are
stubs. This evidence does not establish native steering, obstacle avoidance,
gameplay appearance or installed activation.

`replay.py` uses the normal `gameplay_sources` projection and the existing
`script-vm-probe`; it never installs content or writes player SAVE data. It needs
the prepared project runtime, Python 3.12+ with the project's Pillow dependency,
and `CODEX_SCRATCH_DIR` supplied by the existing managed scratch runner. The run
budget is 512 MiB scratch, 20,000 entries, 180 seconds and 10 GiB free-space reserve;
one compiler/process runs at a time. From the repository root run:

```sh
python3 docs/evidence/autopilot-20261030/replay.py candidate
```

Other modes are `baseline`, `treasurer`, `control`, `idle`, and `decisions`.
Each record retains runner, fixture, policy and transaction-source hashes plus
the raw result/logs. Reproducible PROGRAM copies and VM data remain in managed
scratch and are reclaimed automatically; this directory owns retained evidence.

`native-frame-comparison.json` links the two same-consumer off-mode pilots and
their raw CPU/renderer streams. At640×480, one Manowar1 and112measured warmed
frames, mean native frame CPU is8.304ms before and8.296ms after. Both preserve
the required sail, animated, sea, sky and shadow pass coverage. The difference
is within run noise; neither pilot accepts whole-game FPS or pixel identity.

The normal benchmark's process lifetime required the Darwin compatibility fix
in `tools/metal_storage.py`: Python3.12 omits `os.waitid`, while libSystem exposes
the same unreaped-child `WNOWAIT` facility. SDK/C and ctypes layouts agree
(`siginfo_t`104bytes, pid/code/status/band offsets12/8/20/40). Owned exit7,
timeout, stopped child, exited leader with live descendant, non-child ECHILD
and unrelated-session preservation were checked; the original native benchmark
then completed normally. See [CPython's upstream fix](https://github.com/python/cpython/pull/113542)
and [Apple's ABI owner](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/signal.h).

## Reopened saved-state defects

The newer player snapshots contradicted the earlier fixture acceptance. Trade
was stopped with `waiting/market`: all five holds exceeded the NPC 75% freight
limit although real capacity remained. Hunt was `sailing` among ports with no
enemy target despite 23 eligible hostile fleets among 62 saved descriptors.
The earlier seven-unit private-cargo assertion described the old implementation;
the user subsequently authorized trading existing ordinary cargo as well.

The current policy uses actual free hold space and plans an existing-cargo sale
before a purchase route. Destination bids are compared with the current local
bid using the existing distance-weighted route policy, without inventing an
acquisition cost or a global maximum-profit optimizer. Cargo is sold only at its
planned destination, or locally when no eligible destination improves the local
bid. Food, ammunition, repair supplies, weapons, medicine, Rum, cannon goods,
inventory items, canonical commodity `isquest` flags and configured
`TransferGoods` target quantities remain outside automatic commodity sales.
Purchases and sales use the same finite player/store ledger.

Hunt selects an existing hostile ordinary fleet through the native encounter
catalog and descriptor policy. The stable fleet ID is authoritative; the cached
index is revalidated before reuse. Live positions drive the chart course, then
the existing deferred encounter queue admits a fleet within the ordinary
25-unit radius. Quest, friendly, retired, expired, sinking, port-service and
skipped encounters are excluded. Enemy coordinates discard port settlement
authority. Manual Off still cancels control and pending encounter admission.

`save-policy-replay.py` is a read-only one-off replay producer. Supply an
original player snapshot via `AUTOPILOT_SAVE` and select `AUTOPILOT_PROBE`:
`trade` inspects the saved route admission, `hunt` inspects the saved missing
target, `trade-after` proves full derived holds, planned finite sale, paid next
purchase and protected-cargo negatives, `trade-tick` replays normal dated-retry
and arrival callbacks, and `hunt-after` proves pursuit, admission and Off.
Run through the existing managed scratch runner with 2 GiB, 20,000 entries,
180 seconds and a 10 GiB free-space reserve; use one producer at a time.

The producer copies the original saved script state into scratch, suppresses
`OnLoad`/game scenes and leaves the original untouched. Entity/locator/task
dispatch and the native encounter catalog are explicit seams; fleet identity,
relations, eligibility, actual stores/cash/holds and policy run in the real VM.
The full-hold case adds commodities only to the disposable derived holds.
Private snapshots and personal values remain outside this repository.
`behavior-repair.json` retains the nonprivate raw results and their conditions.

Performance remains source-bounded: no new rendered-frame loop, no new polling
cadence, native catalog acquisition/reacquisition is linear in live fleet count,
and a valid cached target needs one selector plus policy validation per Tick.
Port pricing occurs at arrival or the existing dated retry; per-good hold and
sale quantities are computed once per decision. The recorded VM invocation
times include startup/resumed script work and multiple callbacks, so they do
not establish whole-game FPS or per-frame latency. Native gameplay and installed
activation remain the root integration owner's separate delivery boundary.
