# Player fleet autopilot

## Contract

The player can enable autonomous trading or hunting, watch the existing flagship
and companions, retain normal camera/time controls, and return to manual sailing.
The entry is Debug menu → Diagnostics: trading autopilot, hunting autopilot and
manual control. The existing command slots, camera and time controls are reused.

The existing `Characters`/`RealShips`, player money, store inventory, cargo,
ammunition, damage and companion slots remain authoritative. No replacement
fleet, generated cargo/profit, NPC traffic identity or second consumption clock
is permitted. Existing NPC fleet ownership/release guards remain unchanged.

The saved owner is `pchar.Ship.Autopilot`: `mode` is 0/manual, 1/trade or 2/hunt.
`active` is the runtime control projection, asserted only while the current
runtime and player orders are eligible. The same subtree owns the current port,
leg, planned sale destination/minimum bids and stable Hunt target ID. A cached
native encounter index is revalidated against that ID. Cargo itself remains on
each captain. Old saves without the subtree are manual.

Enable selects a real reachable/legal port from observed position. Arrival is
based on actual flagship position, not an elapsed timer or teleport. Trade uses
finite stock, actual prices/cash/capacity and per-hull cargo; repeat arrivals
must not repeat transactions. Existing ordinary commodities are routed for sale
before purchasing another load, including when the real holds are full. Food,
ammunition, repairs, weapons, medicine and Rum remain aboard, as do cannon goods,
inventory items, commodity `isquest` flags and `TransferGoods` target quantities.
No sale occurs at the first service port merely because it was nearest: a sale
uses its planned destination, or the current port when no eligible destination
improves its bid. Unknown acquisition costs are not treated as proven profit.

Hunting on the chart selects an existing hostile ordinary fleet from the native
encounter catalog, follows its live position and uses the ordinary deferred sea
admission within25map units. Quest, friendly, retired, expired, sinking,
port-service and skipped encounters are excluded. A valid target is retained;
without one, existing legal-port patrol remains the fallback. Physical combat
uses existing enemy relations and native combat tasks. Companions follow/defend
through existing squad commands.

The policy yields during reload, boarding, interfaces, death, surrender, task
locks and active combat. It must not overwrite unrelated quest/companion orders.
Disable releases only autopilot orders and restores manual control without
rewinding pose, cash, cargo, damage, camera or time acceleration. Save/load
rebinds the control projection after native restoration, without actor snapshots.

## Owners and reuse

- Native `AIShip`/`SHIP` keep player identity, camera and HUD. A shared explicit
  control predicate admits the same navigation, avoidance, rotation, sail and
  automatic cannon controllers used by NPC ships; manual sailing inputs yield.
- A player policy adapter owns enable/disable, route decisions, live companion
  projection and sale plans. Existing market/route scoring, locator
  resolution, store pricing and player cargo/cash functions are reused.
- NPC `WdmFleetSeaActorOwned` and snapshots continue excluding companions.
- Existing player daily food, wages and ordinary repair owners remain unchanged.
- `scripts/sea_autopilot.c` is the shared policy entry; normal PROGRAM projection
  and native engine installation activate it together in existing installations.

## Performance and falsifiers

Route/market selection runs at route transitions, not each rendered frame. The
policy uses the existing sea/map residency cadence. Hunt acquisition scans the
native live-fleet catalog; a retained target needs one index/ID revalidation.
Port decisions project cargo quantities once, reusing them across candidates.
Native overhead applies to
one flagship; disabled mode preserves ordinary controls. Full-path cost is
unmeasured at intake: retain a same-consumer native baseline before native edits
and compare disabled/enabled paths with their actual control outputs.

Use the existing native component and real PROGRAM VM runners. Required evidence:
actual player control admission/return, real movement and obstacle avoidance,
two-port stock/cargo/cash conservation, companion identity, manual-off negative,
insufficient cash/capacity, blocked locator/route, combat/boarding/task locks,
pause and old/current SAVE codec replay. Native seams remain explicitly labelled;
compilation cannot establish gameplay appearance or the user's observation run.
No new permanent test target or separate world simulator is part of this change.

