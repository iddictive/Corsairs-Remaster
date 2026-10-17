# Ship fire and damage control

## Status

Spatial fire, finite crew suppression, keyed saved state and paired hull/cloth
damage are integrated. The current installed engine and delivery verdict are
owned by `runtime.md`; earlier component evidence below is historical, not a
second installed-state inventory. Existing models and their assets are unchanged.

The player's night-fire screenshot reopens visual acceptance for enlarged black
spots around rigging. The reduced-depth coordinate and bilateral reconstruction
repair below passes the actual Metal component scene and is installed in the
revision13 aiming batch; player replay remains pending. Whole-scene visuals, fleet performance
and battle balance still require the accepting Metal app. No game control or
replacement of the player's SAVE is authorized.

## Player outcome

A hit starts a local fire which can grow, scorch wood, harm exposed crew and
burn sails. A staffed ship can recover; sustained hits and neglected fires can
overwhelm it. Bombs create pressure without a free random whole-ship kill.
Damage remains visible after extinguishing and survives saving/loading.

## Owners and integration

- Tracked `engine/src/libs/ship/src/ship.cpp` and `ship_fire_model.hpp` own native local fire state,
  fixed-step updates, old-save admission and ship draw submission.
- `PROGRAM/sea_ai/AIShip.c` owns real hull/crew/sail loss and ordinary bomb
  balance. It replaces the independent legacy fire-damage timer.
- `PROGRAM/sea_ai/AICannon.c` consumes effective staffing; firefighting does
  not change the authoritative crew population.
- `PROGRAM/battle_interface/BattleInterface.c` owns the existing quick-menu
  firefighting priority command, without a new management screen.
- `ship_fire_bridge.hpp` owns the simulation/render boundary. Local points
  contain position, radius, intensity, char, heat and stable variation seed.
  `backend.mm` and `ship_fire_render.hpp`/`ship_fire_shaders.hpp` own depth-aware
  flame/smoke/embers, receiver-local char and lighting.
- `ship_fire_demo.mm` is an editable offscreen inspection scene which consumes
  the actual fire renderer/material field. `ship_fire_demo_assets.py` reads the
  installed ship hierarchy/textures through the existing GM loader and exports
  demo buffers; game assets stay unchanged. Demo sea/camera/lighting and cloth
  presentation do not prove installed gameplay or full renderer parity.
  Authored `sail1..4` labels follow polygon order; `SAIL::SetLabel` remaps
  label three/four into its different internal hard-point order. Confusing
  those orders twists otherwise intact demo cloth into apparent giant holes.
- One integrating task owns shared Git, ordered patches, CMake, builds, GPU
  submissions, staging and installed-app replay. Source lanes are released
  before that shared transaction; subsequent revisions require coordination.

### Repeated retained-support queries after the21:50 player profile

The source successor caches only hull membership in `SHIP::ShipFirePoints`.
The key includes geometry pointer/generation/collision revision, owner, root-local
matrix and each source's coordinates; load/mount clears the cache. Main,
reflection, surface and volume submissions reuse that answer until the actual
geometry changes. Subcritical material-health revisions do not invalidate it.
Legacy unimported cut filtering remains direct. Current pose/wave submersion,
source count, raster resolution and continuous visual time remain live/unchanged.

The supplied sample binds PID47873 to installeda9c423de, UUID42C03E02, and exposes
repeated `IsHullPositionRetained`/support queries below ShipFirePoints alongside
substantial main-thread SEA EnvMap rendering. It does not measure a post-fix FPS
gain or attribute all rendering cost to support queries. Exact profile/binary
binding is retained in the [player evidence receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-laser-2150/receipt.json).
Canonical delivery succeeds22:13EDT in installeddecf7a02; real-battle performance
remains unresolved.

## Mechanism and balance

Use real ship-local anchors/surfaces and existing sail identities. Heat,
remaining fuel and char are continuous state; spreading requires
sustained heat and proximity. Repeated impact at one site saturates its heat
and cannot restore consumed fuel. All effects read one simulation state.

