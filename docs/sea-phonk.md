# Phonk Mode

## October15 — integrated fracture lifecycle

Source `9288f64` passes canonical native build/staging with signed engine
`498cc8dc8236fe1e622be129fa439a20eceb450da71bc75d86df0346700077db`.
Fresh protected player hashes remain unchanged; player acceptance is unresolved.
The integration owner records complete delivery evidence in `runtime.md`.
The engine source owner is now `experiments/native-metal/engine`; historical
patch references below describe prior receipts, not the current edit path.
The player rejected the preceding4c14ef fracture behavior: half-hulls remain alive,
crew walk over removed decks and loose rigging remains above absent structure.
The successor binds geometry, retained support and structural death for both
laser and mechanical cuts; source evidence below is not player acceptance.

The current request keeps ordinary Phonk at500 knots and adds held left/right
Shift, using the existing `ILeftShift`/`IRightShift` controls, to accelerate
continuously toward1000 without a timer or consumable. Brake wins; release sheds
excess speed continuously. Supported thrust raises the bow through the existing
damped attitude response; airborne motion and water-entry compression keep their
existing owners. Ordinary/NPC movement and the binary save layout remain.

`PROGRAM/scripts/sea_phonk.c` owns2% maximum hull regeneration per game second
and the post-perk contact-episode budget: at most5% maximum hull or25% entry
hull, whichever is smaller, until0.75seconds without damaging contact. Pause,
reload, death and mode disposal retain their existing guards. The three actual
ship/island/grounding producers share the final budget in `sea_ai/AIShip.c`.
This budget protects an intact ship from contact damage. Critical loss of hull
support is fatal even for the player's Phonk ship; regeneration cannot keep a
structurally broken half-hull sailing.

The existing mapped `Ship_Fire` input owns the Phonk laser; native camera fire
does not spend cannon ammunition or alter its reload during the mode. The aimed
existing broadside supplies one intact cannon near its middle, emitting a
single beam with muzzle-to-target occlusion. One battery drains16% per game second during
fire and restores10% per second on release; zero charge stops the beam. Existing
navigator charge arcs show it only in Phonk, retaining the ordinary charge and
damage paths outside the mode. Holding
fire visibly advances the beam and burns one target for0.65game seconds before
the final material bridge parts; the next cut has0.20seconds recovery. Moving
the aim over the same target supplies the cut plane, allowing vertical bisection
or a horizontal upper-hull trim. A stationary aim defaults to a vertical plane.
Camera picking and the selected muzzle's ray respect intervening ships/islands. The
shared bound includes transformed geometry centres and every model child.

`ship_phonk_laser.cpp/.hpp` own input, transient beam/heat, separated-piece motion,
native wave sampling and persistent plane/flood attributes. Geometry owns actual
half-space splits, original materials/UVs, closed-contour caps, persistent0.08m
char bands and retained-mesh collision. It reuses the hull BVH; fragments have
immutable buffers and are released after6seconds. Actual intersection edges
glow orange and cool over5seconds. The source limit is32 meaningful cuts per
ship, four retained fragment snapshots per node and64 active piece records.

Damage follows the actually removed geometry. Closed meshes use validated
volume; the authored open Flyingdutchman mesh falls back to surface area, which
must not be described as measured mass. Dry upper cuts cause no hull HP loss.
Small wet cuts feed progressive flooding. Losing35% of the original waterplane,
or35% of the draft at three of five original keel stations, marks critical
structural failure through the existing death/quest-guard path. Dry top trims
retain waterplane/keel support and are not fatal. Up to16 actual seam points in
height bands per cut query current `WaveXZ` at4Hz; lengths are grouped within
those bands, conservatively sampling their lowest actual opening. Mixing upper
and lower seams into one averaged point was rejected: it could move an entire
wet opening above water. Submerged openings admit water with a
simplified head/area flow; capacity derives from the original waterline contour
and draft. Flooding continues with Phonk OFF and restores after load. A separate
global sea event applies its progressive HP ceiling, with ordinary immortality
and quest guards. This is a simplified ingress model, not a fluid simulation.

