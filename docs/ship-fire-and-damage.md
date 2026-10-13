# Ship fire and damage control

## Status

October 13 fire source candidate passed current compiler/component checks and
the corrected offscreen prototype replay. The first representation was rejected
for repeated cones/stipple, and its demo cloth was corrected for twisted corners.
The canonical native build/stage consumes those inputs and the installed receipt
matches them. Later requested hull chips/holes and measured sail capacity belong
to the next delivery batch. The installed Metal app remains the accepting runtime;
no installed visual, fleet performance or battle-balance acceptance is claimed yet.
Existing models and their assets remain unchanged.

## Player outcome

A hit starts a local fire which can grow, scorch wood, harm exposed crew and
burn sails. A staffed ship can recover; sustained hits and neglected fires can
overwhelm it. Bombs create pressure without a free random whole-ship kill.
Damage remains visible after extinguishing and survives saving/loading.

## Owners and integration

- `ship-fire.patch` and `ship_fire_model.hpp` own native local fire state,
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
Preserve quest immunity, attribution, death, repair and interface/boarding guards.

Persist additive fire state in existing character attributes, preserving the
native binary SAVE layout. Old saves without it admit a cold/default state or
their active legacy fire once. Pause, load, boarding, death and removal cannot
duplicate updates or leave an emitter/surface scope alive.

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

## Next batch: progressive holes and chipped hull

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
frame cost are the nearest negatives. Implementation remains pending source
ownership release after the current build/staging transaction.
