# Tsunami: local sea and world-map encounter

## Status

October 8, 2026: expanded material/ship/world-map source passes canonical staging
and is installed; the current receipt belongs to `docs/runtime.md`. The player confirms
the installed Esc button starts the wave. Its supplied screenshot rejects the
straight, repeating white crest. Physical F11 delivery remains unresolved.
New waveform, splashes, damage, capsize and natural encounters require player replay.
Horizontal transport, shore splashes, the sky-horizon correction and the wider
crest-material correction pass source/component checks and canonical staging.
The world-map debug menu action also passes canonical staging and is installed;
natural and debug creation share one event constructor.
The expanding arc/ring, rendered-land shelter, NPC map damage and navigation-led
local NPC maneuver pass canonical staging and are installed. Their actual game
replay, including open-water arc silhouette and moving hull responses, stays
player-owned.
The current installed inventory and chronological receipts belong to
`docs/runtime.md`; component probes do not prove real-game acceptance.
The later progressive-contact, geometry-driven pose and debug-feedback correction
passes canonical staging and is installed; its player replay remains unresolved.
The subsequent player report reopens storm-plus-tsunami hang acceptance. A native
collision-separation nontermination is reproduced. Its correction passes native
checks and the canonical build; installed delivery awaits the player's normal
quit, and the reported scenario remains player-owned replay.

## Player action

The pause menu uses **Esc → Цунами** in an ordinary local sea scene. At sea it
starts ahead of the controlled ship, including beside a pier. On the map,
**Esc → Цунами: дуга** or **Цунами: кольцо** creates an expanding raised wave
through the same constructor as natural encounters; its contact triggers the existing
automatic local-sea transition. Repeated start replaces the current wave; ordinary
storms and ships remain. These actions resume through the existing pause menu
exit. The buttons are absent on land and during boarding.
They occupy free cells to the right of the existing pause controls; Ring is map-only.

The existing F11 route remains available in source (Fn+F11 when macOS uses media keys). The script
debug menu contains `Цунами` in the free cell of its lower row.
`Цунами` closes the menu and dispatches the same map/local start owner. The local
wave travels back toward the position where it was called. The correction removes
the visible Stop controls from both menus, including handlers and navigation
references; internal cancellation remains the map replacement owner.

One existing `Log_Info` notification reports the exact sampled strength out of
100 and nominal hull damage before contact/defenses, for example strength 50/100
and base damage 30%. Local start and manual map arc/ring share the formatter;
the map report reads the newly created descriptor rather than sampling again.
Natural offscreen map generation emits no notification. A full PROGRAM/menu
compile and real script VM cases cover bounds, one sample, restart/replacement,
creation failure, ordinary storms/ships and land/boarding rejection. INI controls
and ordinary menu sections are checked against the preceding source. Actual
notification visibility and updated menus remain player-replay requirements.

The menu remains gated by the existing test-mode flag on land. The Win32 debug
console is unrelated; no global cheat/test-mode setting is enabled.