`Ship.PhonkLaserCuts` stores model-bound local planes; `Ship.PhonkFlood` stores
ingress and attribution. Normal startup/load reconstructs cuts, chars, breach
samples and the contact contour without replaying damage or spawning old pieces.
TOUCH invalidates its existing cached contours through one default-zero hull
revision; ordinary ships retain their prior path. Old saves without these
attributes enter unchanged. Native pause advances neither burn nor ingress.
Renderer resets rebind saved planes to each new geometry generation without
duplicating damage, ingress or fragments. Source-node techniques and lighting
are scoped around independent fragment draws.

Every successful cut publishes `SeaPhonk_HullCut` (`alfll`) after its geometry,
owner attributes and attack consequences. The sea-lifetime handler owns both
laser partial damage and structural failure; mechanical partial cuts never pay
HP twice. A structural hit uses the existing death/quest path independently of
Phonk and combat multipliers. Restored critical cuts request fatal-only
reconciliation on usable frames until acknowledged; pause/reload cannot lose
the request, and partial HP damage is not replayed.

`SHIP_BASE::IsHullPositionRetained` owns model-local support as the intersection
of the saved cut half-spaces, defaulting to true for intact/legacy ships.
Sailor graph points, routes, work targets and offsets consume it; removed support
enters the existing fall/swim lifecycle. Mast/yard subtrees stay intact under
their native attachment owner rather than being cut beneath unchanged cloth.
The lowest unsupported/intersected authored mast falls through `MastFall` even
on restoration. Pending mounting precedes either Execute or Realize exactly
once, and deleted mast references are cleared. Individual hull-root rope/shroud
anchors also consume support, cached by revision and model/root generation.

Mechanical impacts reuse the same geometry, plane history, fragments, contact
revision and flooding owner through `SHIP_BASE::ApplyHullImpact`. Their explicit
non-charred plane style emits no beam, glow or burn. TOUCH owns collision work
and ordinary HP damage; that hook never charges HP a second time. Small material
work doses accumulate at the same local contact/normal/other ship, expiring after
one game second without more work. A large dose cuts immediately; up to55% of
the hull's support along the impact normal can separate, at the actual hit point.
Catastrophic broadside work near midship cuts across the keel at the contact's
longitudinal position, separating fore/aft pieces; end contacts retain local cuts.
Phonk protects contact HP, not physical geometry. The separate contact-motion
adapter and slow mass-dependent yielding policy belong to the collision solver.
Active-player ingress during the real contact episode shares its final HP budget
without extending that episode; later flooding remains an ordinary sinking hazard.
Environmental contact uses the existing owner index-1, with the same ingress path.

Source evidence:6000 baseline motion comparisons, held boost/release/brake/air
and complete launch/landing checks; native VM contact/regen/laser/flood/OFF/pause
and saved-attribute reload cases; actual Flyingdutchman cut geometry with512
BVH-vs-linear ray comparisons (max51 candidate triangles, mean6.88). These
include frame-independent energy, dry/wet/paused ingress and plain-versus-charred
cut checks. Mechanical work-dose/expiry/contact-change and catastrophic transverse
plane checks pass at16/50/100ms; the native VM also verifies shared impact/ingress
HP budgets, environmental owner admission and saved-attribute reload. Actual
Flyingdutchman transverse cuts at the centre and±15% hull length separate
28–43% of authored surface into fore/aft chunks, with valid seams and retained
collision; this open mesh result is surface proportion, not measured mass. Native
translation-unit syntax checks pass. These
checks do not prove appearance, game frame cost or actual player acceptance.

The reopened lifecycle passes source-derived ASan/UBSan checks combining the
actual native cut/event body with real mesh splitting: ownership and attack
ordering, waterplane/keel failure, dry/small-wet negatives, mast exclusion and
load/pause/ack retry. The actual script VM passes own-Phonk structural death,
OFF, duplicate settlement, partial damage, environmental ownership, quest and
immortality guards and saved-attribute reconciliation. Crew fall/swim and
route/offset negatives, both mast stage orders and attachment cache invalidation
pass component checks; none proves the repaired battle frame.

Required replay remains held Shift/ordinary return, tsunami launch/landing,
contact survival/regen, aim hold/sweep/cut/fragment/char lifecycle, dry top trim,
wave-driven flooding, bisection, pause/OFF/save/load and an ordinary cannon shot
with Phonk OFF.

