# World-map traffic

## Ordinary admission and legacy pursuit — October 7

The resident scheduler must be the only caller that creates ordinary map fleets.
`WDM_TRAFFIC_TICK` still called the old random Follow generator after that
scheduler. Refresh adopted each new Follow into persistent traffic, while the
transient pursuer counter excluded every descriptor with `trafficRole`. Thus
the counter reopened every tick, bypassing both role and total admission limits.
The player's November 1 save contains 74 living ordinary groups: 38 commerce,
15 patrol and 21 pirate. This contradicts the former asserted 60-group bound.

The tick now retains scheduler admission and special pickups but removes both
legacy ordinary Follow and Warring constructors. Resident fleets already own
pursuit and autonomous clashes; authored quest constructors and storm callbacks
are unchanged. A disposable real-VM probe seeds an already full resident world
and models the existing refresh adoption. Three tick calls reproduce three
extra Follow groups before the repair and none afterward, with zero script
errors. Disabled encounters, ordinary scheduler requests and special pickups
pass, including four native serializer rounds. The composed installed PROGRAM
also compiles with the played app's shared headers; only Main is replaced by
the compile-only runner. These checks do not accept player map behaviour.

The 32/16/12 and 60 limits govern admission. A save already exceeding them keeps
its living hulls and cargo; admission pauses until losses bring its count below
the relevant limits. The repair does not delete fleets to manufacture compliance.
Separate saved evidence shows three LeFransua returning groups remaining around
552/-371 from October 28 through November 1, outside the 12-unit arrival radius.
The cause of that physical arrival failure is still under native investigation;
ordinary timed service at a reached berth is a distinct valid state.

The installed PTC table connects the stalled node3540 to destination3570 in six
transitions. A disposable Python port of edge refinement reports an open route;
it does not execute native ship movement or GM collision. The proposed shore-force
equilibrium omits the installed berth fade, and all three returns already have
ServiceOnArrival=1; neither claim admits a repair. `WdmShip::ShipUpdate` also keeps
a static collision-escape vector initialized by the first ship and reused by
others. That source defect needs a native collision falsifier before attributing
these particular stalls or changing recovery. Root owns the advancing-map
movement/collision capture when the player provides that scene; no arbitrary
arrival-radius expansion is accepted.

## Sea outcomes and ordinary NPC identity — October 7

`InitCharacter` initializes `ch.quest` and `ch.quest.meeting` on every character,
including ordinary sea phantoms. That container is not a story-actor marker.
`WdmFleetSeaActorOwned` previously rejected it, so ordinary actors could neither
restore their saved hull state nor publish `seaLoaded`; death and exit snapshots
then retained the old living map roster. The canonical `sea.c` now rejects the
explicit `isquest` marker instead. Descriptor quest/qID, companion, current group,
character identity, ordinal and admitted-hull receipt checks still gate writeback.
The legacy bridge template is not the active canonical source consumer.

A disposable actual-script VM fixture seeds the real ordinary-character quest
defaults and reproduces rejected ownership before the change. Afterward, admitted
sink → exit → map reconciliation marks the last hull dead and the descriptor for
native deletion; an injured survivor retains hull, mast and cannon damage.
Repeated exit, deferred/unadmitted rosters, story/qID actors, companions, stale
group identity and invalid ordinals pass their negative cases. Four native codec
rounds preserve the sink result with an explicit empty OnLoad adapter and zero
script errors. Cargo/power and scene/entity APIs use explicit fixture adapters;
rendered disappearance and reentry remain player replay in `docs/runtime.md`.
Already resurrected map rosters cannot reveal historical unrecorded losses;
this repair does not guess which old living descriptors should be deleted.

## Player movement and follow camera — October 7

`WorldMap::Realize` owns the map frame. It previously updated the follow camera
before ship simulation, then rendered the newly advanced ship against the old
camera anchor. Variable frame duration therefore varied the ship's displacement
from its intended focus even on open water. `worldmap-navigation.patch` now moves
the same camera call after object updates, before removal/events/rendering. Camera
controls, elastic heading, physics, collision, travel speed and calendar are unchanged.
Storm opacity computed during Update reads the preceding camera height; ordinary
fixed-height opacity is unchanged, while a zoom-height change reaches it next frame.

A disposable ASan/UBSan probe compiles the exact camera Move method and map update
block with explicit linear-motion/control adapters. Independent camera ground-focus
versus rendered-ship position is the oracle: alternating 120/30 Hz frames reproduced
0.186328..0.745361 units of anchor error before the repair, below 0.000011 after.
Following with zoom, free camera, pause, per-object update count and unpaused-object
time pass. This proves frame ownership, not rendered smoothness or measured FPS.
The player's current ship jitter report reopens visual acceptance; it does not
prove an AI avoidance defect. Canonical build/install and player replay remain
separate evidence in `docs/runtime.md`.

## Player journey hardening — October 7

The current requested batch evaluates the installed Living Caribbean from the
player's journey: sail the map, continue authored quests, enter a port, buy and
service a fleet, discover events, choose participation, and return for consequences.
An empty port or a temporary shortage is allowed. An unavailable action, permanent
queue, stolen quest actor, duplicated transaction, unavoidable participation,
unreachable return or stale reward is not. Numeric balance and rendered scenes
require game evidence; source or isolated VM checks cannot accept them.

| Coverage lane | Current source owners | Decisive boundary | State |
| --- | --- | --- | --- |
| Trade, supplies and recovery | `worldmap-traffic.c`, `worldmap-recovery.c` | Finite available work; paid service; loading/delivery; depleted/recovering port | Actual-script checks and four codec rounds pass |
| Encounter choice and sea handoff | `fleet-encounter-ui.c`, canonical `src/gameplay/PROGRAM/sea_ai/sea.c` | Plain sea/pursuit/attack; actual roster and surviving state; quest exclusions | Actual-script damage/name/service/admission checks pass |
| Siege, harbour and physical return | `worldmap-military.c`, `worldmap-land.c`, `worldmap-harbour.c` | Sea/fort/city transition; evacuation; ashore flagship loss; terminal cleanup | GM-bound placement, rollback/return and four codec rounds pass; rendered scene unresolved |
| Authored quest and port continuity | `metal_living_caribbean.py`, original callback owners | Existing quest actors/locks; port access; store and day callbacks | Authored admission matrix and four codec rounds pass |
| Discovery, participation and consequences | `worldmap-participation.c`, news/contact callbacks | Truthful rumours; choose/join/leave; finite rewards and reputation | Actual mayor callback, transaction negatives and four codec rounds pass |
| Clock, reload and integrated delivery | Existing saved descriptor/calendar and canonical composers | Prior valid state survives repeated calendar/load/handoff; exact installed package | Whole PROGRAM compilation and integrated dialogue/codec pass; installed verdict belongs to `docs/runtime.md` |

Root owns shared source composition, Git/index, build/staging and installation.
Independent lanes first return observable actual/expected mismatches bound to an
independent contract; a repair follows only that evidence. Existing player saves,
configuration, foreign dirty/staged work and authored scarcity are preserved.
Ordered acceptance is relevant actual-script/serializer checks and their nearest
negative, a focused local commit, canonical `run.sh --stage-only` with the game
closed, and available player scenes. The player's later replay remains a distinct
gate. No automated game launch or rewrite of player saves is part of this batch.

Confirmed state defects have independent owners and expectations:

- The service planner reserved balls even when existing bombs/knippels already
  satisfied the unchanged six-salvo policy, rejecting a physically fitting recruit.
  One shot-count owner now serves reservation and purchase. Full holds, missing
  ammunition/powder and depleted recruitment remain legitimate rejections.
- The global sea flag suppressed every port's paid daily recovery. Only the
  commander actually published by `Fort_Login` is excluded from background
  recovery; remote ports tick once, preserving quest/siege and empty-stock gates.
- Ordinary proximity imported unselected quest/qID/ALONE encounters. Authored
  selection retains control; ordinary selected battle expansion uses one shared
  same-root predicate, including an already committed rescue member in review.
- Fresh sea phantoms replaced the saved name and lost physical gun/mast/sail/blot
  damage. Restoration replaces only persistent damage, retaining native reload
  handles/charges. Unpaid damage survives; existing funded service clears it.
- Neutral berthed ships fired on an assault automatically. Their return fire now
  requires actual hostile allegiance or explicit defender participation. Their
  exposure to incoming damage and harbour loss remains real.
- A late story island reservation allowed ordinary expeditions to continue.
  Map update, arrival and pre-generation sea attachment now yield through the
  existing physical evacuation owner. Pending import is released only without
  a loaded-hull receipt; live actor ownership still vetoes evacuation.
- Dialogue links offered contracts/claims rejected by their action owner, and
  governor quest links overwrote the military slots. Shared pure admission and
  entitlement predicates now serve both links and actions; semantic link keys
  preserve the real `Common_Mayor` quest callback. Local withdrawal permits
  rejoining the existing agreement; departure to the map and betrayal do not.
  Empty treasury/full hold keep an earned claim retryable without duplicate pay.
- Four queued preparation/attack reports suppressed every later outcome. Phase
  progression now updates only its own operation/source report, retaining the
  four-report ceiling, dated scope, unrelated rumours and quest callbacks.

The pre-edit VM reproduced recruit/recovery failures, three authored-admission
failures, seven restoration failures, three encounter-review omissions and neutral
fire/story-yield failures without script errors. The repaired state scenarios and
nearest allowed cases pass; these are not rendered gameplay or balance evidence.

Land placement reuses the existing SHA-bound authored-layout delivery pattern.
`tools/gameplay/land-layout.json` binds 26 canonical town destinations, three fort
rooms and the two real `BOARDING_FORT` locator models. The composer generates
`WdmMilitaryAuthoredLandLayout` and gameplay planning checks every physical GM
asset SHA. An unloaded location is admitted from its authored ID/model tuple,
not transient `Locations.locators`. Loaded coordinate identity and native
`CheckLocationPosition` remain the physical authority before combat. Hero entry
anchors and other characters are reserved; representative counts may shrink to
finite free positions while preserving the troop pool weights. Unknown geometry
retains naval assistance. A native scene is still required to accept placement,
combat, transition and physical return.

The geometry falsifier reproduces both old unloaded/invalid-bank admissions.
Final tuple seeds come from canonical initialization and `MakeCloneFortBoarding`;
loaded coordinates come from raw GM labels, independently of the JSON. All 31
layouts pass unloaded and loaded state checks. PortRoyal's capped projection
retains exact 101/79 troop weights. Occupied/stale/wrong-model rejection, partial
projection cleanup preserving quest actors, explicit withdrawal/return, casualty
and saved-actor reentry pass with four native serializer rounds. Headless entity
and occupancy adapters delimit this evidence; no native character scene was run.
The exact four-file candidate (map UI, sea, worldmap generator and reload) also
passes whole-PROGRAM compilation and the existing events fixture after integration.

Canonical synchronization retains the complete reviewed proposal from planning,
including direct source files, frozen input guards and the final receipt. Applying
the same batch must not admit its old bytes a second time against an already
proposed new hash. The original normal-stage failure was reproduced before the
repair without entering the atomic writer. Both runtime/app proposals now agree;
an independently edited file still fails closed, and delivered content is
idempotent. Gameplay composition bytes are unchanged by this delivery repair.

## Finite trade cycle — October 7

The requested outcome is a world that can have empty ports without creating
permanent queues of unemployed merchants. `worldmap-traffic.c` owns one cycle:
market work admits assembly, funded service prepares the actual roster, finite
loading commits a shipment, arrival credits its recorded destination, and an
unloaded ready fleet reassesses work before physically changing its working port.
The native 32-commerce/60-total quotas are ceilings, not economic demand.

Remaining demand is the authored target minus current stock minus actual living
freight already committed to that store. No speculative stock or second ledger
is introduced. Assembly and empty repositioning yield to an existing unloaded
fleet at the prospective working port. Exact hull/escort fit and one aggregate
dry run of the existing service planner must admit a newborn before map creation;
the live transaction remains the only buyer of supplies and recruits.

A ready fleet without an undelivered shipment reviews the local market once per
game day. After a full day without work it may request a real voyage to an
admitted, unclaimed port with feasible outgoing work. No available work means
waiting, not deletion, teleportation or free goods. A paid shipment retains its
endpoint through refuges and shortages. If that endpoint becomes inadmissible,
an explicit cancelled return sends the physical remainder to the original seller;
the recorded destination stays unchanged and a separate returned receipt closes
the job. Neither a refuge nor the former buyer receives a false delivery credit.
If both endpoints are inadmissible, a recorded salvage port receives the physical
remainder with a separate salvage receipt; the former buyer is never credited.
Empty positioning carries no trade job
and competes for a working berth; arrival releases that claim. Recovery may leave
exports unavailable while incoming supplies still have positive demand.