Actual native rain and wind drive immediate cooling and transport. Moisture
does not persist after rain stops. Effective repair,
available crew and damage-control priority determine suppression. Rain helps
exposed surfaces; reduced crew loses capacity rather than spawning extra fire.
Emergency suppression trades cannon staffing for faster extinguishing and
returns workers when fire is out. Small isolated fires need a recovery window;
casualties and sail wear grow with exposure, not a per-fire full-population hit.

Ordinary bombs retain their ammunition identity and deliberate direct damage.
Their hull critical is 1.6–2.4x and crew critical is 2x; they cannot trigger the
old bomb-specific instant magazine catastrophe alongside the new fire.
Player-directed ignition admission uses one roll per hit: an ordinary bomb has
0.5%, other noncritical ammunition has no added roll, and a final true critical
has50%, for every ammunition type. Skills/perks and target defence determine
whether the underlying hit is critical; they do not scale this ignition roll.
A critical bomb uses that probability once, without another ordinary-bomb roll.
Direct sail bomb hits
use the same ordinary-bomb admission.

Fifty independent noncritical bomb hits yield about 22.17% probability of at
least one ignition and0.25 expected ignitions. For ten independently admitted
critical hits, the expected number of successful
ignition rolls is five, before same-site saturation and damage control. These are
probability examples, not guarantees of a surviving fire or battle measurements.
There is no ship-class or salvo-size override; small ships retain the same
per-hit ammunition/critical contract.
No new unlimited chain explosion or duplicate legacy DOT. Round shot,
grapeshot, knippels and airburst retain their existing independent contracts.
The October 13 `HeatedShot` source candidate adds 20% ignition admission only
to the 20% of round shots tagged hot at launch; a hot hull critical combines
with the existing 50% into one 60% roll. It uses this same simulation and
damage-control lifecycle. Its perk/projectile contract and pending installed
battle replay are owned by `naval-ammunition-and-damage.md`.
Preserve quest immunity, attribution, death, repair and interface/boarding guards.

Persist additive fire state in existing character attributes, preserving the
native binary SAVE layout. Old saves without it admit a cold/default state or
their active legacy fire once. Pause, load, boarding, death and removal cannot
duplicate updates or leave an emitter/surface scope alive.

## Damage integration

Measured sail capacity and the paired cumulative-hole adapter are documented in
[sail-capacity.md](sail-capacity.md). Activation includes the repository model
capacity patch, native admission/projection and hull geometry/chips; installing
only the HP adapter with the old provisional-hole engine is not supported.
Saved thermal records must be imported before newly supported cloth allocates
cells. Keyed schema2 keeps one Ship.FireState authority and preserves prior hull,
cloth, pending damage and missing-mast records. The simulation pool can exceed48;
the render ABI remains32-byte records with48 points, hull char stays on hull and
each sail reads its own char. Repeated-save component evidence does not accept
the installed game's complete OnLoad or battle.

Near-burst fire also needs the independent actual-hull witness described in
[airburst-ammunition.md](airburst-ammunition.md). Its paired script admission
uses the existing ordinary-bomb probability owner and one final roll; critical
50% stays authoritative. Airburst fuze radius and fragment distribution retain
their current contracts.

The player explicitly requires preserved FPS and bounded CPU load. Thermal
updates run once per0.25 game-seconds with preallocated scratch. Exact near-hull
geometry queries run only for already admitted close detonations; finite chips
keep the global64-piece draw cap. Accept complete CPU/GPU frame time in the same
battle before/after, including a dense broadside, several burning ships and
restored damage. An isolated renderer demo or CPU kernel timing cannot accept
that scene; the cold256-cut hull rebuild remains an unresolved component bound.

## Visual direction and Mac budget

Ignition starts near the hit; growth follows the ship surface; intense fire
combines warm irregular tongues, hot cores, smoke and embers. Extinguishing
decays flame before residual smoke/char. Ship motion, wind and camera orbit must
not reveal detached plumes, hard volume boxes, white rectangles or repeated
orange cylinders. Char stays on the receiver with its original texture detail.