The black-splash source repair is separate: `splash`/`splash2` already bake
ambient into vertex RGB and now explicitly disable a second lighting multiply.
`splash.fx` joins canonical technique delivery. Black tsunami `SEAFOAM_PS`
particles are not reproduced by this finding: their ordinary technique already
disables lighting and their installed texture contains no visible black texels.
The player clarified that the unresolved effect is surface wake/splashes while
sailing, visible at particular camera angles. That producer remains under
investigation; do not close it from the ordinary splash correction.
The latest tentative location is ahead of the bow. `SEAFOAM::RealizeShipFoam_Particles`
uses `frontEmitter[0/1]` (`seafoam`/`seafoam2`, `water1`) for ordinary bow spray;
`frontEmitter[2]` (`seafoam_front`, `Sparcle2`) also serves tsunami and Phonk
water entry. Both use unlit particles with culling disabled; installed `water1` has no black
visible texels in any of its five mip levels (minimum visible luma80.06/255).
The hull-side carcass starts at zero alpha at the nose. Missing normals and
local particle lighting do not explain the camera dependence. The next decisive
evidence is the same splash in correct/black camera views, or its texture label,
shader entry and fog/local-volume inputs; source arithmetic alone cannot bind
the reported black patch to one of these producers.

## Status

Installed October 13 through the canonical Metal stage-only path. The exact
native patch, controller/menu and Faster input passed component, full PROGRAM
compile and installed-byte/signature checks; installation preserved SAVE/settings.
The player's sea replay found insufficient speed, barely visible illumination
and braking that changes the sliding direction. The installed successor adds
adjustable throttle and tsunami flight. A subsequent screenshot confirms flight
activation but rejects its jump/landing motion and water foam under an airborne
hull. The current correction replaces the crest kick and instantaneous landing
with hull-support departure and damped water entry, and gates both water effects
through one native contact query. Component acceptance and delivery are recorded
in `runtime.md`. The player now rejects that revision too: frozen/weightless
flight, weak landing without a deep hull dip or impact splash, and a water trace
still visible beneath the airborne ship. Retained track/particle draws and the
water-projected glow were not covered by the earlier insertion/emission checks.
The successor passes complete movement/model-matrix, particle/render, sound and
lifecycle component checks. Canonical delivery and the exact installed engine
belong to `runtime.md`; player assessment of this successor remains unresolved.
Music disposal before death/scene replacement is already installed in `802fbde`.
The October14 music successor is local source only: the player reports silence
across combat-state changes. The reproduced fallback victory path stops the loop
before its routed replacement is suppressed; the repair and its player-replay
boundary are recorded in `runtime.md`.
No agent game input or launch was performed. The current engine and chronological
installation receipt belong to `runtime.md`.

## Contract and owners

At local sea, **Esc → Phonk Mode** sits directly below **Цунами**. The same
button turns it off and resumes play. Map, land and boarding retain their menus.
W accelerates, releasing W coasts, S brakes, and A/D turn the hull through the
existing remappable ship controls. Speed is controllable below a hard **500-knot**
effective horizontal ceiling, including tsunami/whirlpool transport. There is no
minimum cruising speed or automatic acceleration to the maximum. Toggle-on
preserves current velocity. The independent motor accelerates at 120 knots/second;
the brake removes 240 knots/second along the current world vector. The hull turns
while momentum keeps carrying it sideways; countersteering and recovering
traction straighten the course. Ordinary/NPC propulsion, steering and scripted
impulses retain their existing response. The reopened high-speed contact repair
bounds collision-induced turning for all ships. Mode exit caps its own residual
motion to ordinary propulsion.
Grounding, collision, death, stopped/fixed/surrendered ships retain their guards.