Acceptance: a sacked zero-export port admits no new merchants; existing ready
empty fleets disperse only to finite work; competing shipments cannot satisfy the
same demand twice; repeated clocks/reload cannot duplicate debit, credit or
navigation. Negatives include player-depleted stocks, ordinary scarcity, a
loaded/diverted shipment, hostile endpoints, dead/quest fleets, and a wholly
inactive market. Isolated actual-script/serializer probes are the decisive state
falsifier; installed harbour behaviour and perceived balance require player replay.
The implemented batch is installed by canonical stage. Actual-script probes
reproduce the old incoming-demand, zero-export admission and eight-day idle-port
failures. Fifteen affected scenarios pass, with eight using four native save/load
rounds; unchanged paid-service/ammunition and quota-fallback scenarios retain
their earlier passing evidence. Final early-export filters also pass the four
affected cases. Whole installed PROGRAM composition compiles with the real VM.

Market demand is projected once into a decision-local object and discarded after
selection/loading. There is no persisted cache, expiry or second stock owner.
A 35-colony/51-good/60-encounter isolated reposition probe measured 12.514 seconds
with repeated shipment scans, 0.0585 seconds with depleted seller stocks after
projection/early export rejection, and 0.1773 seconds with rich stocks. This
measures one script decision, not gameplay FPS. The scripted state checks do not
accept a rendered harbour drain or perceived balance; those remain unresolved.

## Gameplay safety contract — October 7

Ordinary admission is limited to 60 resident fleet markers (32 trade,
16 patrol, 12 pirate), including fleets receiving service. Existing over-limit
saves retain their living ships and stop new admission; it is not a promise
of 60 moving hulls. The native object ceiling remains 80. Quests are excluded.
Traffic cargo/service, military, sea handoff and native navigation retain their
existing module owners; the composers bind those sources to delivery hashes.

| Owner | Reproduced failure | Required result and nearest allowed case |
| --- | --- | --- |
| `worldmap-traffic.c`, service | One unavailable full-repair item blocks affordable work; maximum recruitment cannot fit; paid work is absent from cargo admission | Stock funds partial work once, paid crew/guns reserve physical capacity, completion applies only purchased work; empty reserves cause no free repair or departure |
| `worldmap-traffic.c`, cargo | Seller availability is deducted twice across holds; impossible destinations win; wrong arrival credits stock; residual freight is marked delivered | Finite stock, feasible eligible holds, immutable shipment endpoint, exact arrival credit and retained residual; repeat/reload does not debit or credit twice |
| `worldmap-military.c` | Rejected endpoint, orphan mission reservation or no surviving original hulls can retain an operation | Physical recovery and matching reservation cleanup; ordinary pending navigation and a surviving hull remain valid |
| `fleet-sea.c`, `worldmap-harbour.c` | Stale/unadmitted actors overwrite tombstones; orphan sea ownership hides a descriptor; reusable officer slot is recovered; purchased hull retains an old berth | Existing group/ordinal/admission identity guards; normal map entry releases sea ownership without inventing sinks; only the living original officer and purchased hull change |
| Native traffic and berth patches | Avoidance fades on quests/pursuit/patrol, saved recoil drifts in service, old pursuit survives a new route | Fade only on committed ordinary port approaches; hold every velocity component; successful departure clears steering while a rejected locator remains pending |

Supported prior funded service plans must migrate without another stock debit
or a restarted clock. New partial plans record paid targets and starting state;
damage/casualties acquired afterwards cannot be overwritten by stale targets.
Existing finite stocks, cargo weight, crew reserves and original ship identities
remain authoritative. No timeout despawn, teleport, free stock, enlarged quota
or replacement ship is an acceptable liveness fix.

Verification uses actual native methods and the actual COMPILER serializer in
isolated task-owned fixtures. It proves state and conservation, not an interactive
harbour drain, player save upgrade, balance or FPS. Those remain player replay
requirements; canonical stage-only is the separate installation gate.

`script-vm-probe` is an explicit native diagnostic target, reusing the engine's
compiler libraries and native registration rather than a second compiler build:
`cmake --build experiments/native-metal/.cache/build --target script-vm-probe`.
Run `script-vm-probe <task-fixture-directory> <entry> [0..4 rounds] [after-load-entry]`
from that fixture directory. `cases.c` supplies integer-returning entries (zero
means pass), no `Main`, and `OnLoad`; `OnSave` is optional. The tool writes only
its driver and `.vm-userdata` there. Errors fail even when an entry returns zero.
The bounded pilot used about 10 MiB RSS and four native serializer rounds; cargo,
service and handoff fixtures share the runner but retain independent finite-stock,
physical-fit and identity/tombstone oracles. Fixtures remain disposable; no new
required CI suite or example-specific permanent test registry is introduced.

## Sea strength and ammunition contract — October 6

`WdmTrafficHullPower` uses actual hull HP and installed artillery on the native
`HP + 100 per gun` scale; gun contribution also uses the cannon table's
`DamageMultiply`, intact fraction and ammunition readiness. Hull/sail condition
and crew readiness remain monotonic. Optimal crew is full readiness; the extra
25% crew capacity is not mandatory. Class and trade/war eligibility labels select
hulls but cannot give identical physical ships different tactical power.

The same calculation serves active player/companion ships, live NPC battle sides,
persistent rosters and encounter-panel estimates. A saved `RealShip` and installed
cannon type take precedence over the original archetype. Unknown quest strength
remains unknown. Missing rosters also remain unknown: the former class-only
fallback uses incompatible units. This is an approximate comparison, not a victory
probability.

`cannon-loaded-count.patch` extends the existing cannon-controller writer with
`Ship.Cannons.Borts.<bort>.LoadedCannons`: the number of intact, nonempty guns after
execution, including charges still reloading or awaiting fire. Reload percentage
is not ammunition: empty guns can report a completed reload. Spare charges need
both ammunition and powder; already loaded charges retain their contribution
until fired. The live sea entity owns this count even while `bSeaActive` is false
during teardown; outside that entity, saved counts are ignored.

Sea exit snapshots normalize loaded rounds/powder into the surviving NPC cargo
copy before native unload refunds the live inventory. The snapshot does not
mutate live cargo, and repeated exit cannot refund twice. The shared physical gun
getter accepts canonical or legacy damage attributes, prefers canonical when both
exist, and cannot read beyond the physical damage entries. The former `||` guard
ignored damage in a canonical-only state; normal dual-alias generated ships were
not all affected.

Old ordinary descriptors retain their target/intent. `trafficPowerVersion=2`
invalidates only observations recorded in former strength units on their first
refresh, preventing a scale change from being interpreted as combat losses.
Subsequent refreshes preserve current observations; quests remain excluded.

Disposable native-VM probes reproduce the old loaded-salvo drop from 25.6 to 6.4
and a destroyed canonical-only gun counted as intact. The corrected fixture keeps
7200 before and after loading. Empty/partial guns, no powder, optimal crew,
different armament and same-class hulls, sparse/legacy damage arrays, sunk ships,
NPC sea-exit inventory, unchanged sea/map strength, repeat exit and old-save
observation migration pass. Exact main and installed-baseline compositions compile
without VM errors. Engine compilation passes; player scene acceptance and
installation of the current batch remain pending in `docs/runtime.md`.

