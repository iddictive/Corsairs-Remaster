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