Climbing a tsunami increases acceleration toward the same ceiling. The existing
6×6 hull-averaged `ShipRocking` surface owns buoyancy height and inclination.
Departure occurs when a rising hull would require a negative support reaction
to follow that surface; the moving wave and ship motion determine its actual
vertical velocity. There is no fixed upward kick or minimum launch speed.
Phonk alone uses a cinematic vertical scale: gravity grows smoothly with the
cube of horizontal speed from native gravity to8× at500. Native support remains
authoritative on the ramp, and measured launch velocity is conserved. This
shortens the high-speed flight; it is an arcade choice, not ordinary gravity.
Flight retains horizontal inertia without motor/braking/traction/currents, while
continuous angular response follows momentum in the hull's forward/side axes.

Contact requires a closing boundary and uses hull draft, including the root
geometry's bottom. It retains incoming position/velocity. The finite compression
then follows the current hull-supported water surface using latched normal
impact velocity; its response is solved for a minimum up to92% of root-hull
freeboard. No position/velocity hard clamp is used. A strong landing therefore
submerges most of the hull before recovery; a low-energy crest skim settles
without a fake slam. The response returns to ordinary supported heave when it
decays. Controls resume on contact. A later wave serial can launch again; the
same serial cannot launch twice. Ordinary waves alone do not activate flight.
Flight and momentum reset on load/death/mode exit.
Existing shoreline grounding remains authoritative during flight. The inherited
TOUCH hull-admission solver can miss thin ships on extreme long frames; no new
collision bypass or all-frame no-tunnelling guarantee is introduced.

`SHIP_BASE::HasWaterContact` defaults to the existing wet behavior; the Phonk
ship overrides it from transient flight state. ShipTracks stops new wake inserts
AND retained-row draws in air while rows age normally. Landing begins a separate
strip, so rendered triangles cannot bridge the flight. SEAFOAM hides hull foam
and stops particle emission/draws in air, using the existing particle processor
to age retained particles. The water-projected halo is hidden in air; purple
ship-local lights remain. Wet contact restores normal emission rules.

One default-zero native `GetWaterImpact` snapshot supplies strength, generation
and the interpolated contact position to the existing seafoam_front particle
system and ship_bow sound. The pulse is emitted at that fixedXZ, reprojected onto
current WaveXZ; sound starts once per new impact. Existing particle lifetime
ends the extra impact draw. Only Phonk caps the stock speed-based bow offset to
hull length. No parallel script flag or SAVE layout is introduced. TOUCH restores
hull samples and all transient motion after speculative movement.

The navigator's numeric speed uses raw native knots for the live player in
Phonk mode. Ordinary arcade scaling previously displayed250 as100; the physical
ceiling was already250. Ordinary, dead and NPC readouts retain their existing
scale, as does the spyglass. The saturated dial angle does not clamp the number.
The player subsequently rejects250 as too slow; the source ceiling and matching
motor/brake rates are doubled. This changes the speed range while preserving
approximately4.2seconds of held W from rest to maximum. Canonical stage-only
delivery installs this speed packet; the compiler receipt binds its exact patch
and built header. Player assessment of the new speed range remains unresolved.

The lilac halo peaks in the exposed area around the hull rather than underneath
it. Four local light sources sit outside the port/starboard surfaces so outward
hull normals receive light. The existing scene source catalogue and additive
water-surface rendering own these effects; all changed render states are restored.

`PROGRAM/scripts/sea_phonk.c` owns transient `Sea.Phonk.Active` and one looped
music voice, using the existing music-volume bus. `PROGRAM/sound.c` routes
ordinary playlist requests through that owner while active and retains the latest
requested ordinary track for restoration. Sea teardown/initialization, boarding
and death dispose the mode. No separate setting, permanent perk or save layout
is introduced. Native motion/light are owned by `ship-phonk.patch`, appended
to the existing ordered native patch stack.

Combat entry/exit retains the same live Phonk voice. `AIShip.c`'s
`Ship_PlayVictory` asks that owner before stopping music; its ordinary victory
sound and timer remain unchanged when the mode is off. `SeaPhonk_Music` checks
the existing native `MSG_SOUND_IS_PLAYING` query, rather than trusting the track
name alone, so a previously stopped voice can restart on the next music request.
This does not add a per-frame playback poll or alter scene/death disposal.
The Great Recipe composer pins the complete normalized `AIShip.c` input in
`tools/patch_great_recipe.py`; its reviewed input fingerprint must include this
victory-path correction. The frozen pre-receipt delivery bootstrap is unchanged.