Reuse decision: retain the existing per-gun controller, damage getter and fleet
owner. The latest upstream beta4 controller exposes progress but no loaded count;
bounded cannon issue/PR searches found no applicable ready export. See the
[upstream controller](https://github.com/storm-devs/storm-engine/blob/beta4/src/libs/sea_ai/src/ai_ship_cannon_controller.cpp)
and [release history](https://github.com/storm-devs/storm-engine/releases).

## October 5 coherent fleet model — implementation contract

The next batch replaces scene-specific guesses with one fleet descriptor. This
section is the implementation contract, not an installation receipt. The
canonical build and stage-only launcher deliver the engine and script batch;
installation requires the player application to be closed.

- **Identity and composition:** ordinary fleets select their hull roster once
  with the existing nation/class/encounter eligibility rules. Map decisions,
  reconnaissance and sea generation consume that roster; gaining a player rank
  must not silently regenerate an existing fleet. Legacy ordinary descriptors
  acquire a roster on refresh; quests and ALONE retain their authored contracts.
- **Readiness:** derive strength from the entire active fleet, actual hull HP,
  hull/sail condition, optimal crew and usable guns/ammunition. Use the same calculation
  for generated fleets and the player. No player rank or personal health enters
  tactical power. Losses and depleted cargo survive sea exit; sunk/captured ships
  must not return through a fresh encounter generation.
- **Purpose:** commerce travels between eligible ports, patrols defend a bounded
  home territory, raiders search shipping routes for profitable targets. Fixed
  captain temperament changes tolerated risk, not hostility or ship stats.
  Most raiders favour merchants; a small reckless minority accepts unfavourable
  odds. Recklessness is assigned once, never rerolled every second.
- **Knowledge and commitment:** acquire only visible opponents. Persist target
  and attack/escape/route intent, including the strength observed when committing.
  A scene transition alone is not a new reason to retreat. Loss of contact,
  actual losses, ammunition exhaustion or newly arrived superior forces can
  change the decision. Do not force doomed ships to fight or lock quest tasks.
- **Battle sides:** sum fleets actually fighting the same opposing side. Two
  hostile squadrons attacking the player do not evaluate themselves as isolated
  ships. One nearby home patrol can rescue its nation's commerce; unrelated
  neutral traffic and distant fleets are not invisible reinforcements.
- **Map lifecycle:** retain the existing finite population and island navigation.
  The resident-voyage candidate replaces commerce retirement with elapsed port
  service on the same descriptor; patrol/raider routes continue or return for
  actual service. Preserve survivor roster/state when entering
  and leaving sea. Paused play freezes simulation. This batch is a fleet
  simulation while the map runs, not a hidden economy or off-screen cannon solver.
- **Reconnaissance UI:** retain the game's parchment/flag vocabulary. Show each
  observed group once with purpose, visible composition and an approximate
  relative-threat sentence grounded in fleet data. Do not present a guaranteed
  win probability or expose hidden cargo/personality as perfect knowledge.
- **Actions:** entering the sea is independent of choosing a target. The Enter
  quick menu exposes both plain sea entry and targeted pursuit when applicable;
  the encounter dialog names attack, pursuit or approach truthfully. Plain entry
  imports nearby traffic without assigning player aggression. Hostile ships
  already committed to attack remain dangerous; this is not a combat immunity
  button. Barrels/boats, storm/story restrictions and deliberate attacks remain.

Falsification scenarios: a reckless raider follows a stronger player and retains
its attack after sea entry; a cautious raider avoids it before contact; a patrol
plus another attacking squadron evaluates their actual combined side; a crippled
or dry-ammunition ship can escape despite commitment. Enter sea beside friendly
shipping and collect a barrel without selecting attack. Re-enter after damaging,
sinking or capturing a ship: survivors keep losses and dead ships do not respawn.
Old saves receive only missing ordinary fields and keep player/quest state.

Reuse the existing eligible hull selector, group task API, encounter descriptors,
navigation, sea import and UI widgets. Replacing the whole world-map engine or
introducing a parallel AI registry would duplicate those owners without closing
a further requirement. Exact native/script probes and canonical staging precede
installation; the player owns final scene/balance acceptance.

## October 5 resident voyages and service — source verification

`worldmap-traffic.patch` now admits a guarded lifecycle on the existing ordinary
descriptor. `trafficLifecycle=service` stops movement and repeated arrival;
`WdmTraffic_Arrived` hands the same ID to scripts. A saved `trafficNextLocator`
must resolve through the existing native maritime locator owner before `Goto`
commits the next leg, clears that request, increments `trafficVoyage` and emits
`WdmTraffic_Departed`. Invalid locators leave the request blocked in place.
`trafficServiceOnArrival` admits a patrol/raider's actual service return, while
their ordinary search/patrol legs retain the existing reversal. Explicit loss
still retires a fleet; the legacy lifetime cannot expire a resident descriptor.
Quest descriptors retain their authored lifecycle. Old ordinary Follow ships
migrate through the native reload constructor into the resident implementation,
using the same descriptor, coordinates, nationality and roster. A missing old
destination assigns a future refuge and sails there; it does not invent a former
origin or teleport the ship. Expiring/deleted old descriptors are not resurrected.
The script's off-map one-day expiry excludes admitted residents as well.

`worldmap-traffic.c` owns the saved calendar stamps, arrival/departure handlers,
service plan and next ordinary job. Each surviving hull reserves actual Store
planks, sailcloth, matching replacement guns, ammunition and provisions before
its work interval starts. Store Norm and trade classes remain unchanged; service
leaves 15% of the authored stock target. Recruitment debits the existing colony
hiring pool while retaining 25% of its authored refresh, with a minimum
five-person reserve. That reserve remains fixed until `CrewDate` changes;
successive NPC purchases cannot reduce it. Player depletion remains possible.
Small pools
fill a hull through paid, elapsed partial service intervals. Only actual MinCrew
permits departure; readiness still reflects the partial crew. Dead slots remain
dead. Initial assembly uses this same contract, and the sea bridge imports the
paid supplies and actual crew quantity instead of generated cargo/crew.

Work takes at least 24 game hours, with hull/sail damage extending the interval.
The saved plan prevents duplicate purchases, repairs and recruitment after load.
One stock review per game day bounds catch-up; a long skip does not replay missed
purchase attempts or spawn several departures. These numeric service/reserve
values are initial tuning, not measured gameplay balance.

New service plans apply paid partial HP/sail/gun deltas after elapsed work; they
do not assign full health or invent ammunition. Food, paired shots/powder and
MinCrew must physically fit. Pending paid crew/guns count in every cargo-room
check. A late capacity reduction delays completion without an overweight hull.
Legacy funded plans retain their original stamp and paid targets. An unvisited
legacy plan interrupted after casualties has no original crew watermark; it
keeps observed survivors instead of reconstructing unknown lives.

An actual failed stock review blocks new assemblies at that physical port until
paid progress/readiness returns. Existing fleets remain, and another port or quest
is unaffected. This guard does not raise the 60/80 ceilings or guarantee a moving
fleet count. Ordinary trade loading uses at most 75% of total physical capacity,
including crew, working guns and pending service; scarce jobs carry less. Prize
transfer may use remaining space. The 75% ceiling is initial tuning, not evidence
that perceived balance is accepted; wars and demand may still leave empty ports.

The service source is installed; disposable VM checks execute
stock/reserve and recruitment failures, timed repairs, partial hiring, real gun
damage restoration, dead-slot and quest exclusions, coordinate-preserving old
Follow migration, and descriptor save/load without repeated debits. The save/load
probe uses the native serializer with scene hooks replaced only in its disposable
fixture; it does not accept the player's actual upgrade path. A source-branch
probe checks arrival identity, one event, pause, service hold and patrol reversal.
The current ordered patch applies to a preserved source copy, and changed native
translation units pass the canonical compiler's syntax checks. Read-only canonical
script composition admits exactly three changed consumers and the verified prior
installed hashes. Canonical build/staging, installed callbacks and real save replay
remain unresolved. The cargo component below has subsequent source/state evidence;
the military/player/economic expansion milestones remain pending. The first native
compile exposed a missing `core.h` include, now fixed.

## October 5 cargo, prizes and fleet assembly — source verification

The existing per-hull `trafficSupplies` attribute now owns the complete physical
off-screen cargo. `trafficFreight` marks its deliverable subset; it is bounded by
the physical quantities and cannot create a second inventory. Actual sea entry
replaces generator goods from this manifest, including empty holds. Sea exit
copies actual remaining goods back and clamps freight after looting/consumption.
Capturing or sinking a ship clears its old descriptor's cargo without clearing the
separate live captain/ship object used by the capture interface.

Commerce selects admitted buyers using live export/import demand and regional
travel weights. Departure debits actual export stock once, leaves 15% of Norm,
and respects each hull's actual hold after provisions/ammunition. Import demand
admits stock up to 125% of its authored Norm; other demand uses Norm. Several hulls
share that demand bound. A saved cargo job prevents a second load while awaiting
departure. Arrival credits only surviving, physically held freight and removes it
from the hulls. Store Norm, trade classes and ordinary daily replenishment are
unchanged. Initially valuable goods costing at least 200 require two working
escorts; this threshold and demand/reserve values are unmeasured tuning.

Native NPC battle resolution labels captured versus sunk tombstones and emits one
settlement after all participating hull losses. Captured goods fill survivors only
within their remaining hold; sunk cargo and an uncarried captured remainder are
lost. The captured hull stays gone and is not also regenerated in the winner's
roster. Profitable raiders physically return for unloading. Real crew/provisions
use the canonical daily food ratio and saved elapsed calendar intervals; sea owns
its own consumption interval. Repeated settlement, arrival or save/load cannot
credit the same goods again. A battle interrupts existing port work, preserving
paid recruits while folding new wear into a newly funded service interval.

New ordinary fleets reserve the existing map slot directly, bypassing the legacy
hero-rank generators. Existing nation/CanEncounter/type/class eligibility remains;
quests and legacy roster seeding retain their contracts. Small shipping has 1–2
merchants/0–1 escorts; ordinary caravans 2–4/1–2; rare large caravans 3–5/2–3, within
the existing eight-hull bound. Pirate groups range from one to three hulls; four or
five require a returned prize, a fourteen-day refuge assembly interval and an
available recruitment pool, then the same paid readiness/service gate. These
proportions remain tuning candidates. Assembly invokes the actual ship generator
once and saves its values/working gun anatomy without holding a RealShips slot;
MinCrew and capacity therefore cannot reroll on first sea entry.

The sea's actual `Ships[MAX_SHIPS_ON_SEA]` bound is 32, including companions and
already loaded island/story ships. Admission reserves a complete local battle
bundle before generation. A bundle that does not fit is explicitly deferred on
the saved map, preserving hulls/cargo/wear; no roster trimming or partial battle
resolution stands in for admission. Native `SetLiveTime(0)` clamps to one second,
so the new `trafficRetired` guard permits explicit resident losses to expire while
positive ordinary voyage lifetimes remain persistent.

Disposable native VM falsifiers pass actual Store debit → sea restore → remaining
cargo save → destination credit, valuable-cargo/escort rejection, hold bounds,
partial-unit weights, finite capture/prize losses, elapsed food and ammunition,
interrupted paid recruitment, complete-battle capacity admission, actual authored
ship assembly, invalid hull/legacy negatives and native descriptor serialization
without repeat settlement. The quantity oracle transported 35 units, removed 23
in the sea snapshot and credited only 12; a 200-unit prize transferred only 40 into
an existing 10-unit load with room for 50. Current native patch application and
changed translation-unit syntax pass; a source-branch probe verifies resident,
retired-loss, pause and legacy lifetime outcomes. Permanent test delta is zero.
These are source/state results. Canonical build/stage, installed callbacks,
player-save upgrade and actual gameplay/capture/convoy replay remain unresolved.

## October 5 regional shipping routes

Route selection now uses the existing authored island centres as a coarse distance
estimate, while the native pathfinder still sails between verified offshore port
locators. Destination weight is `20 + 80 / (1 + distance² / 400²)`: short voyages
are more likely, but the positive floor preserves interregional trade. Existing
hostility, ownership and encounter eligibility filters still run before selection.
No player position enters departure or route selection.

New departures spread their home ports by `100 / (1 + resident²)`, counting only
living ordinary fleets of the same role whose saved `trafficOrigin` matches that
port. This reduces duplicate home patrols and repeated departure clusters without
increasing the admission cap. Old resident fleets keep their routes and actual losses;
missing origin metadata is not guessed or used to reset their state.

Raiders add a smooth destination bonus from the actual positions of ordinary
merchant fleets: `40 / (1 + distance² / 400²)` per merchant group. Shipping can
attract pirate routes; quest ships, the player and removed encounters cannot.
This changes route preference only, not enemy omniscience or attack permission:
local sight, strength assessment, commitment and the one-patrol rescue cap still
own actual encounters. New origin/destination metadata is saved inside the existing
encounter descriptor, with no second registry or economy.

The design reference was TaleWorlds' documented caravan protection/patrol response
([War Sails release](https://www.taleworlds.com/en/News/587)) and purposeful sea
journeys ([developer update](https://www.taleworlds.com/en/News/598)). Those sources
explain their documented mechanics, not proof that these weights make Corsairs fun.
A larger spawn cap was rejected: it raises the number of actors without giving
them better reasons to cross paths.

Disposable checks compile the changed route helpers and complete creation function
in the native script VM. Seeded VM checks verify nearer-route preference, nonzero
long-distance weight, actual commerce attracting raiders, underrepresented home
selection, unchanged other-role counts, quest exclusion and missing-coordinate
fallback. Exact-hash composition admits the previously installed revision and
rejects unknown edits. Real-game density/enjoyment remains player acceptance:
travel through regional ports after new departures have replaced older traffic.

## Living Caribbean expansion — design, not an installed feature

Requested outcome: resident fleets, trade caravans, robbery, prizes, military
expeditions, fort sieges and port plunder should form one causal simulation with
observable opportunities for the player. The previously installed route weights
are a prerequisite, not completion of this expansion. Implementation and real-game
acceptance of this section are pending.

### Objects and authoritative state

- Fleet: retain the existing encounter ID, fixed survivor roster, cargo snapshots,
  nation, captain temperament, position, current mission and observed opponents.
  Add voyage/service/prize intent to that descriptor, not a second actor registry.
- Port: retain live Colony ownership, fort commander, garrison and Store goods.
  Supply/disruption/recovery state belongs with that colony and its existing
  stock updater; no independent decorative prosperity score.
- Cargo: use one manifest per surviving hull. Store departure, sea import, prizes,
  player looting and destination delivery consume the same quantities. A generated
  cargo reroll cannot stand in for a transported shipment.
- Simulation clock: the existing world-map clock and calendar advance own elapsed
  time. Map pause freezes time. Returning from land/sea advances only the missing
  interval once; saving or entering a scene is not a fresh random resolution.
- Information: visible nearby fleets expose observed purpose/composition; distant
  port reports require a visited port or an ordinary news source and carry an age.
  No exact cargo, hidden temperament, win percentage or omniscient enemy targets.

### National strategies, cooldowns and event chances — proposed

No national treasury, funding accrual, spending envelopes or fiscal simulation.
Each ordinary state keeps only strategy/review date, operation eligibility dates
and active operation IDs under its existing nation owner. Physical fleet readiness,
port admission and actor limits remain separate constraints. Losses persist;
survivors need service time, while replacement fleets appear through bounded
replacement eligibility and receive new identities. No resurrection of dead hulls.
Piratical groups use their own recovery/assembly intervals, not one central pirate
state dispatching a synchronized navy. Real player money, goods and finite siege
loot/rewards retain their existing owners; removing AI budgets does not make prizes
infinite or remove actual crew/ammunition losses.

**Strategy rotation.** Keep a modest national temperament bias, not hard national
combat buffs. Proposed game biases: England convoy/interception, France expedition/
raiding, Spain home/valuable-cargo defence, Holland trade/escorts. These are tunable
playstyle distinctions, not historical assertions or permanent prohibitions.
All states can choose every eligible strategy. Strategies weight eligible event
chances, mission selection and available fleet roles:

| Strategy | Preferred missions | Triggering pressure |
| --- | --- | --- |
| Recovery | Repairs, recruitment, replacing key escorts | Heavy losses or damaged home ports |
| Trade protection | Convoy escorts, threatened-route patrols | Repeated merchant losses or valuable upcoming cargo |
| Home defence | Local patrols, supplies, available relief | Observed siege preparation or current attacked port |
| Interdiction | Enemy shipping interception and bounded blockade | War pressure with insufficient land-assault strength |
| Expedition | Prepare one eligible port assault and its transport | Hostile diplomacy, credible available force, healthy home defence |

Review on persisted 30–60 game-day intervals, with a 14-day minimum hold against
random churn. A major loss or actual home siege can switch immediately to recovery/
defence; save the reason and next review date. Choose weighted eligible strategies,
reduce repeated identical picks where alternatives remain credible; do not blindly
cycle to an offensive strategy while depleted or at peace. Existing voyages complete
or cancel for their actual safety/mission conditions; strategy rotation never
teleports ships, rerolls crews or reverses every task simultaneously. Threat
information must come from observed losses/contacts and owned-port reports, not
exact global enemy/player strength. Do not change authored diplomacy just to create
a siege. A new peace cancels uncommitted attacks and triggers a coherent ceasefire/
withdrawal for ongoing operations; it does not reroll ships or pay capture rewards.

October 5 strategy source candidate, not installed: the existing `Nations`
objects own versioned `trafficStrategy` state. Initial migration seeds a 30–60-day
review, a 75–150-day authorization clock and one weekly decision clock; it emits
no operation or historical catch-up. Canonical calendar stamps and a one-hour
review gate suppress frame/load churn. Owned-port attack/recovery reports and
actual surviving fleet readiness determine eligibility, without reading enemy or
hero strength. At peace, offensive strategies are excluded; expedition preference
requires two ready, uncommitted patrol fleets. A scheduled choice holds at least
14 days, downweights a repeated choice and retains modest national biases.
Actual home attack or major hull loss changes posture immediately and records
its reason. Existing voyages and rosters remain intact. Home selection weights
patrol defence/protection; expedition preference permits a rarer 3–5-hull authored
large-patrol assembly through the same real service contract, never an instant
replacement or raid. This is strategy/readiness behavior, not a dispatched siege.

The exact script composition compiles in the native VM. Disposable state probes
cover migration/no immediate operation, timer range/no repeated roll, a day-14
emergency, the subsequent minimum hold, peace after a year skip, major loss even
when recovery was already randomly selected, pirate exclusion, and a native
save/load without reroll. A failed probe exposed that a nested attribute passed
directly to an `aref` calendar parameter resolves incorrectly in this VM;
`makearef` fixes the strategy hold and the existing pirate-assembly timer.
Permanent test delta is zero. Actual rare authorization/target reservation,
expedition travel, fort/city arbitration and real-player pacing remain pending.

**Rare assault scheduling.** City targets are weighted random among currently
hostile nations' eligible ports; randomness does not override fort/quest admission,
physical travel, credible force or the home-defence floor. Weight distance,
known defence, strategic value, recent target history and viable logistics.
Sometimes decline every target rather than guarantee an attack on the weakest
city. Reconsider unviable targets after a bounded delay; never roll every frame or
reroll on save/load until success. Fix the operation identity and target at dispatch;
changed access/diplomacy can cancel or safely divert it through the same owner.

Proposed initial ranges, explicitly not measured balance:
- Per attacking nation: next major-expedition authorization 75–150 game days after
  its preceding dispatch. Authorization is permission to consider, not a scheduled
  guaranteed spawn. One active major offensive expedition per nation.
- Per targeted port: 90–180 game days between major assault commitments, and never
  before its admitted recovery/protection boundary. A failed assault still consumes
  cooldown, with the range/attempt severity chosen and saved at commitment.
- Across ordinary states: at most two active major operations including preparation
  and voyage; at least 20–40 game days between authorizations. Existing quest sieges
  occupy the affected port/exclusion region but are not rewritten by this scheduler.

Sample ranges once from the saved calendar/operation state. Check cooldown and
readiness eligibility before reserving hulls/target; serialization prevents two
nations claiming the same fort/garrison. Lack of actor slots delays preparation,
not removal of unrelated fleets. Cooldowns do not restrict ordinary piracy,
commerce or existing local battles, which need not be rare. A lost expedition's
survivors and recovery remain active state; a cooldown expiry does
not magically finish them. Longer time skips apply
existing operation transitions without dumping years of newly spawned assaults
around the player in one frame.

**Event chance after cooldown.** Evaluate eligible major expeditions at most once
per nation per game week. Proposed initial probability is 10–20% per eligible
review, adjusted by current strategy within that range; lack of an eligible target
or ready force means no event. Save next review date and each decision. Cooldown
expiry alone never spawns an operation. These values require player pacing tuning.
No frame-by-frame retries, no new roll on load or entering a port, and no backlog
of missed weekly rolls after a long time skip. Ordinary traffic uses its existing
bounded population scheduler rather than this rare-event chance.

### Fleet variety, escorts and pirate groups — proposed

Use the existing merchant/guarded-merchant/patrol/pirate encounter templates and
fixed roster/Ship bridge. One map encounter represents the entire real fleet.
The preceding installed `WdmTrafficCreate` selects three template sizes and includes
player-rank gates. The uninstalled cargo candidate above replaces those ordinary
gates with fixed role composition and physical assembly/service. Preserve quest
rank requirements and authored ship validity. Native/source per-fleet limits must
be verified before committing a composition; actor budget counts fleet entities,
while sea cost also depends on total hulls.

Proposed variety ranges: small traders 1–2 merchant hulls and 0–1 escorts; ordinary
caravans 2–4 merchants and 1–2 escorts; rarer valuable caravans 3–5 merchants and
2–3 escorts, trimmed to admitted hull/scene capacity. Escorts differ by speed,
working guns, crew and readiness. Cargo value/known danger justify their expense;
short safer routes can stay lightly guarded. An escort is a real hull with costs,
ammo and damage, not a multiplier. Keep the full roster in sea import. If escort
support is unavailable, delay a valuable convoy, shrink its load or select a safer
route instead of always generating a suicidal unguarded treasure ship.

Piracy includes single hunters, 2–3 ship bands and less frequent organized bands
up to five hulls where the native limits permit. Large bands require successful
prizes, available captains/support and elapsed assembly, not the hero's level.
Boldness remains per captain/band, while actual whole-fleet strength, loot capacity,
readiness, supplies and nearby observed escorts determine commitment. A five-ship
band can attack a defended convoy without being five elite battleships. Defeat
reduces the real band; prizes can finance repair/crew or bounded future replacement.
Large pirate groups compete for viable prey, disperse/rebase after depleted routes
and return to unload, rather than all selecting the player. No free multiplication
of captured hulls into both merchant survivors and pirate additions.

Small commerce rescue still admits one eligible local patrol. A rare expedition or
relief force follows its own actual mission and arrival, but local battle/sea limits
must arbitrate joins without silently discarding hulls or duplicating crews. Admit
new participants in the existing battle owner, not several overlapping resolutions.

### News through existing tavern/citizen dialogue — source reuse and design

`Common_Tavern.c` and `Enc_Walker.c` include `Rumours/Common_rumours.c` and call
`ProcessCommonDialogRumors`; `SelectRumourEx` chooses existing queued information.
`Rumour_func.c` provides AddSimpleRumour/Ex/City/CityTip with city/nation scope,
lifetime, repetition and event fields, and existing OnSiege branches. Reuse this
queue and dialogue, not a parallel news window or unsolicited dialogue popup.
Some existing rumour events generate merchants/quests when heard; new simulation
news must read an already existing operation and use no generation callback.
Hearing about a convoy cannot create that convoy or extend its life.

Create bounded news records from actual committed departure, first reported
attack, relief, surrender, loss, return and disrupted supply. Attach operation ID,
phase, origin, observation date and knowledge scope; propagate to nearby/connected
ports after a bounded information delay. Departure ports can report preparation,
affected ports report current siege, arriving survivors report aftermath. Do not
broadcast hidden routes, future targets or exact crew/prize amounts globally.
Delay matters: a stale report can describe a convoy already gone, with its age
stated; witnessed current state supersedes it without rewriting history.

Use existing “Что нового?” exchanges. Example information styles:
- Tavern: “Три дня назад из ... вышла эскадра. Говорят, собираются ударить по ...”.
- Merchant/citizen: “После налёта пороха мало; последний привоз быстро разобрали”.
- Aftermath: “Нападение отбили. Остатки эскадры ушли ... дней назад”.
Only fill counts/places/dates actually known to that source. No invented departure
bearing from a hidden current coordinate. Keep normal quest tips available; limit
ordinary simulation news share, expire/update operation phase records and suppress
duplicate insertion. Preserve dialogue exits and repeat limits. Existing legacy
rumour date encoding is not permission to use a second approximate simulation
clock; canonical game calendar owns operation/cooldown/recovery elapsed time.

### Scheduling admission before implementation

Persist a versioned compatible state under the existing nation/fleet/colony owners.
An old save seeds cooldown/review phase once without immediate expeditions or
replayed history. Existing ordinary fleets enter timed service at their next valid
transition without teleportation; pending story operations remain excluded.
Calendar catch-up, event decisions and settlement are idempotent.

Required design falsifiers: depleted nation cannot dispatch an unready expedition;
recovery permits later operations without resurrecting hulls; event checks cannot
repeat on reload/frame updates; two states cannot claim one target; cooldown survives
reload and failed assault; strategy rotates without rerolling underway fleets;
peace interrupts coherently; ordinary piracy continues during siege cooldown;
five-hull band and variable escort convoy import unchanged; hearing news creates
no fleet; a year-long skip cannot generate a burst of active wars. Numeric timing,
event chances and group proportions remain candidates until normal player voyages show
rare discoverable operations and continuous varied ordinary shipping.

### Coupled mission loops

1. **Commerce and caravans.** A port selects an eligible allied/neutral buyer using
   live export/import demand, stock, travel cost and known route danger. Load from
   actual available stock within hull capacity, leave reserves for local/player
   supply, and debit once. A caravan is a real fleet with merchant hulls and
   optional escorts; valuable cargo and unsafe routes increase escort demand,
   with capacity/availability bounds. Arrival credits the surviving manifest,
   unloads, services the fleet and chooses the next job. Cheap local coastal
   traffic remains alongside rarer valuable long voyages.
2. **Piracy.** A captain searches locally, compares whole opposing fleets and
   escort/gun/readiness, estimates attainable loot versus losses and time, and
   chooses shadow/chase/attack/escape. Keep fixed bold/reckless temperaments;
   recklessness raises risk tolerance but does not grant hidden knowledge or
   free combat stats. An escort can be drawn away, a damaged straggler attacked,
   or an overmatched merchant coerced into a limited cargo surrender. After
   profitable plunder or major damage, return to a pirate refuge to sell prizes,
   repair and replenish instead of pursuing indefinitely. No reroll at sea entry.
3. **Patrols and relief.** Home patrols defend their territory, respond to observed
   attacks on their nation's commerce, and escort endangered survivors for a
   bounded leg. Preserve the one-patrol rescue cap for a local commerce fight.
   Repeated losses reduce supply and create bounded demand for replacement/relief
   departures from eligible allied ports, rather than instant reinforcements.
4. **Military expeditions.** Only hostile diplomacy permits an offensive port
   mission. Dispatch requires an available squadron, supplies and credible siege
   strength; it competes with home defence. The expedition physically travels,
   can be spotted or intercepted en route, and can abandon a newly untenable
   mission. A small pirate gang must not randomly bombard a powerful fort.
5. **Blockade, bombardment and plunder.** Distinguish interdicting shipping,
   suppressing fort guns, defeating a garrison and carrying away loot. A blockade
   can succeed without storming a fort. Derive siege power from working cannons,
   hulls, ammunition, crew and the live fort/garrison, with stationary-defender
   advantages; close battles resolve over days with accumulated costs. Relief
   ships join only if they actually arrive. Sack requires suppressed defence and
   surviving landing strength/cargo capacity; bounded loot comes from the port.
   Failure costs ships/time/supplies and sends survivors home. Ownership changes
   need a separately admitted full conquest/quest contract; ordinary successful
   raids damage supply and recovery, not authored story availability.
6. **Aftermath and recovery.** Destroyed/captured hulls stay gone. Surviving fleets
   need elapsed service time and available port supplies to regain readiness;
   entering a scene must not restore damage. Disrupted ports recover through the
   existing stock/garrison update and successful deliveries. Scarcity must remain
   recoverable: retain local reserve/recovery floors and cap simultaneous raids,
   so a few raids cannot permanently starve every shop.

### Player jobs and consistent scene transitions

- Observe a convoy, identify its escort, shadow it, intercept it or let it pass.
- Join an ongoing commerce fight on the same observed sides, rescue survivors,
  take a prize, or exploit damaged winners; no freshly regenerated opponents.
- Enter an eligible port sea scene during a blockade/siege: import the same
  attacking fleet, damage/ammunition and mission, and bind its actual target.
  Entry and hostility follow the encounter/fort and existing nation contracts;
  loading alone must not invent a new allegiance or guaranteed safe harbour.
- Relieve a port, defeat raiders, deliver scarce supplies, or wait until a shipping
  route becomes safer. Effects must be evident in actual goods/defence and the
  player's ordinary store interactions, not only in a log message.
- Hear a dated report about a disrupted port through ordinary port/news surfaces.
  Map icons summarize observed convoy/combat/port activity without flooding the
  screen. Use existing encounter review; no new omniscient dashboard.
- Preserve direct sea entry, deliberate pursuit, barrels/boats, story encounters
  and save compatibility. Quest fleets do not become economic actors or receive
  autonomous targets, cargo changes or service resets.

### Revised game-compatible siege contract — design, not installed

This replaces the earlier free-roaming battlefield proposal. The game has compact
location chains, not tactical town districts. Source discovery in
`PROGRAM/scripts/colony.c` establishes `PrepareTownBattle`, the authored
`NullCharacter.GenQuestFort.<city>.next*` capture chain, up to nine principal
fighters per side per location, additional bounded musketeers and scripted squadron
support. It scales actual manpower into representative fighter counts using
`PlayerCrew_per_char`, rather than spawning one character per soldier. Current
`FightInTown` groups primarily target the hero, and `AfterTownBattle` clears the
hero capture state. Those transitions require adaptation for two autonomous sides;
they cannot establish neutral visitors, autonomous defence or persistent NPC siege
acceptance unchanged. `Default.BoardLocation` is only an authored approach field,
not evidence of a safe route, sufficient room or a supported battle in every town.

**Assault threshold and sequence — agreed design.** Once more than half of a
fort's installed cannons are destroyed, both player and NPC attackers may initiate
its land assault. Exactly half is insufficient. Use the actual cannon counts and
persisted destroyed state, not a hull-HP approximation or a new roll on entry.
Meeting the threshold permits assault; it does not force it or itself win the fort.
Surviving landing strength and the existing encounter/quest admission still apply.
The combat sequence is fort-supported harbour naval combat → land battle at the fort → land battle
in the city's admitted capture chain → settlement. If the defenders defeat the
attackers in the land-fort battle, the assault ends in a defensive victory; do not
advance to city combat. If attackers win that battle, survivors enter the city
stage. A town genuinely without a fort skips cannon threshold/land-fort combat and
uses its admitted city capture stage after any actual naval opposition is resolved.
An absent compatible scene is not equivalent to an absent fort. The land-fort scene/transition must
be discovered and connected before claiming implementation; this contract is not
proof that every town already has that scene.

October 5 source prerequisite, not installed: the same native `AI_FORT` now
persists every physical cannon's damage under its commander
`Fort.Cannons.Damage.gunN`, alongside the unchanged native sea-save stream.
Scene creation restores those values after scanning the authored cannon locators;
hit, save and load paths refresh the per-fort destroyed count. The script count
and spyglass read that fort's state, rather than the former global damage counter.
`Fort_CanLandAssault` requires a positive actual installed count and strictly more
than half destroyed. The existing cannon-hit transition uses that gate, retaining
ordinary fort boarding rather than treating the threshold as a won land battle.
Partial fort crew losses survive scene re-entry when its guns have actually been
hit; an intact fort retains the existing initialization path. Existing admitted
6/8-day fort resurrection clears cannon damage with the restored defence.

The ordered traffic patch applies on eleven native paths and the changed fort
translation unit passes canonical syntax. Disposable probes execute the actual
persistence helper with fractional/destroyed guns, repeated restoration, independent
forts and damage bounds. The exact whole PROGRAM compiles in the native VM;
50/100 rejects, 51/100 admits, another undamaged fort remains at 100, and native
script-state save/load preserves the separate counts and fractional damage.
The real land-fort entry owner is `ships/ships.c:GetShipLocationID`: a Fort
hull resolves its actual `FindIslandReloadLocator(...).go` and calls
`scripts/ShipsUtilites.c:MakeCloneFortBoarding`. This copies that port's original
FortV/FortVRight location into the existing `BOARDING_FORT` slot, replaces ordinary
locators with the authored lAttack asset, removes ordinary reloads and advances to
`Boarding_fortyard`, then one of the two existing bastions. Five actual GM locator
files provide separate `rld.locN`/`rld.alocN` banks: 25 positions per side at the
outer fort and 16 per side in yard/bastions. These are geometry/admission evidence,
not a playable two-sided siege. Preserve port geometry and bind sides before load;
reuse this clone/template path rather than a free-roaming battlefield or fictitious
fort destination.

The current boarding state cannot be reused unchanged for defender participation:
`LAi_StartBoarding(BRDLT_FORT)` forces `isMCAttack=true`, derives manpower from hero
ships, restores officer HP and owns global boarding transitions. Its missing-fort-
location fallback sends `FORT_CAPTURED` and opens loot, so a missing compatible
siege scene must reject entry rather than invoke that fallback. New finite mission
representatives need their own saved troop/phase owner while reusing geometry and
existing character/group primitives. `Loc_ai/LAi_events.c:LAi_Character_Dead_Process`
is the casualty boundary before logoff/reincarnation/group completion; suppress
rebirth for admitted mission representatives and consume their saved weight once.
The following `NullCharacter.GenQuestFort` chain is likewise hero-keyed and cannot
supply autonomous defender state unchanged.

Actual berthing belongs to `pchar.location.from_sea`, written by
`reload.c:SetPlayerShipLocation` and the real sea-to-land reload transition; the
current town/flag alone cannot locate exposed hulls. `AIShip.c:ShipDead` schedules
normal game over for the hero's aboard flagship; a background ashore outcome must
use a separate finite hull disposition without invoking that path for the hero.
`AISea.c:SeaAI_SwapShipsAttributes` is the existing Ship-object transfer owner, but
its curshipnum handling assumes native ship-array participation. Physical ashore
reassignment must bind that requirement before using it and restore a displaced
companion captain through the existing officer owner once.

Connecting two autonomous sides and real player admission to the fort/city chain,
casualties, naval danger and accessible shipless recovery remains active military
work; the owners above are discovered, not implemented feature acceptance.
`tools/gameplay/fort-layout.json` pins the 23 installed maritime fort locator
assets, their SHA-256 and separate cannon/culverin/mortar label counts. The existing
living composer owns the file's exact hash and emits the script lookup; canonical
sync verifies those exact physical RESOURCE inputs before preparing changes.
Counts follow native `RDF_LABEL.name` and `ScanFortForCannons` prefixes, with the
commander's enabled cannon types applied at runtime. An independent read of each
GM matches the catalog; all 23 match the installed application. Unknown layouts,
contradictory saved counts and an old boarding/dead state without cannon evidence
remain unknown, not fortless/intact. Nation readiness uses the same bound live
fort counts and identifies materially suppressed home defence. Actual VM probes
cover all/subset cannon types, preserved prior damage, unknown/contradictory state,
genuine HasNoFort, and the national defence report. A changed locator file is
rejected before delivery. This catalog contains source metadata, not cloned assets.

Canonical build/stage, old player-save migration and real cannon/boarding replay
are unresolved. These checks do not accept an expedition or playable land defence.

**Two resolution modes.** Without a player in a land fight, use a simplified
strength-and-loss calculation with saved stage timers, not unloaded NPC simulation.
The agreed autonomous siege duration is 10–12 game hours from its active attack,
excluding voyage/preparation. Persist current phase, remaining forces and elapsed
work. Stage outcomes depend on force/readiness and valid assault threshold; time
alone is not victory or a deadline that makes capable attackers abandon. A town
without a fort skips that phase; duration allocation must follow its actual chain.
A joining player enters the current fort/city stage with remaining forces.

Foreground combat replaces that stage's timer outcome. Defending victory at the
fort ends the assault; attacking victory advances immediately to city combat,
without waiting remaining background hours. At the city stage, actual victory
settles the current assault. On jungle departure, resume simplified resolution
from remaining forces/stage work once, not from a new full-duration siege. No
background casualty calculation runs for the same active land fighters.

**Player participation occurs before combat entry.** Use the observed encounter
review to select naval intervention or an eligible land-side agreement. Helping
attackers means joining their current capture stage, not teleporting to a freely
chosen street. Helping defenders enters an explicitly admitted defender scene at
its authored entry with relations set before combat initialization. No governor
conversation while walking through an already hostile melee is required. A
commander/contact dialogue outside the active fight, where an actual contact exists,
can provide the same agreement. If the defender entry or two-sided scene has not
been proven for that town, offer naval help only; do not claim a playable defence.

**A siege starts while the hero is ashore.** Check actual player-fleet berthing,
not just current town/flag. If the fleet is moored in this attacked port, a sailor
notification/dialogue at the next safe interaction boundary offers staying for
land defence or returning to the ship for naval defence of the fort. During an
existing dialogue/load/quest transition queue it once rather than interrupting
with competing dialogue. Staying leaves the fleet berthed and exposed to naval defence resolution; it
is not immunity. Returning imports the real fleet into the current sea operation
through the admitted transition before autonomous resolution consumes it. One
choice per operation persists across save/load. The sailor explicitly warns:
“Если форт и корабли охраны не отобьют нападение, мы потеряем суда в гавани.”
Queue the choice before resolving danger to the moored fleet; no retroactive choice
after already computing its destruction.

**Harbour clearance precedes landing.** Attackers engage actual hostile ships in
the fort combat vicinity plus a bounded manoeuvre margin; allied/neutral ships do
not become targets merely by proximity. Determine the extent from the real fort
range and admitted sea positioning, not an arbitrary global deletion radius. Fort
guns support the naval defenders during this phase. The operation must defeat or
drive off effective naval opposition before committing its exposed landing force.
Bombardment may occur during the same sea fight; do not disable fort fire until
some artificial naval phase ends. Then the more-than-half-cannons threshold permits
land assault, followed by the city stage.

**Berthed player fleet shares the naval outcome.** If the hero declines sea entry,
resolve actual fort and defending ships, including the exposed berthed player
ships, through the admitted background sea owner. No automatic player piloting or
full free readiness is invented for moored ships. If naval defence repels the
attack, preserve its surviving ships and recorded damage. If attackers defeat the
fort-supported naval defence and secure the harbour, the player's ships still
berthed there are lost through recorded capture/destruction, before the city result.
An eventual player land victory cannot resurrect sunk ships. Captured ships can
only be recovered if they still physically exist and their captors/control are
actually defeated; they cannot also be counted as destroyed or duplicated prizes.
Ships demonstrably elsewhere are unaffected. Jungle withdrawal grants no automatic
evacuation. Persist each hull outcome once with its crew/cargo; entering sea later
imports only actual remaining ships. This supersedes the earlier protected-player-
fleet proposal. The background owner and normal save/sea import must support these
outcomes before delivering this choice; no silent removal of the entire squadron.

**Flagship loss while the hero is ashore.** Losing the flagship does not kill an
ashore hero or delete surviving companion ships. Each hull keeps its actual
location, control, damage, crew and cargo. Once the hero can physically reach a
surviving player-controlled ship, select it as the new flagship through the normal
ship assignment owner; its former companion captain returns to the officer pool
without duplication or loss. Until harbour access is restored, the interface cannot
teleport the hero aboard or mark a captured ship as available. Preserve the existing
ordinary defeat when the hero is aboard the sinking flagship; no automatic jump
between companion ships during combat.

Cargo and cabin-chest contents of a sunk flagship are lost. Other ships keep their
own contents; capture preserves goods only on the actual captured hull until real
recovery. Officer disposition follows actual location, not the flagship pointer.
The ashore survivor path must support saving, loading and a working sea return
without a flagship; a surviving accessible companion must prevent a no-ship softlock.
If all hulls are lost, use an admitted shipless recovery path rather than inventing
a replacement ship or an unusable sea command. This remains an implementation gate.

If the hero visits a hostile town without actually being berthed, there is no
fictitious sailor warning from ships in its dock. The current assault can appear
when exiting an interior into the admitted exterior: hostile land groups follow
the same stage and relations, with no automatic governor alliance. Queue scene
population at transition, never replace actors mid-dialogue. A currently loaded
exterior needs its supported staged actor admission; this is an implementation
gate, not permission to freeze the entire operation. Prior protected story actors
remain governed by their existing contracts.

**Attack the fleet while its soldiers are ashore.** The sea branch remains useful
throughout the assault. Transports have their assigned aboard crew; covering ships
retain gunners/sailors, not a uniform fleet-wide reduction. Cover responds to
observed player aggression. Transports can seek cover or recall troops through an
actual valid embarkation transition. Crew on shore cannot fire ship guns or join
a naval boarding defence; capturing a ship transfers only its aboard people/cargo.
Shore losses and naval losses debit different portions of the same survivor pool.

**No stranded-raider loophole.** Throughout the expedition, including after town
victory and during loot loading, loss/capture of every usable evacuation ship and
route makes the surviving shore force surrender deterministically at the next valid
combat transition. Capturing the town does not grant an occupation-base exception,
replacement fleet or an exemption from evacuation. The expedition cannot continue
holding the town after its shore force has surrendered; reconcile local control
with the actual accepting force. Prior deaths, damage and goods already lost remain
recorded rather than rolling the operation back.
There is no random refusal, invisible rescue fleet, endless street combat or
teleportation aboard. Emit one surrender result and stop its fighting groups;
pause and scene synchronization still apply. A remaining ship counts only if it
can actually reach embarkation and transport people; an inaccessible hulk does
not prevent surrender. Insufficient capacity permits evacuation of a finite
number, with the rest surrendering. Do not award the hero every surrendered person automatically:
prisoner handling requires the actual accepting force and available capacity.

**Governor entry respects landing access.** During the naval fight, direct
harbour landing to seek the governor is available only to a hero genuinely neutral
to both fighting sides under current relations, not merely displaying a neutral
flag. Otherwise the hero must use an existing accessible shore/jungle connection;
no residence teleport through a hostile harbour is offered. Reaching the governor
uses the admitted actual town/residence path; any special residence dialogue
transition starts only after legal location entry, not from a remote encounter
button. If the governor is captured/dead or the scene is incompatible, do not
resurrect him to offer help. Neutral visitor access does not itself join either
side or move the fleet out of danger.

**Neutral helper's fleet: decision before violence.** The governor distinguishes
an offer to help from confirmed participation and explains the real location of
the player's ships. The hero can return to sea to defend/extract the fleet, secure
it manually at another actually accessible anchorage then return overland, or
commit to land defence leaving it in this harbour at risk. No automatic relocation,
AI evacuation or invulnerable neutral hulls. Ship safety must follow real position,
not a flag changed in dialogue. The agreed defending hostility/safe-conduct overlay
begins at participation commitment; hostile attackers identify the participating
player and exposed vessels through the normal operation relation/target owner.

A land hit cannot directly destroy/capture every player ship or call a blanket
fleet-loss handler. Intentional engagement changes local participation/hostility;
actual naval damage or capture requires the ongoing sea outcome and exposed ship
positions. While the harbour defence remains viable, the fleet can survive; when
hostile attackers secure it, exposed player ships are lost under the agreed rule.
If the harbour is already held by attackers, explicitly warn that committing from
land with ships still there leaves them capturable; do not offer a fictitious safe
naval-return window. The same risk applies to an uncontracted deliberate attack.
This removes the surprise first-hit deletion without granting fleet immunity for
choosing land participation.

The governor offers assistance against the current assault, the finite reward,
and local safe conduct. Agreement temporarily allies the defending siege garrison
with the hero/party and admits their land participation. It also suppresses this
colony's defensive hostility toward the participating player fleet while the
agreement is valid, regardless of its displayed flag; it does not make every ship
of that nation friendly, silence enemy attackers or legalize unrelated crimes.
Implement a scoped effective relation overlay with siege ID and participant IDs,
not deleting nation/crime/quest attributes or setting blanket AlwaysEnemy false.
Keep prior relations underneath for restoration. Relevant uncommitted personal
hostility can be suspended locally; incompatible story locks remain explicit
entry exclusions, not surprise betrayal after the governor agreed.

**Safe conduct ends at global-sea departure.** After successful relief, defenders,
local guards and the fort keep the hero/party/fleet friendly until the first actual
transition to the global world map. Ordinary town/shore/cabin/local-sea transitions,
changing ship flag, saving and loading do not end it. This removes a surprise
thank-you-then-attack and avoids a countdown while collecting the agreed reward.
Scope is this colony and participating defenders only; unrelated enemy fleets
still apply their real relations. The governor states the exact boundary.
Leaving the global map and immediately returning does not restore the old agreement;
a new siege needs a new agreement. Betrayal, deliberate defender attacks or civilian
robbery revoke protection early. A stray hit follows the bounded warning rule.

**Decline and independent entry.** A refusal is not an attack or a new national
crime. End the dialogue normally and disclose that no defender safe conduct was
accepted. Provide an explicit return to the prior eligible sea state; if the player
chooses to stay/go out into town instead, restore original effective hostility at
that transition, with no immediate ambush inside the protected dialogue. If both
sides were hostile, both can attack there; they continue fighting one another and
prioritize immediate threats, not share omniscient hero coordinates. If one side
was already allied, refusing the governor does not make that side hostile. Exit
placement and actor relations require the actual capture-scene admission; do not
promise a generic three-way crowd beyond the proven town actor budget.

**Attacker cooperation is narrower.** A hostile expedition does not accept an
unknown enemy merely because he asks to help or has a matching displayed flag.
An already allied, verified cooperating or authored privateering party may join
an admitted attacker-side scene; otherwise naval/independent intervention applies.
A defender-help agreement cannot also earn attacker rewards. No automatic enemy-
of-enemy alliance, citizenship or national pardon is implied by a local agreement.

**Flagship contact and voluntary attacker assistance.** Separate permission to
visit, combat allegiance and entitlement to payment. Offer a boat/contact action
to the real expedition flagship when its commander is alive, interaction is safe
and the current local-sea contact mechanism supports it. This is a requested
interaction, not a proven boat/cabin implementation. Do not create a second
commander or reset the flagship's damage/task. During immediate combat the visit
is deferred/unavailable; it is not invulnerability or free boarding of an enemy.
A hostile flagship requires explicit accepted parley before transfer. Rejection
returns through the admitted contact exit without teleporting into its cabin as
an enemy. Verify the actual sea contact owner before claiming this action works.

A commander evaluates genuine current national allegiance, a valid patent for
that nation, mission eligibility, prior betrayal/crimes and useful available force.
Displayed flag alone is not credentials. Existing allied players can be declined
for a paid contract without losing their existing allegiance. Eligible privateers
can receive a contribution-based prize agreement; unlicensed allies may be offered
an explicit fixed job or unpaid participation. Hostile visitors can be refused;
there is no requirement that every expedition recruit every available hero.
Show terms before accepting and persist one agreement per siege/player.

Once the fort is suppressed and an admitted landing/capture stage is accessible,
an already allied hero may enter the ongoing land fight without first visiting
the commander. Do not force the governor negotiation branch on attacker-side
participants. Attacking soldiers remain allied based on real relations, even
without a paid contract; defenders remain hostile under the siege participation
rules. Finite capture representatives and the current stage do not regenerate.
An unaligned hero does not gain attacker friendship merely by shooting defenders.

Payment modes:
- Accepted contract: its bounded share/payment follows measured sea/land support,
  troop losses, agreed objectives and operation outcome, not the final blow.
- Valid participating privateer of the attacking nation, no prior contract:
  permit a modest post-operation assistance claim for material contribution.
  This is a chosen game reward rule, not an asserted historical/legal entitlement.
  It uses a smaller bounded allocation than contracted assistance, from the same
  finite distributable prize pool; prior agreement and automatic claim cannot stack.
- Allied unlicensed volunteer, no contract: no automatic money or loot share.
  Acknowledgement/reputation can reflect material help; no hidden promise of pay.
- Late spectator, noncontributor or betrayer: no assistance claim. A patent gained
  only after the fight or switching flags cannot retrospectively create one.

Independent naval prizes retain their normal admitted prize/loot rules; a town
assistance share cannot count the same captured cargo twice. City treasury/store
remain under operation settlement, not free looting by every allied volunteer.
Save contribution and patent eligibility when it occurred, and settle once at
the real expedition commander or its admitted replacement. If the commander dies,
do not resurrect him or erase contributions; settlement waits for a valid surviving
command/payer. A failed assault with no recovered prize cannot manufacture a share.

**Uncontracted outcomes.** Defeating attackers while leaving defenders intact is
relief, not player conquest: the governor may acknowledge material aid and offer
a bounded discretionary reward/settlement. A parley after help can explicitly grant
local safe conduct; no invisible retrospective alliance is promised. Defeating
both land forces is instead an independent seizure attempt, never a defence reward.
Admit it only through the real conquest/residence settlement path with sufficient
remaining player force and compatible quest/ownership transitions. Merely killing
all currently visible representative fighters does not eliminate unrepresented
reserves or win the entire siege. A defeated governor can negotiate surrender,
ransom or an admitted transfer; he cannot pay both a rescue bonus and unlimited
conquest loot from the same finite stock/treasury.

**Offshore survivors do not disappear on land victory.** The expedition reacts to
its actual surviving landing force, viable ships, supplies, current sea opponent
and mission. Loss of its entire assault force normally aborts this assault; capable
survivors recover troops and withdraw when a route permits. Trapped ships surrender
under an admitted surrender rule or fight; a still viable naval force may hold a
blockade/bombard rather than surrender merely because the player killed a small
land detachment. Entry/exit keeps its actual damage, crew, ammo and cargo, never
spawns fresh ships or awards loot for ships still afloat. Local land capture cannot
automatically grant the hero naval victory or make the expedition vanish.

Consequences follow actual actions: repelling attackers harms their operation;
attacking the garrison/conquering its city harms the defending nation too. No
arbitrary “maximum enemy of both nations” penalty simply for entering. A genuine
independent conquest can produce both nations' hostility and finite ransom/prizes,
but also casualties, supply costs and unresolved offshore danger. Exact diplomatic
penalties reuse admitted crime/conquest owners and remain balance work.

Use bounded representative fighters fitted to the authored capture stages. Retain
actual troop pools, their representative weights, stage and casualties in the siege
record. Do not reinterpret nine visible fighters as the entire town garrison or
as nine individual soldiers when calculating all campaign losses. Loss conversion
must follow one admitted weighted contract consistent with the existing capture
model and each side's starting strength. Scripted extra helpers are not free troops.
Save/load restores the same surviving representatives and pending stages, not a
fresh army. No continuous reserve waves or invented street control system. Only
admitted authored entries can place both sides without overlap, immediate unfair
player damage or obstruction; port-by-port entry checks precede delivery.

Foreground capture owns its current fight; background resolution handles only
unobserved forces/intervals. A sea intervention can affect an unresolved landing,
but cannot double-resolve a land stage previously played by the hero. Surrender,
phase completion and return-to-sea must reconcile one siege ID and survivor ledger.
Leaving the operation into jungle ends the hero's active participation and resumes
autonomous resolution from its remaining state as game time passes. Persist earned
contribution, revoke current participation orders and reconcile surviving officers/
representatives; do not restart actors or settle a victory at the exit. This does
not itself end governor safe conduct, whose agreed boundary remains global-sea
entry (unless betrayed). Capture reload locks must admit this explicit withdrawal
path where the authored jungle connection exists. Do not invent an escape route in
a scene with no connection; disclose and implement its actual return/withdrawal
transition before offering participation.

**Loot and compensation.** A raid loads actually available money/goods into
surviving hull capacity over time, after supplies and evacuation needs. Loading
can be interrupted by arriving relief. Departing cargo remains the same sea
manifest and can be recovered by interception. A defence or attack agreement
states a finite payment/share, settled once by contribution and outcome; no
hero-capture helper automatically pays the player for an NPC victory. No double
reward for switching sides. Service/reputation benefits reuse admitted existing
mechanics. A destroyed transport loses aboard loot, not uncollected town stock.

### Visual presentation within existing surfaces — proposed

| State | Map / encounter review | Local sea / admitted land scene |
| --- | --- | --- |
| Approach/blockade | Visible fleet approaches or holds the port; observed blockade state | Harbour approaches covered, no fake town fire |
| Naval assault | Port operation marked as fort attack | Batteries engage the actual fort; return fire, damage and smoke follow actual combat |
| Landing / land assault | Same operation marked as landed assault | Ships remain offshore in cover/transport roles; admitted capture chain contains the bounded battle |
| Intervention | Available naval action and any admitted join-side action named before entry | Player enters the selected supported encounter, never an invented safe district |
| Lost evacuation | Report stranded attackers and surrender once observed | Eligible shore fighting stops; no instant deaths or magical crew refill |
| Loading / departure | Temporary port stay then real departing fleet | Troops return where possible, loot loads, surviving ships leave |
| Recovery | Observed aftermath, dated ordinary news | Store stock/services recover; retained damage follows admitted visual assets |

Keep one contextual operation marker and short observed status, not icons for
every invisible platoon. The reduced aboard crew uses actual ship performance;
do not change hull/sail appearance to mean absent soldiers. Do not require extra
deck crowds, animated troop boats, new districts or white flags to communicate a
state. Reuse compatible effects/actors; asset-dependent extras remain optional
until proven. Smoke/fire cannot claim buildings which have no damaged variants.
Rumours distinguish preparation, fort attack, land assault, surrender and departure,
with source/date; they are reports of this state, not additional random events.

### Revised trade recovery — preserve authored specializations

Retain each town's commodity Norm, import/export/contraband classifications,
base costs, commerce perks and daily replenishment identity. Imported commodities
continue to regenerate too: Norm recovery represents ordinary production and
background supply not individually drawn on the map. Visible caravans add actual
finite shipments and short-term opportunities; they are not the only source of
all imported goods. This is intentionally a bounded hybrid economy, not a closed
simulation claiming every supply unit has a visible ship.

Current `UpdateStore` restores approximately one seventh of the stock-to-Norm gap
per day, plus bounded random variation; it is not a fixed seven-day full refill.
Ignoring random variation, about 34% of the original shortage remains after seven
days and 12% after fourteen. `AddPriceModify` already reacts to stock/Norm within
bounds, and `GetStoreGoodsPrice` already combines base cost, trade class and skills.
Reuse those owners. Do not change marker goods, replace all daily recovery with
caravans or add an unrelated permanent scarcity multiplier.

**Recovery stages.** Ordinary towns keep the current recovery speed. A blockade
reduces positive refill for affected imports/ammunition, never stops all goods.
A sack debits actual stock and applies a finite disruption period; a proposed
initial tuning is 14–45 game days based on the actual goods lost and damage:
limited plunder 14 days, substantial sack 30, severe sack 45. These durations are
proposed map-scale starting values, not measured or proven balance. Recovery starts
at 25% of ordinary positive refill and rises continuously toward ordinary speed
through that period; imports, exports and marker goods retain authored identities.
A supply caravan immediately restores its actual goods, but does not instantly
repair damage or cancel the whole period. An active blockade keeps its separate
bounded access penalty; ending it cannot erase remaining sack disruption.
This is a tunable starting model. Ending the blockade removes
its continuing slowdown; sack recovery runs out by elapsed game date. Positive
refill is scaled once at the existing daily update, not at every location visit.
Negative correction of surplus retains its ordinary behaviour. Preserve residual
recovery and essential supply access so the player cannot become permanently stuck.
Multiple attacks may extend bounded disruption but cannot stack slowdown below
its floor or multiply prices indefinitely. A delivered caravan credits real stock
once and reduces shortage now, while the remaining deficit continues to refill.
No simultaneous full hidden replacement of the same named caravan shipment.

**Prices and protection from confusion.** Define a normal local unit quote from
authored cost, trade class, restored normal modifier and the same player's skills.
That is the comparison baseline, not raw Goods.Cost (contraband and trade classes
already differ). Reuse the stock-driven price modifier with a final bounded crisis
adjustment only if measured existing response is too weak; never double-count the
same shortage. Ordinary disruption should remain below 2x that normal local quote;
3x is the absolute proposed ceiling for the worst shortage. Delivery and gradual
refill reverse scarcity; no instant reset on capture, scene load or siege end.
Buy/sell spread and rounding must prevent same-shop round-trip profit. Show the
current actual transaction total, and keep journal prices dated estimates; a
profitable destination is not a guaranteed buyer at the last recorded price.
Do not silently change a committed total or confiscate a stranded player cargo.

Retain existing quantity/pricing transactions initially. A new bulk-price curve
is deferred unless a measured trade exploit or balancing need justifies it; no
exchange-market redesign is required to make this world lively. Treasury goals
remain final stock targets, respect capacity and cash reserve, and need an admitted
player price/spend limit before automatic crisis-price purchases. Unmet orders
report stock/access/price/cash reason once. Chest-sale rarity cannot restore the
rejected automatic equipment-protection veto; keep player-owned sale rules.

### Required scenarios and remaining admission gates

- A normal trade voyage preserves town specialization and a profitable route;
  disrupted imports refill more slowly, then converge normally after the period.
- One actual caravan arrival credits its surviving manifest once; saving, entering
  a store and returning to the map cannot repeat shipment or daily recovery.
- Maximum shortage respects the normal-local-price ceiling for every trade class
  and permitted skill/perk combination; immediate buy/sell cannot manufacture money.
- Fort attack uses the actual fort target and no fictitious unguarded harbour entry.
- Both fighting groups fit the town's existing capture chain and representative
  manpower contract, with player allegiance set before any active actors appear.
- NPC shore troops without any real evacuation surrender; a partial evacuation
  accounts for capacity and leaves the remainder, not a free full-crew return.
- An ashore hero loses the flagship while a companion survives: save/load preserves
  the survivor, its captain and cargo; restored physical access permits flagship
  reassignment and sea return. Enemy-held access rejects transfer. A hero aboard
  the sinking flagship still receives ordinary defeat.
- Player naval intervention and land participation share casualties, loot and
  phase; background combat never resolves a foreground stage a second time.
- Each town admits defender entry, withdraw/return and protected quest actors before
  offering those actions. Unsupported towns keep naval intervention only.

These requirements correct the earlier unsupported free-roaming/fully closed
trade proposals. Native fort targeting, NPC capture ownership, representative
loss conversion and each offered land-side entry remain unimplemented and need
source/runtime proof. No playable defence, complete town coverage, numeric balance
or installed siege expansion is claimed by this document.

### Balance and causal model

Availability, distance, cargo value, observed force ratio and prior route losses
are continuous inputs. A riskier/high-value job should demand more support;
stronger effective defence should make raids less attractive and costlier.
Raider success should make a route less safe until patrol/escort/relief responds,
while merchants can switch to a viable alternative. This is a feedback loop with
bounded pressure and decay, not a player-rank-driven difficulty spawn.

Do not promise constant combat. A powerful player squadron can deter weak gangs
and choose profitable dangerous waters; reckless rivals, equivalent forces,
contested convoys and military operations provide meaningful optional danger.
Do not add global AI knowledge or ensure an enemy victory/loss for spectacle.
Maintain the finite actor budget and one authoritative battle resolution per
encounter. Reports/icon density and regional activity are separate from combat
frequency; increasing all spawn rates is not this design.

### Source discoveries and reuse verdict

- Native `WdmMerchantShip::KillTest` currently deletes commerce on arrival and
  reverses patrol/raider legs; `WdmEnemyShip::Update` ages fleets out. Rework these
  into service/new-voyage transitions for ordinary resident fleets only. Preserve
  explicit sunk/captured retirement and legacy quest lifetime semantics.
- Existing `WdmMerchantShip::StartTrafficBattle/ResolveTrafficBattle` already owns
  local paired battles, one rescue patrol, wear and hull tombstones. Adapt it;
  do not build a second battle scheduler.
- `tools/gameplay/fleet-sea.c` already preserves exact Ship/RealShip cargo/damage
  and imports persistent IDs. Extend its same manifest/task bridge for commerce
  and fort targets, rather than parallel sea-only generators.
- `PROGRAM/store/storeutilite.c` owns Set/Add/RemoveStoreGoods, current quantities,
  prices and daily stock updates. `Colonies[].StoreNum` supplies the actual store.
  Supply consequences must integrate there and reconcile daily regeneration.
- `PROGRAM/sea_ai/AIFort.c` uses Fort_FindCharacter, live colony nation, actual
  cannon quantity, Fort.HP/Ship.HP and fort resurrection. Integrate map siege wear
  after initialization and before fighting; initialization currently resets HP,
  so an attribute-only map patch would be visually false.
- The existing city siege branch in `PROGRAM/scripts/colony.c` creates six hunter
  captains, sets AlwaysEnemy/player hostility, locks a scripted timer and targets
  PLAYER_GROUP. Reuse underlying group/address/fort facilities, not this authored
  player-attack wrapper. It is not an NPC-versus-port simulation.

### Ordered delivery and falsifiers

The implementation uses the existing exact-hash composition and ordered native
patch stack, then canonical staging. The unresolved native fort-target binding
must be proven before accepting the siege bridge; the authored player-attack
wrapper is not evidence of that contract.

1. **Persistent voyages and timed service:** bind cooldowns, event decisions,
   compatible old-save seeding and calendar catch-up to the same owner.
   New/old ordinary descriptors complete a route, spend game time servicing,
   depart again and retain identity. Seed a
   damaged survivor and a dead hull; only the survivor can recover. Quest timers
   and pause remain unchanged. Port capture invalidates incompatible destinations.
2. **Cargo-backed commerce/piracy:** loaded stock equals transported manifest;
   loss/capture/sea exit/delivery conserve goods, with no repeated debit, duplicate
   credit, cargo reroll or loot creation. Saved-state reload is idempotent. Existing
   player store actions and quest cargo are unaffected.
3. **NPC expedition and fort handoff:** a strong hostile supplied fleet can reach
   a defended port, blockade/fight and suffer losses; a weak raider avoids it.
   Import the same damaged actors and fort state to sea; a neutral player is not
   attacked simply for loading. Nearby arriving relief changes the actual battle;
   distant fleets and invalid/quest ports do not. Continue to a land assault only
   with surviving landing strength, carrying the same siege ID, casualties and
   allegiance into an admitted authored capture stage. Verify failed landing,
   stranded-force surrender, eligible side entry, betrayal and interrupted
   extraction before claiming city-assault acceptance.
4. **Consequences and player-readable activity:** a disrupted port loses a bounded
   amount of real supply, successful shipping/recovery restores availability, and
   the normal store reflects it. Far rumours remain dated/imprecise. Native probes
   and script VM establish implementation properties; canonical stage-only proves
   delivery; the player voyage accepts pacing, visibility and enjoyment.

No later milestone may substitute decorative events, instant arbitrary stock
changes, invisible escorts or forced player enemies for these prerequisites.

## Contract and owners

The map should contain trade travelling between ports, naval patrols and pirate
raiders with local targets. Battles should grow out of those encounters instead
of appearing already engaged around the player. Seeing farther must not give
hostile captains a larger detection radius or make every ship chase the hero.

`tools/metal_living_caribbean.py` owns the exact-hash map-script composition;
`experiments/native-metal/build.sh` owns the ordered engine patch stack. New
traffic uses the existing merchant navigation, encounter descriptions, fleet
generator and paired-fleet sea import. Quest encounters retain their existing
constructors, routing, timers and callbacks. Installed files are untouched while
the player is running the game; the canonical launcher serializes delivery.

## Candidate model

Traffic exists across the whole map. Native population scheduling creates
departures at real ports and keeps a bounded resident population; it does not
spawn traffic in a ring around the hero. Scripts remain the fleet-content and
diplomacy bridge, including existing story/quest encounters. Navigation,
target acquisition, escape, patrol limits and battle timing belong to the engine.

- Most traffic is commerce. A caravan represents an actual generated fleet,
  not several decorative copies of one ship. Routes use live port ownership
  and relations; patrols operate around their home port.
- Raiders prefer trade. Ordinary captains tolerate an enemy up to 1.10 times
  their side's power; 10% are bold (1.40), 2% reckless (1.65). Temperament is
  assigned once. Patrols use 1.10. Visible same-nation fleets already attacking
  the same opponent count as allies; distant traffic does not. After committing,
  retreat requires changed strength (20% own loss or 25% enemy growth) and an
  unfavourable comparison. Trading ships flee.
- Player strength uses every active companion-slot ship with actual hull HP,
  installed/intact artillery, loaded/spare ammunition and hull/sail/crew readiness.
  Parked ships and personal stats do not count. Ordinary raiders compare the player and NPC prey in the
  same scoring pass; superior player fleets cause escape and cannot force a sea
  encounter through the native contact flag. The player can still enter combat.
  Map entry rebuilds the estimate before creating the entity, including old saves;
  creation assigns awareness immediately and legacy creation refreshes before
  the next native update. No random risk reroll occurs during target evaluation.
- Old ordinary pirate and naval `Follow` encounters acquire the same roster,
  awareness and commitment checks. New ordinary Follow spawning is disabled:
  pursuit comes from resident traffic. Quest/qID/ALONE constructors and tasks
  retain their authored behaviour.
- Opposing fleets become a battle only after approaching each other. Both
  original descriptions and identities enter the sea scene together. Battles
  last at least 24 game hours using the native map clock (up to 48 for close
  forces), with pause freezing combat progress. One same-nation home patrol can
  rescue commerce already fighting pirates; another patrol cannot join that fight.
  All three identities import together even when the sea-entry radius cuts
  through the battle. Twice the enemy's combined strength wins deterministically;
  closer forces use squared-strength odds. Captured commerce expires; patrol
  and pirate survivors keep their stable slots, losses and ammunition wear.
  Overwhelming winners lose no hulls. Surviving losers escape with a 180-second
  cooldown; winners resume their route with a 90-second cooldown.
- Population caps distinguish trade, patrols, pirates and legacy pursuers.
  Increasing all random rates and the global cap is explicitly rejected.
- Saved routes, targets, roster slots and battle state survive map exit/re-entry.
  Old ordinary descriptors gain only missing fields; quest state is excluded.

Strength uses actual saved hulls and installed cannon damage, with hull/sail
condition, optimal crew, intact guns and loaded/spare ammunition. The same function
reads active player ships and imported NPC ships. Exact hulls are chosen once,
not rerolled with the player's rank. Sea exit saves Ship/RealShip values without
old runtime indexes, including cargo and captured/sunk tombstones; re-entry
allocates fresh runtime slots and restores those values. Aggregate map wear is
applied once and reset after exact sea state is saved. This remains fleet-level
autoresolution, not off-screen cannon-ball physics or an economy simulation.
Population runs while the world-map entity is active.

`tools/sync_metal_gameplay.py` composes the source-only `metal_fleet_sea` and
`metal_fleet_ui` adapters after existing exact-hash owners. They do not install
files independently. AIShip's existing fleet layer runs before the sea bridge.
The Enter menu retains contextual approach/attack/pursuit as its default. Both
quick-entry paths use 31130 to review a nearby encounter before entering; only
the explicit sea button in that panel commits through 31131. Plain entry clears
selection but preserves genuine hostile commitments. The encounter panel keeps
a square 160-unit illustration beside the top-aligned summary, a bounded body
scroller and one action row; its initial focus is the contextual action.
The encounter panel shows purpose, surviving composition and approximate
relative power, with truthful approach/pursuit/attack labels. Story restrictions
retain their existing callbacks; barrels and boats remain separate encounters.

## Acceptance

Primary falsifier: generated trade has a valid destination and real fleet;
nearby hostile patrol/pirate fleets acquire each other and form a paired battle,
while a distant player is ignored. Exit/re-entry preserves the pair and route;
sea import targets the opposing fleet, not the player by default. Nearest
negatives: friendly fleets and quest encounters are never retargeted, a removed
partner leaves no dangling pointer, and pause stops traffic timers.
Player-target falsifier: adding a healthy active warship prevents a weak pirate
from pursuing or forcing contact; removing/damaging it restores eligibility.
Comparable prey remains eligible, merchants compete with the player, and
quest encounters and voluntary player engagement remain available.

Compile and disposable script/native probes establish implementation properties.
The player owns the accepting scene replay: travel past a trade route, observe a
patrol intercept, enter that battle, and return to the map. No installed or
visible behavior is accepted until that replay is recorded in `docs/runtime.md`.

## Discovery

The prior layer raised pre-created battle frequency from 0.015 to 0.045 and
follow frequency from 0.025 to 0.035, with a shared 30-ship cap. Follow ships
steer only at the player; merchant ships have reusable destination/pathfinding
and saved coordinates. Existing sea import already includes both members of an
`attack` pair and assigns distinct `egroup__wdm_<id>` group identities.

## Candidate evidence

The canonical native build passes. Disposable probes using actual native
methods pass day-scale timing, pause, one-patrol rescue, saved-ID restoration,
missing partners, decisive superiority, guarded-trade avoidance and quest/friend
exclusion. Exact composed map/init/encgen/reload/sea scripts compile together in
the native VM. VM state probes pass rescue-range inclusion/cursor restoration,
distant/quest negatives and persistent wear with untagged/quest negatives.
The script lane separately checked real class tables/count generation and
maritime locators. Canonical staging and exact installed engine/content hashes
are verified. Renderer/world geometry and creation/scene replay are outside
those fixtures; player scene acceptance remains pending.

October 5 player-target correction: exact composed scripts compile in the native
VM; state probes cover full active fleet, parked-ship exclusion, hull/sail damage,
removed companion and no ship. Disposable probes execute actual native avoidance
and acquisition methods for ordinary/brave thresholds, escape, trade preference,
distant-player and quest/qID/ALONE/missing-state negatives. Read-only independent
review found the first-frame awareness gap; entry/creation refresh closes it.
The initial script compile failed because `makeref` cannot bind an attribute
branch; `makearef` corrected that owner and the final VM probe passes.
The VM additionally refreshes an old pirate Follow descriptor with real class
tables, verifies quest exclusion and checks that bravery is retained across ticks.
Fixture-only enum/nation initialization errors were corrected without product
changes. The broad repository hygiene check remains unresolved: its own
`git check-ignore --stdin` exceeded a bounded 20-second check and was stopped.

October 5 coherent candidate: canonical native compilation and disposable probes
pass committed versus uncommitted acquisition, same-side nearby reinforcement,
quest exclusion, direct-entry selection cleanup and off-screen survivor handling.
The exact final script composition compiles in the native VM. Seeded VM checks
pass old-save roster creation/no rank reroll, zero-survivor tombstones, quest
exclusion, pursuit import, active player fleet and sail readiness, committed entry,
loss-triggered retreat, cargo/damage save and repeat restoration without doubled
wear, proportional ammunition consumption, capture removal and semantic action
labels. Final composition is idempotent and rejects unknown installed revisions.
These are implementation checks, not proof of visual layout or battle enjoyment;
the player owns encounter-panel and gameplay acceptance after installation.

## Military operations — source candidate

`tools/gameplay/worldmap-military.c` composes into the existing encounter owner.
A colony owns `trafficSiege`; its saved fleet ID refers to the original descriptor
and survivor roster. No additional fleet registry, treasury or replacement army is
created. Preparation lasts 6–12 game hours, then the same fleet requests its actual
maritime target. Only native arrival starts the naval phase; time alone cannot
teleport it. The military mission prevents ordinary patrol/service logic from
replacing its route, while damaged returnees resume the ordinary service contract.

National authorization, global spacing, port protection and recovery are checked
before the single weekly 15–20% consideration. At most two operations exist, with
one per nation and one per island. Commitment consumes the saved cooldowns even
on failure. Two ready home patrols are required before one is reserved. Supply
planning validates all actual Store debits and real holds before publishing the
operation. Quest-owned fleet/port exclusions remain; complete story-siege coexistence
still requires player replay.

The strongest hull retains 75% of maximum crew for cover; other transports retain
25%, always at least MinCrew. Only the remainder becomes the shore pool, debited
once at landing. Fort shelling consumes actual balls/bombs and powder, damages
physical indexed guns, hulls and crew; a nearby real hostile patrol interrupts it
through the existing NPC battle owner. A genuine fortless town skips the fort
only after local naval defence is absent. Fort towns require strictly more than
half their physical guns destroyed before fort then city stages. Autonomous land
combat lasts 10–12 total game hours, with cumulative losses against the same pools.
These force, damage, duration and probability values are unmeasured tuning.

City victory is a finite raid, not a national ownership transfer. Surviving defence
is recorded as temporarily surrendered during loading and restored as the same
remaining garrison after withdrawal. Plunder removes actual shop stock into living
holds, preserving space for evacuation. Existing cargo freight handles its later
home unloading. Usable crew, hull/sails, maximum crew and cargo load bound rescue;
crew and working gun weight use the real cargo convention. Partial space rescues
only that many survivors. With no usable transport, all remaining shore troops
surrender, including after city victory. Ending is idempotent and clears landed
assignments; dead transports and their goods remain dead/lost.

Island sea entry imports the active expedition at its actual map-to-sea coordinates,
including beyond the normal player encounter radius. Complete-fleet admission still
respects the 32-ship limit and preserves deferred rosters. The sea adapter resolves
actual hostile ships and the real fort commander; cover/transports hold their harbour
position once the fort is suppressed. Admitted sea or foreground land actors own
losses, and admission/unload advance the operation watermark to avoid duplicate
background damage. Explicit blocks in the new fort scan are required: a disposable
VM replay of an unbraced for/if changed a following `target=-1` into `1`, selecting
an unrelated character. The braced scan preserves the absent-target case.

Disposable whole-PROGRAM VM checks pass paid shelling, defence interruption, peace,
strict half-damage rejection, count-only damage migration, fixed preparation and
arrival, island reservation, partial rescue, all-transport loss after victory,
finite city loading, scene admission/defer, harbour hold and mid-assault native
attribute serialization with no repeated debit/loss/evacuation. The final run has
zero script errors; permanent test delta is zero. Current native patch application
and the changed merchant translation unit syntax pass. The October 7 batch passed
canonical `experiments/native-metal/run.sh --stage-only`; both development and
installed-app composition report zero pending consumers. The callback
adapter in `tools/metal_military_integration.py` composes after existing gameplay
owners, validates exact input/output hashes, and reverses only its reviewed bytes
for subsequent owner validation. Unknown revisions retain the original rejection.

The source suite now includes finite foreground room/city projections, scoped
participation and finite settlement, real berth/ship-loss and survivor recovery,
and calendar store/fort recovery. Hooks bind existing location admission/load/
unload/death, relation, mayor dialogue, map entry, rumour selection, sea death,
physical embarkation and selected-ship confirmation owners. The existing land
exit command offers withdrawal only in the owned foreground scene and restores
its original note elsewhere. No new popup, fleet registry or replacement hull is
introduced. Foreground pools preserve the fixed city reserve and do not refill
cleared rooms; location transitions suppress ordinary generated guards and the
native hero/officer healing path, then restore their exact prior settings.

The focused integration VM executes the actual departure event, Blood/Intelligence
and same-island national-quest exclusions, active/cleared legacy siege lifecycle,
late complete battle attachment, reciprocal attack tasks, six-hull admission and
whole-bundle deferral on hull/map-slot shortage. Existing four serializer rounds
remain green with zero script errors. This headless VM uses a task-local identity
localization adapter; original localization source still compiles, but rendered
text and native localization/runtime scenes are not accepted by this evidence.

October 7 source closure binds the following contracts:

- New ordinary fleets start with an explicit empty manifest/zero crew in service.
  Stock/recruit debits and elapsed work gate departure; the overdue bypass is gone.
  Initial sea admission rejects any unfunded/uncompleted hull atomically. Existing
  observed legacy state and explicit low/zero HP, sails and crew remain intact.
- Cargo, service and expedition transport share physical goods + crew + working
  gun weight. Planned recruits/repaired guns reserve space before any stock debit.
  An explicitly disarmed ship has no gun weight/fire. Missing legacy crew uses
  the old default; it does not turn an explicit zero into recruits.
- Targets use one cumulative weighted draw after unchanged weekly eligibility.
  Authored distance, defence, value/stock, physical holds, food endurance and
  prior attack influence that draw; their numerical balance remains unmeasured.
- The real berth gets a saved choice before autonomous harbour damage. A safe
  self-dialogue defers during incompatible UI/fighting; return requires the actual
  quay and normal reload. Healthy inaccessible hulls provide no evacuation.
  Harbour control outlives land-result cleanup until its actual fleet/control ends.
- Actual hull HP loss, newly destroyed fort guns and counted land casualties feed
  one saved contribution ledger. NPC losses advance the watermark without player
  credit; repair/repeat/unrelated actors cannot earn more. A late patent is not
  retroactive, and later signing retains the uncontracted portion's 5% ceiling.
  Local civilian murder revokes protection through the existing crime consumer;
  terminal service arrival publishes existing dated return news once.
- Existing `BI_Boat` → `Sea_DeckBoatLoad` → `SetSailorDeck_Ships` owns contact.
  `worldmap-contact.c` admits the real surviving commander only at safe distance,
  with a usable authored deck, no combat/transition/diversion, and actual alliance
  or patent. No captain clone, task reset or flag-based credential is introduced.
  Hostile contact uses one contextual command: request negotiations, explicit
  acceptance/refusal, then a separate revalidated visit. The first action moves
  nobody and grants no allegiance, participation, payment or damage protection.
  Consent binds the operation, fleet, commander identity/slot/group, island and
  phase. Real alliance/patent, usable force and no crime/story/offensive-task
  veto are required; a displayed flag is insufficient. Actual airborne rounds
  are queried synchronously from native AIBalls records, failing closed without
  the native capability. Mere enemy proximity cannot stand in for that query.
  Only the exact captain/hero relation and ordinary deck actors' groups are
  protected during the visit. Original groups return before deck teardown; a
  visit-derived return marker suppresses Cabin's proximity damage once and is
  consumed before sea execution resumes. Sea entry/load/unload clears consent.
  Both existing BI_Boat selector/launch cases remain byte-identical.

The composer has 23 callback owners, including lazy MainHero/Capitans dialogue,
fort gun damage, hull/death/crime and the custody berth writer. Canonical source
comes from reviewed originals; mutable carrier changes do not replace that owner.
LF/CRLF differences are admitted only against exact reviewed representations.
Pinned parallel cannon aiming, speed, boarding dialogue and custody berth changes
remain in the runtime variant. Backups retain original raw bytes; a repeated plan
is empty, and an unknown callback revision is rejected. Unowned runtime files are
preserved instead of overwritten from a historical carrier.

Final whole-PROGRAM/lazy-dialogue VM and four serializer rounds pass with zero
script errors. Separate probes use the actual hull consumer and canonical cargo
load owner, test contact admission, old crew absence versus explicit zero, NPC/
repair/repeat contribution, late signing, murder scope and return news; restored
ledgers remain inert. Fixture-only assumptions were corrected: real bidirectional
embarkation, complete RealShip Class/OptCrew, crew capacity reservation and cannon
weights. A contact fixture erroneously assigned the `DistanceToShipTalk` macro;
removing that invalid assignment restored the real 300-unit contact criterion.
The parley transition probe also covers actual enemy relation, no first-action
transfer/HP/entitlement change, identity replacement, renewed fire, quest/attack/
false-flag/crime refusal, island absence/change, exact local protection, group
restoration and one-time return consumption. The compiled native query branch
counts seeded live projectile records without changing them; real entity-event
and rendered command/deck transfer acceptance still require player replay.
No permanent tests were added. Canonical native build passes; runtime scenes and
perceived balance are not accepted by this evidence.

The played `/Applications/Corsairs Iddictive Remaster.app` now contains this
exact package and its native projectile-query capability. Build readiness and
the deep/strict bundle signature pass; installed, built and VM message headers
are identical. Player SAVE/config preservation is verified by unchanged hashes.

Still unresolved: rendered hostile pre-transfer parley, actual scene placement/
sea-to-land paths, rendered harbour choice and commander contact, real guard/crime/
contribution/settlement and evacuation replay, and normal prior-save/player replay.
Recovery/pricing and survivor reassignment still need player interaction evidence.
Canonical stage/install waits for the running game to close; root owns delivery.