F11 previously reached only the land branch of `ProcessControls`. The installed
sea branch now dispatches `BOAL_Control` before the land-only test-mode gate.
The native SDL mapping is F11 → VK122; this ordinary control is unaffected by
`ondebugkeys=0` and remains outside the groups that freeze scene-specific controls.
[Apple's function-key documentation](https://support.apple.com/guide/mac-help/mchlp2596/mac)
also assigns F11 to Show Desktop. An Fn modifier alone does not prove that the
physical key reaches the game; no actual OS interception has been established.

## Continuous strength and ship consequences

`src/gameplay/PROGRAM/seadogs.c::SeaTsunami_SampleSeverity` samples `u = frnd()`
once and returns `s = u²`. Debug and natural starts share this continuous owner;
there are no fixed grade buckets. Native parameters use the same strength:
height `28 + 56s`, width `220 + 65s`, speed `22 + 6s` and nominal hull damage
`10 + 40s` percent of maximum HP before contact and defenses. The nominal 40–50%
band has probability `1 − √0.75 ≈ 13.4%` per wave, independent of encounter rate.

The original native contact paid a full crossing on the crest and a graze on
exit, explaining the reported instant hull loss. The correction integrates actual
profile height along the ship's movement between the ordinary one-second queries:
both leading and trailing slopes contribute while the hull remains in contact.
First discovery has no retroactive payment; same-age queries cannot double-bill.
Paid exposure is capped by the strongest encountered profile and resets on
cancel/restart. A stationary full passage approaches the nominal damage; a faster
crossing can take less, and following the wave cannot exceed the contact budget.
Actual-header ASan/UBSan checks cover 0.1/1/2-second partitions, moving ships and
entry/cancel/restart. The existing hull owner preserves immortality,
reload/dead guards and battle armor. `StormProfessional` («Плавание в штормах»)
multiplies damage by 0.7; `DontHitInStorm` prevents it. Natural tsunami starts
ordinary sea weather, avoiding generic hurricane/tornado damage on top.

Native SHIP adds damped pitch/roll after ordinary buoyancy, before model placement.
The original extra tilt was driven by height: even a flat crest could demand a
117-degree roll. The correction instead drives the response from the signed slope
of the same profile used by rendering and `WaveXZ`. Bow impact is bounded by
0.20 radians (about 11.5 degrees) beyond surface following; maximum additional
roll remains 2.05 radians, scaled by `s²`, actual flank and protection. Opposing
flanks reverse the forcing and a flat crest supplies no torque. During this
response buoyancy samples the tilted hull footprint, and rendering uses the
current post-Move position/angles rather than the preceding frame. Ordinary sea
retains its existing path. The storm perk reduces motion too. Script `Ship.Impulse.Rotate.z`
is an invalid substitute: `SHIP::Move` integrates yaw, while `ShipRocking` owns
the other axes. The native final pose is the authoritative response owner.
Exact native ShipRocking/response/CMatrix checks pass current waterline placement,
the tilted footprint and 400-step ordinary-sea equality. Actual-profile probes
still permit unprotected beam capsize at strength 0.9 and 1, while ordinary strengths,
bow/stern headings and protected response pass the nearest negatives. These are
component checks; they do not accept the installed hull appearance in the player save.

Fatal capsize requires live contact, heading **50–130°** to wave travel on either
side (90° ±40°), and actual model up-vector at or below zero. Strength must also
permit a 90° roll: `2.05s² ≥ π/2`, hence `s ≥ 0.87535256`, near **45.0%** nominal
damage. This eligible tail is about **6.4%** of samples; actual deaths also depend
on heading, exposure, defenses and prior pose. Bow/stern approaches are excluded.
Death uses normal `ShipDead` hooks. A synchronous `Ship.TsunamiCapsizeBeam` marker
chooses the continuing sink direction and is immediately removed. Native
`SetDead` transfers tilt into the existing death pose; no ship save fields added.

## Horizontal transport and shore collision

The follow-up uses actual native `SHIP::Move` and `TouchMove`. Incoming propagation
is opposite the stored outward direction: `frontDistance = launch − speed × age`.
The transient transport budget interpolates continuously from **60 to 300 metres**
with strength, multiplied by the existing storm protection. Actual local profile
and time determine velocity; maximum budget bounds the passage, without a
position teleport or query-side force. A stationary full-profile component probe
travels 60.000/179.987/293.884 m at strength0/.5/1; the strong professional case
travels209.339 m, immunity0. Moving or grazing vessels can travel less.

Velocity is added temporarily inside Move after ordinary inertia, then removed;
it does not survive in saved State. Collision prediction reads the same effective
old/new TOUCH velocity without spending/resetting the transient budget. Grounding
keeps its authoritative zero speed. Actual moved hull points reach the existing
`CheckDepthCollision`/`SHIP_SHIP2ISLAND_COLLISION` chain, whose normal script hull
damage remains the shore-hit owner. No separate tsunami coast damage is added.
Save body is unchanged; Load resets transient budget. Repeated prediction, actual
movement, cancel/restart/zero delta, grounding, fixed/dead/immunity and400-step
ordinary propulsion/impulse equality pass source-bound disposable checks.
Real shore impact and damage remain player-replay requirements.

## Finite collision separation

`TOUCH::FakeTouch` repeatedly checks predicted ship contours while moving actual
hulls apart. Its legacy 1024-iteration limit counted only island collisions.
Ship pairs could run forever when equal centres produced no displacement, or
vertical separation made the horizontal step smaller than float resolution.
The transient tsunami current makes even stationary hulls have predicted motion,
so an unchanged pair can continue satisfying the actual collision predicate.

The correction counts both branches and stops a ship-pair solve when neither XZ
position changes. It preserves the existing displacement, weight sharing, impulse,
damage and island solver; unresolved overlap can be retried on the next frame.
No saved state or event-specific collision bypass is introduced.

Disposable ASan/UBSan checks use the actual FakeTouch, contour/intersection methods,
CVECTOR, wave evaluator and drift budget, with explicit ship-prediction/island
adapters. Equal-centre and raised-hull fixtures exceed 6000 predictions with
unchanged positions before the correction, and return after one solve afterward.
Six converging ordinary/event pairs preserve positions and prediction counts;
the island path preserves its 1024-step limit and exact final state. The canonical
build compiles one changed source, whose bytes equal the tested candidate.
The player's terminated session has no saved hang stack. The restarted process
samples normal Execute/Realize/present activity; this does not establish the
original freeze's cause or accept the corrected player scene.

A bounded October 8 upstream check finds the same unbounded ship branch in
[develop](https://github.com/storm-devs/storm-engine/blob/develop/src/libs/touch/src/touch.cpp).
The [release list](https://github.com/storm-devs/storm-engine/releases) and
[open/closed collision-freeze search](https://github.com/storm-devs/storm-engine/issues?q=is%3Aissue+collision+freeze)
provide no matching fix; the pinned native source remains 4860fe13245b. This
local correction adds the missing termination contract without an engine upgrade.

## Shore splashes

The existing CoastFoam authored cross-shore strips identify candidate shoreline
segments. Depth broadphase and actual `ISLAND_BASE::Trace` at water level refine
them to land geometry. Splash admission requires the incoming crest, sea height
above the hit and an emerged island; a nearby wave or outward passage is insufficient.
There are at most 512 cached contacts and 8 geometry traces per frame. Synchronous
`MSG_SEA_TSUNAMI_SHORE_SPLASH` 50203 carries scalar hit/direction/strength values to
SEAFOAM, without a new island/ship library dependency or borrowed pointers.

SEAFOAM owns 12 separate native particle emitters using the existing
`seafoam_front` profile, `Sparcle2` texture and particles technique. Continuous
severity controls spray size/speed and emission duration. Cancel/restart, expiry,
island teardown/submersion and attribute Clear reset only the new pool. Ordinary
ship/storm emitters and coast mesh stay unchanged; particle scale defaults to 1.
Matched array allocation/deallocation in SEAFOAM_PS supports reliable pool teardown.

Disposable actual-method probes cover real trace hit coordinates with an independent
geometry shim, the bounds above, 21 severity samples and ordinary particle byte
equality at scale 1. No-hit/all-water, absent or submerged island, outward wave,
low water, zero delta, absent receiver and lifecycle negatives pass. Five native
translation units compile; these checks do not prove installed island rendering
or the appearance of spray. Those remain player-replay requirements.

## Natural world-map encounter

`seadogs.c::SeaTsunami_CreateMapEncounter` owns the shared native CreateStorm
command and strength assignment. Debug map start calls it after cancelling only
`Storm` descriptors with `tsunamiSeverity`; natural generation calls it after
the unchanged lottery. Cancellation uses existing `needDelete`/`deleteUpdate`,
without a new saved schema. Spawn remains 100–140 map units away with two-second
activation, allowing the event to appear before contact. Source-bound headless
VM cases cover replace/cancel, zero strength, ordinary-storm/ship preservation,
failed creation, land/boarding rejection, local sea, menu click/activate/resume
and visibility. The complete integrated PROGRAM compiles without VM errors;
visible map movement and the forced sea transition remain player-replay checks.

The existing saved `Storm` descriptor owns a `tsunamiSeverity` marker, including
valid strength zero. It is sampled once and reused on restore/sea transition.
`tools/metal_living_caribbean.py` composes generator/reload in their existing
owners. Importing a full generated script would skip traffic/fleet projectors.

Rate `0.000001` allows at most one marked event alongside the ordinary storm
limit. Existing encounter-off, pause and no-encounter gates remain. The cumulative
lottery makes spawn waiting roughly ten times the rate-0.0001 storm under matched
active-map conditions, not a 100-fold guarantee. Player frequency is unmeasured.
The installed front replaces the moving filled circle with a fixed-origin hollow
front: a **140° arc** or full ring. Radius starts at 12 map units, grows at
`6 + 3s` units/s and stops at `240 + 80s` (at most 320). Lifetime follows this
bounded reach plus four seconds for entry/fade. Full strength lasts through the
ordinary configured maximum spawn distance (currently 140); the shared gain then
decays smoothly to zero at maximum radius, with entry/exit and arc-end envelopes.
Contact below 0.06 exposure is rejected. Swept front checks admit a
crossing between updates, while the passed interior is safe. Ordinary storms
retain their six-cloud geometry.

Esc has separate map actions **Цунами: дуга** and **Цунами: кольцо**, sharing
`SeaTsunami_CreateMapEncounterWithShape`; natural generation chooses the shape
once. The local-sea start keeps its existing action. Existing
marked saves without new fields initialize a front without rerolling severity,
including valid zero. The existing Storm descriptor persists origin, heading,
shape, age and radius parameters. Water-origin admission uses a finite nearby
search without moving the ship or resampling the event.

`WdmStorm::BuildTsunamiMesh` produces both the raised water body and authored foam;
`WdmTsunamiWave` uses the existing compact fixed-function backend. The technique
is delivered through the canonical managed-material registry, whose first
admission binds the identical LF/CRLF predecessor bodies. No new shader pipeline
or map texture is introduced. The original blue debug disk/cloud/rain is absent
for marked events; ordinary storm visuals remain.

`WdmIslands` owns a one-time cache of active authored land triangles, clipped at
the sea plane and transformed with actual model placements. Exact source-to-point
queries govern shelter and contact; cached angular queries cut the rendered front
at the same landfall. Island reload/teardown invalidates the cache. The installed
archipelago is consolidated in `mein.gm`; most named island files are submerged
placeholders without usable BSP. Consequently `GEOM::Trace`/`Clip` and ordinary
map collision cannot prove shelter. Navigable-water patches are also insufficient
to identify rendered land. The small projected-triangle BVH adapts the existing
buffer intake, rather than adding a 3D ray-library dependency such as
[TinyBVH](https://github.com/jbikker/tinybvh).

The source-bound component intake finds 32 active models, 169,394 clipped land
triangles, 65,535 BVH nodes and 8 MiB capacity, taking about 26 ms once. 2,500 segment
queries match an independent edge/barycentric oracle without relocking buffers.
An actual water→Jamaica→water segment is sheltered; an open water ray passes.
These checks prove geometry/query behavior, not player-visible map acceptance.

The first four-frame offscreen fixture was rejected because its camera clipped
Jamaica and its unanimated substrate hid the water body. Corrected framing uses
the supported free-camera height 500 and actual WdmSea fixed-function/animated
substrate, plus installed island geometry/material inputs. A further two frames
consume the final shared gain: full initial strength and a sheltered decaying ring.
The raised dark face and pale foam are visible, the sector behind Jamaica is absent,
and the opposite water front survives. At maximum radius gain reaches zero and
the producer emits no mesh. The generic Metal shader compiles. This fixture has
no live reflection/sun refinement, labels, ships, spray or FPS evidence; the full
open-water arc silhouette and installed moving scene remain player-replay checks.

`TestInStorm → WorldMap_PlayerInStorm → wdmReloadToSea` forces local sea without
the skippable encounter dialog. Reload consumes the descriptor and copies actual
attenuated severity and radial incoming direction into Login. `SeaLogin` consumes
them before early exits; after successful sea creation `Sea_FirstInit` invokes
`SeaTsunami_StartWithDirection`. Map/sea coordinate axes preserve this direction;
the local owner normalizes and negates it into the existing outward-wave convention.
Missing direction in an old descriptor uses the ordinary bow fallback. Pending
commands clear on rejected login, teardown/load and after their single consumption.
`bSeaTsunamiEncounter` extends the existing map/land/SailTo lock only for a natural
wave while native `Sea.Tsunami.Active` is true. Finish clears it; debug does not
impose that lock. Sea teardown/load clears flags and pending commands. Attribute
clears during weather/cabin rebuilding republish actual native active state,
preventing a stale projection from unlocking a surviving event.

## NPC map damage and local navigation

Ordinary local SHIP instances already query tsunami contact for their own
character; player, enemy and companion damage share that owner. The map adapter
adds `WdmStorm::DispatchTrafficContacts` for live ordinary encounter ships only.
Tsunami uses the same swept front, arc attenuation and exact island shelter as
the player; ordinary storms use their existing cloud intersections. Paused,
inactive, expired, deleted, quest/qID/ALONE and locally admitted fleets are excluded.
The developing front does not spend a once receipt before age two seconds.

`tools/gameplay/worldmap-traffic.c::WdmTrafficStormContact` owns persisted roster
HP, cargo loss, survivor counts/power and last-hull deletion. Unknown hull state
is left unknown. Nominal tsunami damage is `(0.10 + 0.40s) × exposure`; the saved
Storm descriptor records one hit per fleet, surviving map reentry and save/load.
Ordinary storm damage uses actual active simulation seconds at the existing
local rate 0.011 maximum HP/s and bow/beam multiplier 0.25..1. The native dispatch
subtracts preactivation time, so different time partitions pay the same duration.

The intrinsic roster fraction and optional absolute Ship.HP snapshot stay aligned;
outer `trafficCondition` is applied once by the existing sea bridge. Paid service
adds only its original funded repair delta, preserving subsequent storm losses.
Source-bound VM cases show an intact 1000 HP hull becomes 700 while a 50 HP hull sinks;
four restores preserve losses/once receipts, and actual sea restore reads 200
intrinsic HP × condition 0.5 as 100 effective HP. Quest and unknown-state negatives,
ordinary elapsed-time partitions and funded-service completion pass.

The local AI move controller reads existing normalized `TmpSkill.Sailing`, derived
from summoned navigation skill, to anticipate incoming support by `5 + 55 × skill`
seconds. It faces the bow toward the source using ordinary rotate/speed controllers
and collision steering. Emergency steering temporarily skips task/move evaluation;
the task itself is untouched and resumes after passage/cancel. Main-player control,
fixed/grounded/unmounted/stopped/dead ships, locked or forced boarding/brander/drift
orders and surrender retain existing behavior. Missing/zero skill gives no maneuver.
No separate saved captain skill or world-map maneuver is invented.

Actual-controller CPU checks preserve 19 eligibility negatives and the task state.
With the same 0.04 rad/s yaw-inertia fixture, skill 1 meets the crest at 1.819° from
the bow, while skill 0.05 remains 55.623° from it; onset changes monotonically with
skill. This proves the bounded controller response, not success for every hull,
speed, collision or player scene. Local skill/pose and map fleet losses still
need the human's installed-game replay.

## Expanded geometry and material

`sea-tsunami.patch` owns one transient native frame/evaluator for scalar/SSE
height, slopes, foam, ship motion/contact and rendered vertices. Its compact C1
asymmetric crest has finite support and long noncommensurate lobes/meander.
Vertices sample the same global geomorphed point before render-offset subtraction.
They preserve the original 32-byte prefix and add float2 normalized height/foam,
giving a 40-byte stride. Private shader decoding supports both layouts; inactive
40-byte draws keep layout selection enabled while event envelope gates material.
The 256-constant ABI and water/depth/MRT contracts remain. Aim consumes displaced
scene/water depth, without another solver. Cancel restores ordinary material.

Material adapts [Crest foam](https://docs.crest.waveharmonic.com/Manual/Appearance/Foam.html)
and the existing local anti-tiling helper to installed PENA. A full Unity/Crest
dependency is incompatible. Existing SEAFOAM hull intersection/front emitters
own ship splashes. Only tsunami-owned particles clear on replace/cancel/expiry;
ordinary storm particle lifecycle remains unchanged.

Disposable checks cover 16,146 evaluator/contact cases, swept/grazing/once/reset
negatives and finite-difference slopes. Actual SEA/SHIP/SEAFOAM translation units
compile against canonical inputs. 243 full-profile CMatrix sweeps yield 30 real
overturns and 213 safe cases; bow/stern, medium, perk/immunity negatives pass.
Fifteen VM damage cases use actual hull damage with query/sea-presence/death
terminal seams. Descriptor/generator/login/lock and two save-codec rounds pass;
native disk/event bodies cover radius, activation, deletion and ordinary storms.
Creation, event containers and final sea start are explicit fixture seams.

First seven-frame offscreen Metal pilot proves shader compilation, inactive
32/40-byte identity, cancel parity and depth/MRT preservation. Visual admission
is rejected: neutral bump/uniform reflections and camera nearly intersecting the
fierce wave do not represent installed sea. The corrected seven-frame/27-pass
pilot uses all 64 installed sea heights and canonical four-mip normal conversion,
installed day-12 sky faces and a sky-only reflection capture. At matched crest
distance 520, camera clearance is 18 (installed minimum 1); low/high/pan frames
retain the sea substrate and show a distinct raised face with irregular foam,
without the initial grid or fixture plane clipping. Component material passes;
islands, live spray, vessel pose and perceived danger remain unproven.
The player's subsequent repetition report reopens material acceptance. Wider
oblique/native-frame endpoints reproduce recurring PENA motifs and unit-cell
contrast from the existing four-tap sampler. A continuous low-frequency UV
distortion uses two samples of the same authored texture before that sampler;
native density/height/foam ownership remains. Its source Fourier oracle reduces
unit-lattice contrast about49.5% at PENA mips3/4 with mean/variance within0.4%.
Six paired wide renders at different world positions/time preserve macro coverage
and depth/MRT. The far-right finite-plane cut is an unchanged fixture limitation;
repetition and temporal flicker in the actual game remain player-replay checks.
Player replay remains start/approach/cancel, low/strong visuals, splashes/damage
with defenses, beam capsize versus bow/stern survival, natural forced transition
and exit release, pause/restart/teardown and current-save behavior. Component
probes do not provide that replay; the player owns real-game verification.

## Historical debug-only ownership and evidence

- `src/gameplay/PROGRAM/seadogs.c` admits the existing F11 debug menu at sea and
  owns the one guarded start/cancel writer shared by both interfaces.
- `src/gameplay/PROGRAM/interface/debuger.c` adapts those actions to the debugger exit.
- `src/gameplay/PROGRAM/interface/game_menu.c` adapts the same actions to the
  ordinary pause menu's `ResumeClick` exit. Its canonical INI owns the two new
  button cells; all preexisting menu actions and their vertical navigation remain.
- `src/gameplay/RESOURCE/INI/interfaces/debuger.ini` owns their existing-style
  button geometry. `src/gameplay/manifest.json` binds both imported original
  files to their installed baseline hashes and the ordinary transactional delivery.
- `experiments/native-metal/sea-tsunami.patch` owns a transient SEA instance
  state and one shared physical height/slope evaluator. Scalar buoyancy/collision
  queries and rendered sea vertices consume the same profile.
- The aiming drape consumes current rendered scene/water depth. The displaced
  water draw supplies its surface, without another tsunami solver or water ABI.

The script writes `Sea.Tsunami.OriginX`, `OriginZ`, `DirectionX`, `DirectionZ`,
then `Start=1`. Origin is the current player ship position; the normalized
direction points outward along its bow. `Start=0` cancels. The native owner
consumes the command rather than persisting an active saved event. Sea teardown
discards the wave; pause freezes its simulation clock.

Initial parameters are a crest around 2,000 world units ahead, speed 22 units
per simulation second, height 28, and a wide solitary-wave front. The crest
passes the launch position after about 91 simulation seconds and the event
expires after about 123 seconds. Shorter sea draw distances bound the starting
distance. Appearance and exit fade smoothly; rotation/movement after launch
does not rotate or move the event origin.

The profile is a peak-normalized C1 quadratic B-spline: `1−4u²/3` for
`|u|≤0.5`, `(2/3)(1.5−|u|)²` for `0.5<|u|<1.5`, and zero outside.
Here `u=(along−frontDistance)/220`; support is ±330 and full width at half
maximum is about 279 world units. The front is uniform laterally.

The maintained [OpenFOAM solitary-wave implementation](https://cpp.openfoam.org/v13/solitary_8C_source.html)
was the strongest inspected ready reference. A full CFD dependency adds no
value for this debug event. The compact polynomial crest preserves continuous
height and slope, finite support and a shared native render/physics evaluator.

That first debug batch used only existing buoyancy/camera consumers, without
scripted damage or random encounters. Its uniform lateral profile and fixed
strength are superseded by the expanded contract above.

### Original debug acceptance probes

Compile the scripts and canonical native stack, stage through
`experiments/native-metal/run.sh --stage-only`, then replay the installed app:
open Esc, start, observe horizon approach, crest crossing and ship response,
cancel/restart, pause/resume and sea teardown. Compare ordinary sea after cancel
and verify that aiming follows rendered water. Source/math probes prove
only their properties; they do not accept appearance or gameplay consequences.

The complete PROGRAM include graph and separate pause/debug segments compile
with zero errors in the native VM. Its startup `Main` is renamed only in the
disposable compiler fixture; ordinary game startup is not executed.
Native scalar/SSE, C1 profile, bounds, lifecycle and same-value cancel probes
pass. A task-local no-window probe sets the isolated SDL F11 state and uses the
real SDLInput, PCS_CONTROLS transition and CoreImpl control-event producer. Both
deck and outside-view groups reach the exact script dispatch, menu loader and
debugger initialization. Exact button `click` and `activate` handlers reach the
current native SEA attribute-command branch: start activates the wave, emits the
notice and closes the menu; the command is consumed, the crest advances, repeated
start replaces it, and same-value zero still cancels it. Scene rendering, existing
debugger descriptions, interface exit and visible log consumers are fixture
doubles. No SDL video subsystem or window is initialized; this proves the command
chain, not its visible presentation or physical key delivery.

The nearest rejects pass with test mode off: land, boarding, dialogue and an
already open interface do not open the menu; a missing ship heading rejects the
start and emits its error notice. A separate controls-only native VM save/load
probe seeds an obsolete numeric control ID, then runs the canonical registry and
options restoration. F11 remains VK122, unlocked, and produces its real core
activation event after restoration. Both accepted probes report zero script
errors. A full scene save replay is outside these isolated checks.

Before the native upgrade, the ordinary source delivery gate reports
compiler pending; after canonical staging, it reports zero pending files and
compiler ready with the exact tsunami patch hash. Installed sources match all
five canonical PROGRAM/INI inputs byte-for-byte in their delivery encoding.
The compiler receipt binds
`sea-tsunami.patch` to
`a24fe3e7f5e03a3ce4e2f1adcd82e2a89c34e4078e02610f325a59431eb8f697`.
The earlier combined delivery's player-file preservation and responsive sea
startup do not accept a tsunami action. The exact pending player scenario is
the physical key, visible menu, start, approach, ship response and cancel on the
installed app. The installed pause entry also requires a real-game menu replay;
it does not establish the cause of the failed physical F11 delivery.

The reopened native probe compiles the complete current SEA implementation
unchanged, with a temporary friend seam and no renderer initialization. Actual
`AttributeChanged → ProcessStage(execute) → WaveXZ/SSE_WaveXZBlock` passes at the
nonzero global origin `(14567, -27890)`. After five game seconds the envelope is
one and the front is 1890 units away: a 2.5-unit baseline at the crest becomes
30.5. A real geomorphed rendered block gives 28.978294, matching scalar sampling
at its restored global coordinates after the render-offset shift. All four
vertices beyond the ordinary cutoff still receive the wave; zero delta,
`bStop`, inactive state and same-value cancellation pass. Missing execute-layer
registration, offset mismatch, cutoff skipping and shader Y flattening were
rejected as explanations. GPU visibility and live layer delivery remain untested.

The pause follow-up is compiled against the full current PROGRAM with the real
native script VM. Exact `ProcessCommandExecute`, shared start/cancel,
`ResumeClick` and pause `IDoExit` execute: click/activate publish origin/direction
and start/cancel, emit the start notice, clear `pchar.pause`, restore the resume
result and request interface unfreeze. Repeated start replaces the origin;
ordinary Resume does not alter the wave command. Non-activation, land, boarding,
absent sea and missing heading reject without an unintended exit or command
mutation. The compiler also loads the unchanged debugger action adapters.
Result: `PauseCases=0`, `script_errors=0`. Sea presence, camera/log and the final
interface-release terminal are fixture seams; this is script action evidence,
not a live menu, physical Esc or visible notification replay. One-off INI checks
verify window membership, activation commands and nonoverlapping button bounds.

The two new canonical imports preserve the installed baseline after newline
normalization. Their raw baseline admissions are
`PROGRAM/interface/game_menu.c` =
`15cd14f118117e6e87c549fcb82f2e7e2d3e488a5734c2e56bbaa08a5cf9f7b0` and
`RESOURCE/INI/interfaces/game_menu.ini` =
`d94c091745586bb06515ae41e9ba77c8136fb2893938faacf6967c08e724a16e`.
The integration owner registers these in the existing gameplay manifest and
delivers the source batch; no new native patch or engine build is required by
this content delta.