The strongest existing facilities are the native transient drift/collision path,
ship point-light rendering, and script music bus. Reuse them; no replacement
physics engine, renderer, playlist scheduler or UI shell is required.

## Reopened tight-circle movement and saved mode exit

The player reports ordinary ships following tight arcs after high-speed contact,
and speed remaining after Phonk exit. The native contact producer converts power
to yaw using a size-independent `.02` multiplier; repeated contacts accumulate.
Full consumed movement with Flyingdutchman mass/inertia reproduces excessive yaw.
No-input/no-contact high-speed motion stays straight, and initialized live mode
exit already restores ordinary velocity. A separate full Save/Load reproduction
shows that serializing500 while clearing transient mode state on load bypasses
that exit projection. These are distinct source mechanisms, not proof of which
one produced an individual player observation.

The source successor identifies contact origin through the default-compatible
`SHIP_BASE::AddCollisionStrength` adapter and transient per-slot tags in `SHIP`.
Only the contact yaw component is admitted within hull lever/linear-kick and turn
capacity budgets; its remainder follows the exact native yaw damping. Linear
recoil, script impulses, helm and perks remain separate. Slot reuse/delete,
prediction, death, load and live mode exit own transient cleanup. No new saved
`STRENGTH` field or binary layout is introduced. Blanket impulse clamping was
rejected because it erased a pure scripted rotation.

An active initialized Phonk save writes the same ordinary exit projection to a
fixed-size state snapshot, retaining live momentum. Airborne saves normalize
both existing position records to the water surface; ordinary/NPC serialized
bytes remain unchanged. Full serializer/loader, mixed and queued forces,
prediction and prior jump lifecycle checks pass. Already-written unmarked
high-speed/yaw saves cannot be classified as mode motion versus scripted motion;
they remain unchanged and are an explicit compatibility limitation. Persistent
forced-contact stress is synthetic; it does not accept actual pair detection or
remove the inherited long-frame thin-hull admission limitation. Exact canonical
delivery and actual player movement replay remain owned by `runtime.md`.

A bounded October13 upstream scan finds the same geometry-independent angular
producer in [develop](https://github.com/storm-devs/storm-engine/blob/develop/src/libs/touch/src/touch.cpp).
The [release list](https://github.com/storm-devs/storm-engine/releases) and indexed
open/closed issue/PR search supply no applicable collision-yaw correction for the
pinned native source. The local change reuses its hull geometry, contact queue
and damping instead of replacing the collision solver.

## Music input

The player selected any popular phonk song. This implementation uses
[RAIZHELL — Faster](https://ncs.io/Faster), an official
NCS drift-phonk release. `src/assets/audio/manifest.json` binds its direct
official download, exact bytes and music consumer. `tools/prepare_phonk_music.py`
reuses verified local bytes; canonical staging fetches the input when absent.
The downloaded audio is ignored, while the reproducible recipe is versioned.
The installed app retains the user's downloaded input; the source repository
does not redistribute the recording.

NCS creator-video terms are not a game redistribution licence. A public game
archive containing this recording requires the label's game licence; this
personal installation does not establish that licence. See the
[official usage policy](https://ncs.io/usage-policy). No paid licence is purchased.

## Acceptance

Compile the complete PROGRAM/menu chain against the native script VM, check
on/off plus land/boarding/dead rejection and restored music. Native drift must
show lateral movement after a turn and retain exact ordinary-off behavior.
Canonical `run.sh --stage-only` must consume the final patch and music input.
Installed menu, real ship movement and lilac appearance/music require an actual
sea replay; source/probe/build/staging results remain separate evidence.

The reopened jump contract requires the complete ramp/air/entry/recovery sequence
at the current500 ceiling: continuously changing hull attitude, preserved impact
inertia, a visible hull dip bounded by real hull geometry, a waterline-anchored
impact splash, and a continuous return to buoyancy. No wake, retained contact
particles or water-projected halo may draw beneath the detached player hull;
purple ship lights remain. Ordinary/NPC wet motion and effects, predictor
restoration, OFF/death/load cleanup and shoreline guards remain required.

Current attempt disposition and installed receipt belong to `runtime.md`.