Reuse the existing Metal device, frame lifetime, geometry/depth and lighting
owners. The [Effekseer Metal example](https://github.com/effekseer/Effekseer/blob/master/Examples/GpuParticles/main.mm)
requires a new SDK and effect-asset pipeline. The candidate instead adapts
[GPU Gems fire anchoring](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-6-fire-vulcan-demo)
and [volume integration](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-39-volume-rendering-techniques)
within the current renderer. A whole-scene fluid solver is outside this contract.
Rendering detail may vary by projected size; simulation cannot vary by visibility.

Ordinary material uniforms remain 1328 bytes. Scoped fire receivers use a lazy
material variant with 3008-byte uniforms. Volume rendering samples stored scene
depth in a separate color pass before HUD. Reflected hulls receive char; flame
and smoke volumes currently appear only in the main view.

The first candidate's shared volume budget targeted eight million samples but
retained an eight-step floor. A 48-overlapping-zone component fixture still
admitted 46.08 million estimated samples and the rendered scene showed static
stipple. That representation was rejected. Its replacement retains 32 ray steps
at adaptive backing resolution with FP16 emission and depth-aware composition.
The revised seven-anchor side fixture estimated 39.27 million full-resolution
samples reduced to 2.45 million at quarter backing resolution. Neither estimate
proves an acceptable fleet frame rate. First-use material compilation also needs
observation separately from steady rendering cost.

The first visual acceptance is one complete ship through ignition, spread,
extinguishing and residual damage from multiple camera angles. GPU components
and build checks support this replay; they cannot replace it. New deep hull
fractures are excluded because the user retains existing models. Existing
separate parts and sail-hole owners may be reused without new geometry assets.

## Acceptance and rejection

- A healthy staffed ship survives and suppresses an isolated bomb ignition.
- A dry understaffed target under repeated distributed hits develops more
  intense/spreading fire and gradual hull/crew/sail consequences.
- Same-site repeated hits cannot multiply fire damage without bound; rain,
  repair and emergency priority have observable recovery effects.
- Paused and interface/boarding states produce no hidden combat losses. SAVE
  reload preserves prior heat/fuel/char without doubling ticks or casualties.
- Unburned ship/materials, ordinary ammunition and existing repair remain usable.
- The installed app renders the complete fire lifecycle and remains responsive
  in a multi-ship battle. Measure added cost on matching scenes; no FPS claim
  from a shader fixture or a world-map screenshot.

Canonical staging refuses a running game. Save and close it before installing
the source candidate through `run.sh --stage-only`.

## Baseline findings

The prior `FirePlace` starts two fixed particle systems and decrements a timer.
`Ship_GetTotalFireTime` depends only on repair; `Ship_FireDamage` independently
removes hull HP. Installed ordinary bombs also admit random serious boom and
powder explosion. Existing `shatter`/`HULL` code detaches nodes, but its motion
path is a stub and does not prove complete destructibility. Existing sail holes
already affect cloth and propulsion and retain cumulative saved damage.

These are source observations. Actual old/new battle balance and cinematic
quality remain runtime measurements, not facts inferred from source presence.

## Current source evidence and remaining replay

Exact native patch application and SHIP/FirePlace/SAIL syntax checks passed.
The current assembled Metal sources compiled all 33 ordinary material, 33
scorched material, six fire/composite and four demo entrypoints. Backend and
demo arm64 compilation checks passed. Component checks covered the
32-byte point ABI, translated/rotated upper models, invalid-scope cleanup,
frame reset and input normalization. Model checks covered healthy suppression,
dry poor-crew spread, rain recovery, priority, saturation, pause and death.
The real script VM passed mechanics, quick-menu interaction and updated ignition
probability/guard cases without script errors. These checks generated no new
permanent test infrastructure.

The corrected prototype rendered side/rear/overhead and residual states, then
60 lifecycle frames at 12 fps. Root review confirmed intact cloth before fire,
warm wood/cloth response, cooler unburned surfaces, attached smoke/flames and
local residual char. The five-second movie accelerates the demonstration's
state timeline; it is not a real battle or a game damage-rate measurement.
Whole-demo command times for its reused-pipeline sequence were 0.678/4.651/10.212
ms min/median/max. The separate first warm submission took 15.384 ms. These are
single-ship diagnostics, not installed frame rate or multi-ship acceptance.

Native receiver lighting uses the same fire state and original albedo/normal;
it adds no new cube-shadow passes. The active receiver uniform is 3008 bytes;
ordinary uniforms remain 1328. Flame reflections and fire lighting on water
remain limitations. Existing native sail holes own perforation/SP/repair;
the prototype introduces no independent shader hole threshold.

The native SHIP and FirePlace SAVE/LOAD field order is unchanged. A separate
VM SAVE callback fixture initially failed with `Invalid function` because it
omitted the mandatory `OnLoad` callback documented by the existing runner.
The integration audit passed corrected two- and four-round native compiler
serializer fixtures. Normal old-active-fire migration and new-fire save/load,
burning cloth/hole/repair, installed bomb balance, complete visual lifecycle
and multi-ship GPU performance still require the consuming runtime.

## Progressive holes and chipped hull

The player subsequently requested small fragments at shell impacts and lasting
holes in the hull, plus explicit gradual cloth burn-through into holes with
real sail strength loss when suppression fails. Existing GM assets stay intact.

The current fire candidate already routes cloth exposure through native sail
damage and saved holes. Verify that growth and repair in the consuming runtime;
do not create a second visual hole rule disconnected from SP or SAVE state.

Hull work first resolves actual damage admission, hit position/normal, receiver
coordinates, existing debris geometry and persistence/repair owners. Small
bounded transient chips and persistent hull perforations must read that admitted
damage. A painted black spot does not satisfy an actual visible hole. Preserve
immunity/death attribution, ordinary ammunition damage, pause/cleanup and the
current native build batch. Flooding or another independent damage timer is not
part of this request.

Acceptance includes progressively burned cloth losing SP and forming holes,
suppression stopping growth, and repair/save/load preserving the correct state.
Shell impacts must show local chips and holes which remain registered while the
ship and camera move. Unhit nearby wood/cloth, depth/reflection consumers and
frame cost are the nearest negatives. The native geometry, damage-admission,
capacity and near-hull ignition changes are integrated into the next source
batch after the preceding transaction released its owners. The combined native
build and full prepared PROGRAM compile pass. That batch is installed through
canonical stage-only; the accepting player scenarios remain unresolved.

### Ocean seen beyond the opposite hull

The player explicitly distinguishes ocean beyond the vessel from a horizontal
water surface inside it. Installed FrigateQueen1 and WarShip1 GM receivers have
outward-facing exterior shells without an inner lining at the inspected rays.
The old CCW culling removes the opposite wall viewed through a genuine cut.

The source correction in `ship-hull-damage.patch` scopes two-sided drawing to
the complete damaged GEOM node, including its untouched material objects.
The geometry service applies NONE after each named technique pass publishes
state, then restores prior native culling; GEOM restores the prior service scope.
It adds no vertices, primitives or submissions, and preserves the real opening.
Repair and an undamaged instance retain their prior path. Current native syntax,
exact patch replay, actual-Frigate cut/far-wall and CPU draw-state checks pass.
The tested replacement topology remains byte-identical (16242 triangles,
26186 vertices, one draw); additional GPU rasterization and installed appearance
remain unmeasured. This correction does not exclude sea inside a hollow hull or
establish physically correct inward lighting; those are different mechanisms.
The integrated canonical native build passes; installed replay remains pending.

### Reopened weak-fire report and keyed state

At x8 the core scales Execute delta before the existing0.25game-second thermal
steps; this path does not bypass accelerated time. A controlled CPU replay uses
the same .76 ignition, dry wind and300game seconds on each geometry. With86%
staffing and repair .51, both Manowar and Battleship extinguish the single site.
At17.5% staffing and repair .14, Manowar spreads after46.75seconds and reaches
five active sites; Battleship spreads after57.5seconds and reaches ten. Raw hull
loss is2.36% and3.52%, and crew loss1.49 and2.68, before real damage defences.
This proves the suppression/transport mechanism for those inputs; it does not
accept battle balance. The player later identifies «Редутабль»; the admitted
cloth route below supersedes this hull-only oracle for the reported defect.

The persistence writer had a separate confirmed defect. Native
`ATTRIBUTES::CreateAttribute` appends, so every checkpoint created new hull/cloth
snapshots and duplicate mast nodes. Live SAIL lookups and direct SHIP remount
read the first stale branch. Full script SAVE decoding already merges duplicate
names through `SetAttribute`, so raw-file duplication alone does not prove loss
after the ordinary full load.

The source correction in `ship-damage-admission.patch` uses native
`VerifyAttributeClass` for updates. Initialization imports all historical keyed
records in insertion order, preserves the newest value per key and detached
records, then rebuilds the two owned trees once. Pending damage, clocks and the
existing schema/version remain intact. Real ATTRIBUTES/STRING_CODEC and the
actual native attribute codec replace the former upsert mock:736 prior-state
cases, the archived duplicated states,100 bounded checkpoints, repair/orphan
negatives and two codec rounds pass under ASan/UBSan. SHIP syntax and the
integrated canonical native build pass.
Live flames multiply native intensity; char-only state cannot sustain a flame,
and the macro disables independent legacy flame particles. Installed fire
damage, complete OnLoad and matching CPU/FPS remain unresolved.

### Redoutable cloth transport correction

The player identifies «Редутабль». Its archived Battleship1 has98/559 crew,
repair/defence .14, morale73, passive Doctor2 and BasicBattleState, without
active LightRepair or firefighting priority. The saved ship heading and weather
produce local wind(-2.258879,-3.835098). Fresh bomb cloth ignition has energy .58.
The actual35-cell production model extinguishes four low/high cloth starts in
28.75–29.5game seconds, without spread or crew loss. Crew fractions do accumulate;
the failure is heat transport, not truncation or x8 time skipping.

`ship_fire_model.hpp` corrects two equations: ambient cooling follows cell heat,
while crew/rain removal keeps its previous rate; conduction attenuation uses
the gap between existing cell supports within the same1.5m reach. All118 directed
neighbour links are unchanged. Existing radii and anisotropic plume distance
remain approximations; no new cells, loops, allocations, rates or state appear.
The variant scaling all cooling by heat is rejected because it weakens emergency
suppression; the accepted split preserves deliberate crew and rain removal.

With the same poor crew, four admitted cloth starts spread in25.5–35.75game
seconds. At300game seconds (37.5real at x8), hull loss ranges3.58–11.53% and
crew loss3.29–14.20 after the archived defences. Full crew suppresses the tested
single fires in4.75–8.75seconds; rain in1.25–2.75seconds. Priority suppresses
the poor-crew hull start without spread. Full-crew three-site ignition dies in
19.5seconds, with0.128% hull and0.574 crew loss over its60-second observation.
These figures are fixed-geometry CPU evidence, not installed battle promises.
SHIP moving wave/rocking cooling and active repair remain outside that replay.

Canonical native build passes for the exact model SHA-256
`c15cd9b1c4bfc0e9d30a01bbaed314fe47cf0d3cfbc338324af7adccba787819`.
The player subsequently asks for medium-ship70%-crew and six-site large-ship
balance; that quantitative scenario is a separate open acceptance within this
same mechanic. Activation, installed spread/crew losses and FPS remain unresolved.

### Finite crew suppression capacity

The medium-ship replay exposes a separate allocation error: each thermal cell
received the complete per-site crew suppression regardless of simultaneous
fires. Six fresh sites on Frigate1 with70% staffing therefore disappeared in
8.25–16.75game seconds. The player requires playable fires with meaningful
understaffing and severe multiple-site overload; six is an example, not a fixed
fatality threshold.

`ship_fire_model.hpp` now owns one finite ship-wide cooling budget. Existing
crew ratio, morale, Repair, rain and priority multipliers remain. Current crew
quantity supplies a base suppression slot plus one slot per150 crew units;
150 is a gameplay calibration, not a historical firefighting headcount. Burning
cells receive the budget first, with remaining capacity shared among warm cells.
One local demand evaluator owns the allocation and application equations.
Duty follows budget utilization within the existing20% normal/48% priority
limits. The fixed-step path adds one bounded cell pass and allocates no memory.

Canonical `Ship_FireContext` reads `GetCrewQuantity` once and derives both count
and ratio from that integer snapshot; fractional casualty debits remain in the
authoritative quantity. SHIP reads the new count immediately after the existing
synchronous event. Restored absent or stale count projections are overwritten
before every enabled step. No SAVE codec or fire schema changes. Full prepared
PROGRAM and the real context event pass the native VM, including missing/stale
count, quantity changes, zero/negative crew, reload, boarding, interface, dead,
immortal and unused-ship negatives. Two real VM codec rounds also pass in the
entity-free fixture with its ordinary game Main/OnLoad disabled; this does not
accept the real game's OnLoad. The first full-program codec attempt called the
uninitialized game's OnLoad and crashed; it is rejected as a scene-load oracle.

Dry, fixed-pose production-model replay uses Frigate1's complete13 wood/13 cloth
cells, HP4732,259/370 crew, Repair/Defence .5, morale70 and no Doctor or hull
defence perks. Fresh independent hull sites give:

| Initial fires | Energy | Normal extinction | Hull loss at300s | Crew lost at300s |
| --- | --- | --- | --- | --- |
| 1 | .60 / .76 | 7.5 / 13.5s | .02 / .06% | .02 / .07 |
| 3 | .60 / .76 | 8.75 / 16.75s | .05 / .14% | .10 / .31 |
| 6 | .60 / .76 | Still burning | 11.83 / 18.60% | 28.87 / 45.97 |

Priority recovers the medium six-site cases in7.25/19.75s at up to48% crew duty.
It is not a guarantee for an understaffed ship: Frigate1 with148/370 crew and
Defence1.0, otherwise the same context, still burns after300s. Normal six-site
.60/.76 loses18.28/18.67% hull and10.92/11.15 crew. Priority loses11.73/18.60%
hull and8.30/13.57 crew; the strong-fire case retains the existing higher crew
exposure while on duty.

Defence1.0 reduces fire casualties by75% through `Ship_ApplyCrewHitpoints`.
Doctor2/Doctor1 separately multiply crew damage by .8/.9. Hull protection uses
the strongest BasicBattleState/AdvancedBattleState/ShipDefenseProfessional
perk (.15/.25/.35); Defence skill does not directly quench fire or reduce hull
damage here. Repair, staffing and morale own suppression strength. Active repair
perks retain their existing context override.

The full saved Manowar1 union is20 wood plus19 cloth cells; reading only the
first duplicated cloth branch omitted three sails and was an incomplete oracle.
The complete39-cell replay confirms six .60/.76 sites recover in4.5/8s, or
1.75/3s at priority, with836/836 crew, Repair/Defence .9, morale85, Doctor2 and
ShipDefenseProfessional. This is a skilled full-crew reference, not a promise
that every large ship survives six developed fires.

Redoutable's98/559 crew can now lose a single .76 hull fire even at priority:
spread begins12.5s and it reaches34 active cells,12.25% hull loss and18.25 crew
loss at300s. The former unlimited-cell priority recovery is intentionally
superseded; the player did not require an18%-staffed ship to extinguish every
fire. The rejected cold-cell-first variant starved active suppression. Current
active-first conservation and normalized-model parity pass:22 baseline cases
plus8 complete-population cases, no hot-step allocations. Model SHA-256:
`e4baa80130d82cf5e1b6fd0bfb1b07dbad1394ec658f4e50f1b0edec3703617f`.

CPU numbers omit moving waves/rocking seawater cooling, new direct shell damage,
active repair and later battle actions.300game seconds equals37.5real seconds
at x8. Evidence is retained under the canonical ignored diagnostics owner
`experiments/native-metal/.cache/diagnostics/fire-state-repair/crew-capacity`.
Installed battle balance, visible spread and FPS require player replay; game
input and launch remain explicitly stopped.

Canonical `run.sh --stage-only` subsequently returns0 for this exact model and
new live-context writer. The installed PROGRAM matches its prepared projection,
and its actual context/codec fixture returns0 with script_errors0.277 frozen
inputs and488 SAVE/config hashes are unchanged. The signed installed engine and
the integrated batch's pending player scenarios are recorded in `runtime.md`.

## Reduced fire depth and rigging silhouettes

The reported black spots are reproduced by a one-pixel opaque diagonal rope in
front of the production flame renderer. The full-resolution reference preserves
the rope's own silhouette; quarter-resolution fire broadens it into dark cells.
Two source defects contribute: solid unprojection used reduced fragment
coordinates while the ray/depth read used full coordinates, and bilinear
composition spread occluded coarse samples into adjacent background pixels.

`ship_fire_shaders.hpp` reconstructs the solid with the same full pixel as the
ray. Its existing four composition taps use symmetric linear scene-depth
weights, adapting the maintained
[HDRP bilateral upsampler](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.high-definition/Runtime/Lighting/ScreenSpaceLighting/BilateralUpsample.hlsl)
with its fixed `1e-5` tolerance. The local AO upsampler already establishes the
same symmetric depth-guidance responsibility; fire retains its own premultiplied
color and first-contribution-depth semantics. Occluded taps add transparent
color but keep their weight in the denominator, so a matching opaque sample
cannot be replaced by amplified sky fire. Full-resolution and all-sky pixels
retain the previous composition path. Mapped full-depth coordinates are clamped
at allocated ceil-sized backing boundaries.

The one-sided depth-rejection attempt removed most rope loss but increased
opaque-edge bleed; it is rejected. The maintained HDRP nearest-depth transparent
upsampler removed that bleed but left hard staircase transitions and21 rope-loss
pixels at quarter resolution; it is also rejected. Symmetric reconstruction
preserves the smooth flame and panel/rope coverage in the same rendered scene.
At quarter backing, enlarged rope-loss pixels fall31→1, an isolated pixel0.98px
outside the rope. At the minimum eighth backing the count is0. Opaque rope and
flat-panel edge leaks are0 in both fixtures. Panel RGB error against the full
reference falls0.005018→0.002400 at quarter backing; eighth backing is0.004655.
Clear and full-resolution controls are byte-identical. Ordinary reduced-resolution
interpolation error remains; these are component observations, not whole-game
quality or FPS acceptance.

The assembled canonical MSL byte-matches the rendered candidate, SHA-256
`46557dac4ac0a1f2acbfc68adf400a4d8d1cd3d21060437f50d2671b9defc595`.
Receiver scorch/irradiance and embers remain byte-identical. Camera/material ABI,
32 ray steps, fleet sample budget, texture/pass count and gameplay CPU path are
unchanged; four scene-depth reads are added to the existing composition taps.
Retained one-off inputs, rejected variants and accepted evidence live under the
ignored `experiments/native-metal/.cache/diagnostics/fire-raster-20261013` owner.
No permanent test infrastructure is added. Canonical stage and the player's same
night-fire scene/FPS replay are separate remaining delivery/acceptance steps.

## Developed-fire damage and continuous fire backing

The October14 player report reopens slow hull loss and pixelated flames while
retaining the flame design. The previous damage model limits even complete wood
exposure to about4.26% of maximum hull HP per game-minute before hull protection.
`ship_fire_model.hpp` adds structural loss from area-weighted developed wood:
`intensity * clamp((heat-.80)/.45,0,1)`, at an additional .0030 hull fraction
per game-second for complete exposure. Fresh .76 ignition retains its recovery
window. Damage remains one native output through the existing AIShip defences;
there is no second timer, per-site multiplication or change to thermal state,
crew casualties, sail wear, SAVE schema or suppression capacity.

In the same fixed-pose production replay, Frigate1 at70% staffing with six .76
starts loses15.23% hull at60game seconds and94.58% at300, versus3.41/18.60%.
Its single fire still dies at13.5seconds with .0582% hull loss; six-site priority
still dies at19.75seconds with .2825% loss. The rain case remains2.75seconds
and .0404%. A developed18%-staffed Manowar with35% hull protection loses12.68%
at60seconds and57.72% at300, versus2.44/11.09%. Twelve paired scenarios preserve
crew/duty/heat/fuel/char/cloth outputs exactly across14,400steps; cold, pause
and death still produce no damage. These are component rates, not battle balance
or a guarantee about waves, further hits and active repairs.

The render budget previously stepped1→½→¼→⅛, wasting up to75% of its existing
sample target at each transition. `ship_fire_render.hpp` now derives backing
scale from `sqrt(target/requested)`, retaining the8million target,32ray steps
and⅛minimum. This target is an estimate, not a hard cap: the inherited minimum
can exceed it in an extreme fleet. Actual pixel work can increase over the old
underused budget; installed battle FPS remains an acceptance gap.
Color/front backing textures retain high-water capacity rounded to64pixels,
bounded by the scene target. Small camera/fire changes reuse both textures;
the compositor samples only logical backing dimensions, excluding padding.
Actual encode changes552×311→548×309→544×306 while both576×320objects stay
identical. The first whole-ship frame remains byte-identical before/after this
capacity change. Independent source/pixel review accepts that component boundary.

The existing depth-guided four-tap compositor also approached nearest-neighbour
selection on sloping sails because its fixed world-depth tolerance was1e-5.
`ship_fire_shaders.hpp` retains the same symmetric depth weights and occluded-tap
denominator but gives them0.5% relative linear-depth tolerance. The current
[HDRP bilateral implementation](https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.high-definition/Runtime/Lighting/ScreenSpaceLighting/BilateralUpsample.hlsl)
remains the reused pattern; a full temporal upscaler would need motion/history
inputs absent from this fire pass and is unnecessary for this bounded repair.

Actual Metal component pixels reduce sloping-panel RGB error .004135→.003423
at¼ backing and .008038→.007639 at⅛. Opaque rope and flat-panel leaks remain0;
the prior quarter-resolution single rope-loss pixel is unchanged. Full-resolution
output remains byte-identical. The complete FrigateQueen1 inspection scene spends
2.69million→8million predicted samples at .25→.431 backing. Whole-frame review
retains attached flames, smoke, original cloth/hull and warm material response,
with smoother tongues and rigging; it does not accept the player's game scene.
Retained source snapshots, GPU outputs and the hull-rate replay belong to
`experiments/native-metal/.cache/diagnostics/fire-quality-20261014`.
No permanent test files are added. Canonical build, installed delivery and the
player's burning-ship/FPS replay remain separate evidence levels.

## Visible deck-crew fire response

The old Sailors/ShipMan movement has no fire-state consumer; sailors continue
their ordinary walk/cannon routes through burning deck cells. Direct engine
`ship/include/ship_base.h` and `ship/src/ship.h` expose the existing live thermal
model through a nullable const getter. `sailors/src/sailors.cpp` reads that model
and the existing FireState active/enabled/duty summary. It writes no fire,
damage or SAVE state. Canonical CMake gives sailors the same native helper
include path already used by ship/rigging.

One per-ship planner masks hazardous authored points and complete link segments,
allowing an engulfed sailor only a monotonically outward escape. It reuses the
waypoint graph and existing run/sit_down/stand_up animations. Visible suppression
staffing follows actual finite duty; workers run to a safe edge and crouch facing
the fire, while others seek safer deck points. A trapped sailor stops when no
safe authored route exists. Installed Lo_Man has no bucket animation or prop:
this represents suppression work and evacuation, not bucket throwing.
Pause freezes responses; extinction restores ordinary behavior; death, jumps,
swimming and detached actors retain their existing ownership.

Hazards/graph distances are shared per ship at the thermal revision or bounded
.25game-second cadence. Frame movement adds no cell/graph scans. An initial
planner reset each route at that cadence, leaving an actual-body worker at x=-8
for12ticks on a5m safe edge. That candidate is rejected. The successor retains
validated path progress, replans toward the forward endpoint and invalidates
on changed hazard/target/duty. Interleaved native bodies reach x=-3 and working
state at tick7; fleeing reaches the safe wait state. Pause, cold restoration,
newly blocked routes, role changes, death/detachment and escape negatives pass
under ASan/UBSan, with full Sailors translation and SHIP header syntax checks.

The installed Frigate1 graph fixture has43points/46links and50sailors/39fires.
Planning is23.88µs for dense deck fire and90.27µs for the all-reachable overhead
control in that component; neither measures complete installed frame time.
Replayable rejected/final body evidence lives under
`experiments/native-metal/.cache/diagnostics/fire-crew-20261014`.
The player's full animation/fire lifecycle and dense-battle FPS remain unresolved.