## Current evidence

### Reopened fleet economics contract

The newer player run disproved economic acceptance: the old route score reused
the full cash and hold budget independently for each good, bought in enum order,
and ignored travel, food and wages. A source-authored five-Fleut control also
departed with tiny cargo on one hull. Finite bookkeeping alone is insufficient.

The admitted repair adapts the existing player price/unit/cargo/store ledger
into one feasible decision-local fleet plan, shared by route scoring and
execution. It considers all eligible hulls, shared cash, protected stocks,
legal player shop inventory and finite destination opportunity; NPC EXPORT-only
stock reservations do not govern player purchases. Existing cargo sales and new
purchases may share the same stop and departing leg. The general OR-Tools
multiple-knapsack solver is a maintained alternative, but adds an absent native
dependency/binding; a bounded commodity/fleet/port planner fits this owner without
claiming global optimality.

Route viability must include source-owned food and wage costs and a context-bound
travel estimate. `WdmStaticResources` produces `AutopilotSpeedProfile` only when
queried, returning native `mapMaxSpeed` and `knotsToMeters`; the policy consumes
it once per route decision. There is no new startup service, poll or cache.
An older native installation without that query must wait with a retrievable
`navigation-profile` reason and remain manually recoverable. Chart wind/clock,
physical sea frame/speed/clock and observed elapsed legs retain their own owners;
the estimate is not a future wind or complete island-path oracle.

Acceptance requires a finite monthly replay through existing calendar/store/
consumption owners: a properly equipped merchant fleet earns a substantial
surplus (roughly twice upkeep in trading margin is a benchmark), while a heavily
crewed battlefleet may correctly have no profitable route. A route that cannot
cover its estimated costs must wait with diagnostics, not sail empty or create
funds. Per-leg transactions, old saved waiting state, protected cargo and manual
Off remain binding. Actual cash, remaining inventory value, food and wage
liabilities must be distinguished. No wage/price reduction, free provisions,
companion-count change or second upkeep clock is admitted by this repair.

Performance baseline uses the same actual saved five-hull state and VM producer:
one old route/ledger decision took about50ms, including VM invocation overhead;
merchant/battle authored controls took about69ms. The candidate may add bounded
joint planning at arrival/dated retry only. It must reuse decision-local quotes
and stock projections, preserve idle Tick cadence and report comparable decision
timings; no pricing/allocation work may move into rendered frames. These are
script-policy measurements, not whole-game FPS evidence. Final acceptance is
pending the candidate and monthly replay.

The [native and real PROGRAM replays](evidence/autopilot-20261030/README.md)
retain finite two-port trade, cash/cargo conservation, four VM serializer rounds,
ordinary treasurer/daily-food parity, deferred encounter admission, native control
admission and manual return. The real mounted Manowar1 moves38.065m and turns
0.08036rad over16simulated seconds, then coasts25.626m after Off. The fixture
retains its authored propulsion limits; full crew/weather skill projection,
physical island routing and in-game camera/menu appearance are unverified.

Route selection is profit-weighted using existing world-route weights, not a
global maximum-profit solver. Prices, stock, capacity and cash are re-evaluated
at each port; a profitable repeated gold route remains eligible while its stock
and market demand permit it. Ordinary cargo already aboard is eligible under
the same sale protections. Protected or unsaleable full holds can wait with
`cargo-reserved-or-unsaleable`; the market retry retains its existing dated cadence.

The newer actual saved states reopened earlier acceptance: the NPC75% freight
ceiling falsely removed real free capacity, and chart Hunt had no hostile target
selection. The [saved-state repair evidence](evidence/autopilot-20261030/behavior-repair.json)
and its [replay conditions](evidence/autopilot-20261030/README.md#reopened-saved-state-defects)
prove the corrected policy, full-hold finite sale/buy, canonical cargo protections,
saved waiting/arrival progression, target following, deferred admission and Off.
These VM replays retain explicit native seams and do not establish installed
activation or gameplay appearance.
