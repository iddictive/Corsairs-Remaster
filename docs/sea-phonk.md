# Phonk Mode

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
traction straighten the course. NPC ships and never-active off-mode physics
are unchanged. Mode exit caps its own residual motion to ordinary propulsion.
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

The strongest existing facilities are the native transient drift/collision path,
ship point-light rendering, and script music bus. Reuse them; no replacement
physics engine, renderer, playlist scheduler or UI shell is required.

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
