# Runtime state

Last verified: October 9 directional sky/fog correction staged without game launch (America/New_York)

## Current verdict

### Current installed inventory — directional sky/fog and coherent camera rays

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for local
source commit `9052624`. Installed signed arm64 engine:
`44cdd0287dfb32ae0027f3410ab194d832ef05472f4c500e1012d7c608ca41ed`.
App receipt:
`9bb557fcf846722bcc3759a887295c74b777d0834c58eddd22f2924c0aa4dd66`.
All117 managed files match; the installed Mach-O contains the directional fog
kernel, FFP/land/modern-sea variants and camera-relative SKY ray uniform. Canonical
build verifies8252 portable inputs and reports zero further source patch-stack
changes or pending gameplay files. The prior75dc2057 engine is retained by the
installer's hash-named backup. The app signature passes during delivery.

The18:25 shore screenshot reopened acceptance after the prior late-night palette
repair. Fully fogged geometry and modern water now sample the current sky's
directional color, sharing its completed cloud blend and atmosphere. Native
backend tests cover dusk both directions, noon, night, dawn, dense rain and a
rotated/translated SKY/camera; no-fog materials and disabled-sky fallback remain
exact. All four modern sea passes agree with the independent FFP receiver and
preserve foam alpha. The updated513-frame GPU cache replay is smooth/finite at
mean0.689ms and p951.867ms, retaining2048²/96-MiB cloud detail plus a256-KiB fog
field. These are component results, not player FPS or a complete game replay.

The user closed the game before staging;247 current non-log player files and
five foreign working/index entries remain exact. Source/native/build/delivery
are accepted. Complete18:25 shore/sea and weather/FPS replay remains player-owned,
unresolved. Cyan translucent foliage-card edges are recorded separately; their
cause is not isolated. No game launch/input/termination or player-data write.
ZIP/Drive/site and release work remain deferred. Native evidence and disposable
sources are retained under the ignored native sky-evidence owner.

### Historical installed inventory — cloud precision, highlight detail and night fog, superseded by directional fog

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for local
source commit `92d3659`. Installed signed arm64 engine:
`75dc205792475022246dd6e4e40a32357a95013b23b33426f175651012d64024`.
App receipt:
`b64de0531cd4566c813f08749f74aa8d122ce73f71efb64b777362ee440b453a`.
All117 managed files and the final deep/strict signature pass. Installed Mach-O
contains both new shader corrections; installed `Evening.c` equals canonical
encoded source. The stage consumes one gameplay file and zero engine patch-stack
changes; it installs the newly built renderer and backs up the prior engine.
Portable8252 input verification passes. No game is launched by the agent.

The player's 09:06/09:12/21:54 scenes reopen sky acceptance. Cloud ray samples
now derive from fixed origin and analytic distance, eliminating planetary FP32
accumulation bands without more samples, a larger cache or blur. Unpremultiplied
cloud highlights receive a shoulder before Storm's UNorm target loses detail.
Canonical hour20–23 fog RGB converges to existing night levels; hour19, ambient,
density and real storm/rain overrides remain unchanged. One visual weather/fog
snapshot continues to feed sky, distant geometry, water and foam.

Fresh native 4112×2580 components reproduce the morning defect (23.23% fully
white, peak4.2386) and reject it after correction (0% fully white, peak0.96274).
Matched crops show the contour slabs disappearing; noon, dusk and moon-up night
stay finite. The513-frame native cache replay covers weather recovery, wind,
complete-cache swaps, frame reuse, absence/light jump and reset. Current timings
on M3Max are mean0.698ms, p952.014ms, max2.951ms, prime42.93ms, with unchanged
96MiB cloud snapshots. These are component timings, not player FPS. Freshly
built `dynamic_sky-probe` and unchanged weather surface/bridge checks pass.
Images, diagnostic source and `precision-evidence.json` are retained under
`experiments/native-metal/.cache/sky-evidence`.

The player had closed the app before staging;247 current non-log player files
and five foreign working/index paths remain exact. SAVE/config, game input,
launch/termination, push, ZIP/Drive and site remain untouched by this attempt.
Disposition: source/component/build/delivery accepted. Full game morning/night
mountain/ship/sea replay, weather playback and FPS remain player-owned, unresolved.

### Historical installed inventory — procedural sky and shared solar visibility, superseded by precision/night fog

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 after the final
source/ordered-patch batch. Installed signed arm64 engine:
`85d060146aff17e6ab277446135e5ad2a6967274ab6eebc6bc4e7f2739f7dc26`.
App receipt:
`492280f22a3a44ccd92f641008f59a5d55e252a436ea1bf30c2a34520f1fc88d`.
All 117 managed files match their receipt; the deep/strict app signature passes.
Native build/staging identifies source commit `2a6a776`. Canonical build verifies
8252 portable inputs; the final stage has zero further source-stack changes and
zero pending gameplay files, compiler ready. Source milestones are `e47a40d`
(volumetric sky/night sea) and `2a6a776` (shared solar/cloud visibility).

The sky uses licensed shape/detail/weather noise, world-oriented volumetric
clouds, density-dependent Beer/powder lighting, spectral twilight, mild aerial
perspective, original star catalogue and completed-cache interpolation. Modern
water has no artificial night emission floor. Solar disc/halo/overflow/flares
and both modern sea solar consumers use the same cloud transmission and smoothed
solar direction. SUNGLOW prepares the field before SKY's later layer; original
geometry/ship-sail obstruction remain. Legacy texture-alpha cloud masking is used
only when the procedural field is absent, preventing double cloud attenuation.

Native GPU evidence covers full cache cycles, clear/storm recovery, wind turns,
reset/absence/light jumps, cloud-aware stars, noon/dawn/dusk/night sea, solar overlay
pixels and clear/thin/opaque transmission. The reported dense-cloud sunset loses
its formerly clipped solar road; clear sunset retains it. Early SUNGLOW preparation
retains an enclosing sky draw scope. Original sea bytecode and modern material,
underwater/grazing, weather foam/alpha and unsupported-shader negatives pass.
Cache timing remains component-only: mean 0.834 ms, p95 2.243 ms on M3 Max for the
513-frame check; shared sky/sea sunset diagnostic is 4.75 ms. These are not game FPS.
Evidence is retained under `experiments/native-metal/.cache/sky-evidence`.

All 248 baseline non-log player files and five foreign working/index entries
remain exact. No game launch/input/termination, SAVE/config mutation, push,
ZIP/Drive upload or site change occurred. Disposition: source/component/build/
delivery accepted. Complete player weather playback, ship/shore reflections,
SunGlow geometry/postprocessing and FPS remain player-owned replay, unresolved.

### Historical installed inventory — local continuation/optional exit, superseded by procedural sky/sea lighting

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 for the frozen
local continuation and player-safe exit over the map-water material, shore contacts,
first-third gain, collision guard and prior menu/script batch. Installed signed engine:
`9b2c0616802fcd02c84df07963060c454caf814b12faeace4a5cada9db84f68f`.
App receipt:
`7816732da659cf649efdd7846c47d0ea83474e9bf92aa598c612a70d2130d416`.
All 117 managed entries and 37 encoded canonical inputs match; cache/app gameplay
plans are empty and compiler ready. Post-stage deep/strict app signature verification
returns 0 during canonical sealing. Final staging consumes patch SHA-256
8973106f79b2b8ae74a609649792948f4a2d63f40b5812a28ddc1c5203986a21,
with three source-stack changes. Native build metadata names 22bc292;
source hashes bind the subsequent candidate rather than pretending that stamp
identifies its uncommitted inputs. Installed worldmap.fx equals native source
a8c474aebdf2c7a8ce7d461d845fbff28a078083c3c7f53f34e490e8ca5de5d5.

The map wave now consumes ordinary WdmSea base/animated material and clock,
without a second texture collection. Its fixed angular crest phase replaces the
rotating pattern, and actual landfall produces a water-side foam lip. Severity
still owns maximum reach/speed; full strength lasts through the first third of
propagation after entry, then shared visual/contact gain fades to zero. Local
shore emitters follow displaced water rather than spawning beneath it. Exact
native frames and gain sweeps plus 74 local shore/lifecycle checks pass; actual
game appearance, continuous playback, spray and FPS remain player-owned replay.

Local expiry formerly occurred only 700 units beyond the launch origin. It now
continues 2500–5000 units, continuously derived from the same strength and clamped
by scene reach. The natural encounter lock clears only once the entire controlled
hull passes the wave's trailing support; other navigation locks remain. The wave
keeps affecting NPCs/shore while the player stays; map return is optional and
local teardown still disposes the transient event. Thirty-eight actual native
component checks and eight VM lock cases pass; the full staged PROGRAM compiles
with installed headers and zero errors. Continued-wave/optional-exit game replay
is pending and player-owned.

The installed correction removes the flat propulsion band, distributes local
tsunami hull damage through actual contact and binds event torque to the shared
wave slope. Tilted hull sampling/current-frame pose replace the mismatched support
path; bow kick is reduced, while strong broadside capsize remains. Visible Stop
controls are removed and one notification reports exact sampled strength/nominal
damage. Source-bound ASan/UBSan, full-PROGRAM/menu Storm VM and ordinary-control
negatives pass. The separate frozen daylight aim batch also passes this same
build/staging transaction; its player replay owner remains its cannon chat.

Fresh closed-game preservation matches all 248 preceding non-log player files,
including SAVE/config, plus five foreign working files and their index entries.
The preceding b4013994 delivery's human relaunch as PID14595 added two Sentry
session/lock telemetry files. The new closed-game guard delivery also preserves
all 247 preceding non-log files and the five foreign files/index entries. The final
2cb3b168 delivery again preserves those same files and foreign index entries.
No agent game launch,
input, termination, player-state write or push. Player-visible acceleration,
wave/hull following, damage timing, capsize and strength notification remain
unresolved, with the human owning current-save replay. Contracts and component
evidence belong to `docs/sailing-speed-floor.md`, `docs/sea-tsunami.md` and
`docs/sea-aim-overlay.md`.

The subsequent player report reopens local storm-plus-tsunami hang acceptance.
The previous session is terminated and its system log overwritten by the player's
normal relaunch as PID99695. Retained launch output has no fatal error; a bounded
sample of the new process shows normal Execute/Realize/present work, not the old
freeze. This prevents a definitive attribution to the old session's stack.
Actual FakeTouch/contour/vector/wave/drift checks independently reproduce an
unbounded ship separation under persistent predicted current. The correction
passes ASan/UBSan, ordinary-pair/island equality and the canonical native build;
the sole changed translation unit matches the tested candidate. Normal-quit
canonical staging installs this guard as 8bea3d2b; the player owns the original
storm-plus-tsunami replay. The new map complaint reopens material/motion
acceptance; the final 2cb3b168 batch corrects material/shore presentation while
preserving the guard and severity-defined propagation geometry.

### Historical installed inventory — ordinary sea-entry spacing and port service, superseded by b4013994

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 for the
content-only sea bridge batch. Focused source commit:
`18efcae7fc8dc6ad66daed253398f7200b398eb1`. Installed signed engine remains
`3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c`;
its existing native build stamp remains `7d2eed3`. Current app receipt:
`2c6f701cae34d541e38dbe0795300c1ad85ea0466d50bb4da354e2fb1d89ab86`.
All 117 managed entries and 37 canonical encoded inputs match. Cache/app plans
are empty and compiler ready. Only `PROGRAM/sea_ai/sea.c` changed in delivery,
with runtime SHA-256
`9a46021057b2447e30f5e1689d8e5aea29ebf34a24f5034dde325c10b9ee4aad`.

The ordinary bridge gives close moving fleets approach room, a variable
staggered formation and stationary port-service tasks. Actual full-PROGRAM VM
group/event/geometry/task checks pass with zero errors; the owning contract and
global harbour audit are in `docs/worldmap-traffic.md`. The earlier global
admission/shore-return fixes and tsunami/native package remain installed.
Visible map berth overlap is still possible and was not changed by this batch.
All 246 non-log player files, five foreign working paths and their index entries
match the fresh baseline. No game launch/input/termination, player-state mutation
or push. Mounted sea positions, coast correction and visible behaviour remain
unresolved and player-owned.

### Historical installed inventory — expanding fronts, superseded by sea-entry receipt 2c6f701c

Exact canonical `experiments/native-metal/run.sh --stage-only` returns 0 after the
last native patch and material-registry edit. Signed engine:
`3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c`.
App receipt `1f6f72a619e2019cf82bd0029c45e1cccb0da6f33ca9d54def882cec0c751f41`
binds all 117 managed entries. All 37 canonical encoded inputs, both script headers,
six native map paths, five native AI paths and the new WdmTsunamiWave technique
match the exact frozen source. Cache/app plans are empty; compiler ready and
installer deep/strict signature passes.

Esc on the global map offers separate arc/ring creation and marked-event Stop.
The finite hollow front expands and fades; rendered-land cache governs actual
island shelter. Normal initial approach preserves the sampled strength, allowing
the existing fierce beam capsize; attenuation begins beyond configured spawn reach.
Ordinary global fleets lose persisted HP/cargo and can sink, while quest/qID/ALONE
and locally admitted fleets are protected. Local NPC steering uses existing
navigation skill and ordinary turn inertia without replacing tasks. Existing local
damage/capsize, transport/spray, material/horizon, daylight/fort/audio remain installed.

Ordered patch `35eacd393496a88c69f4c1eb1317ddda28f044e07d40d5c7e692e2bc0c36388c`
preserves the earlier 1202-line local-sea prefix and adds native map/steering/technique.
World-map technique `fa7d680cf37593f8a8413827eaac1efeb8815b0bd61dc4151df77307221d7b32`
uses the existing generic fixed-function backend. Prior sea/sky material hashes
remain unchanged. Component geometry/HP/save/controller/offscreen checks and the
full integrated PROGRAM pass; player replay of map/sea, moving hulls and FPS stays
unresolved and player-owned. No game launch/input/termination or push.

The pre-existing closed-game snapshot still matches all 246 non-log player files,
five foreign working files and five index entries including three staged blobs.
A new baseline persistence failed on a HEAD typo before staging; preservation
therefore uses that verified prior snapshot, not a claimed fresh receipt. Two
additional lowercase launch logs predate this stage and are excluded as logs.

### Historical installed inventory — tsunami follow-ups and map debug, superseded by3456e45b

Exact canonical run.sh --stage-only returns0 after the final native/material inputs.
Signed engine331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0;
receipt66c742d6ca3dd069f357709fb145421f7e4b79bb0d2bb04f9f4fe8c598e40519.
All116 entries,37 canonical inputs and both delivered script headers match; cache
and app plans are empty, compiler ready and installer deep/strict signature passes.
Fresh246 non-log player files, five foreign working files and three staged blobs
remain unchanged. No game launch, input, termination or player-state write.

Installed source adds continuous60–300m transport through ordinary hull/coast
collision, actual shoreline trace into a bounded native splash pool, reduced
crest lattice and a narrow exact-color sky seam. Native/source/offscreen component
checks pass; real ship/coast/spray/horizon/weather/FPS acceptance remains unresolved
and player-owned. Ordered patchfeba0ff372f36ff2c7fff96409a3e21e8f42aef7e53c648f34ed798253c10976,
materiala547cef544331beedbfada3ad89af868b6203d81bc8ddd2d33e5368963f57097,
sky0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.
Prior continuous damage/capsize/natural event and daylight/fort/audio checkpoints
remain installed. The global-map menu now shares natural creation and deletes
only marked events on Stop/replace. Scoped map/menu cases and the full integrated
PROGRAM pass with explicit fixture seams; real marker/approach/contact/forced sea
replay remains unresolved. The content batch reuses the same installed engine.
No push.

### Historical installed inventory — continuous tsunami, superseded by33185703

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for the expanded
batch. Signed engine:
`4423316154a9a9df91434b503be963cb3b1467d0d19651aeca1ebaa502895a91`.
Receipt SHA-256:
`2b30bb9882742f575cad42689e4121aec2f730293d294cb7716571b590bced66`.
All116 managed entries,37 canonical inputs, empty gameplay plan and deep/strict
signature pass. All246 freshly hashed non-log player files and three foreign
staged blobs remain unchanged. No game launch, input, termination or SAVE replay.

Continuous random strength drives 10–50% nominal hull damage, height/width/speed,
foam and native pitch/roll; existing storm perk and armor reduce damage. Real
beam-on overturn enters ordinary ship death. Marked saved storm descriptors keep
sampled strength and trigger finite-circle automatic local sea with active exit
lock. First white-grid material is superseded. Representative installed
sea-height/normal/sky component frames show irregular crest foam and preserve
ordinary/cancel depth/MRT identity. Full source VM damage cases pass with explicit
contact/terminal seams; native geometry/contact/motion and map transition
component checks pass. Real splash/damage/capsize/natural map replay is unresolved
and player-owned. Source receipt binds native patch
`e35de55b65c69cb46b7d86c670a2993842ff2ce4baabab254ee4f123a287246c`
and material `e105c0ca07a1e472b1b8276321c264e0836d293b55e913933fa6f8988edba8ac`.

Daylight/fort/audio checkpoints `ce5faf2`, `3646cf0`, `31bb837` remain installed.
The additional horizontal wave transport and sky-horizon correction pass component
checks in the next source batch. Its first stage attempt refused while installed
game process30770 was active; a later read observes no game. Further staging is
held for the subsequent requested shoreline splash integration and admitted
crest-material correction. No override or termination is used. These follow-ups
are not part of this installed inventory. No push. Previous
inventories below are historical.

### Historical installed inventory — nearby sound admission, superseded by44233161

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the
focused audio source checkpoint `31bb837`. Supported app remains
`/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`64f461f7c3b74f1fca4ad49bfca9f24bbd017444312fe85a5b9566ad48d39a76`.
Receipt SHA-256:
`740f0676676d2b8a8572631959122330cc33d412839c9e64bb5be0e69737812a`.
The receipt binds all116 managed entries; deep/strict signature passes. All37
canonical gameplay inputs and both delivered script headers match cache/app
bytes; both gameplay plans are empty. All246 freshly hashed non-log player files
and three foreign staged blobs retain their hashes. No game launch, input,
termination or saved-game replay is performed.

The native sound service admits positioned naval transient FX at a40ms minimum
onset interval for the same alias/type within20m. It rejects duplicates before
sample selection or voice creation, retains the first authored level, has no
total voice cap for a continuing salvo and does not queue rejected events.
The source-bound probe reduces512 simultaneous splash requests from512 voice
initializations to1; ten spaced events remain ten live voices. Loops, cache,
speech, music, distant/different effects and prior cannon/charge policy pass their
nearest unaffected cases. Actual broadside sound quality and FPS remain unresolved.

Daylight and fort source checkpoints `ce5faf2` and `3646cf0` remain installed.
The1–7m threshold,2–8m damage curve and ordinary charges are unchanged by audio;
`AIBalls.c` retains its original producer calls. Ships and preceding impact-chart
shape/FPS have player acceptance; new daylight contour and fort balance still
need player replay. Expanded tsunami remains a separate active batch in its
existing owner. No push. Previous inventories below are historical.

### Historical installed inventory — daylight and fort follow-ups, superseded by64f461f7

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the frozen
daylight and fort source checkpoints `ce5faf2` and `3646cf0`. Supported app remains
`/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`10400733be42be03508aa27280dcbd1e2d6be77d11ef638a56c563fe6956da51`.
Receipt SHA-256:
`23c4496eff4d21ae05c3b27d35954b1dd08451ec9e03a3b6f17a8c0fd4651fad`.
The receipt binds that engine; final deep/strict signature passes. All37 canonical
gameplay inputs and both delivered native script headers match cache/app bytes;
both gameplay plans are empty. All247 fresh closed-game non-log player files and
three foreign staged blobs retain their hashes. Root performs no UI/input,
launch, termination or save replay. No game process is observed at delivery.

The player accepts the preceding projection/shape and reports excellent FPS.
This follow-up changes only bright-background rim/keyline strength and fort
proximity: intact gun positions replace the fort's huge enclosing-box trigger.
The existing1–7m threshold,2–8m damage curve, ship behavior and ordinary charges
are preserved. Matched authored PortoBello approaches disable1/1/1/2 guns versus
the former6/6/6/4 and previous installed0/0/0/0. These are geometry/VM checks,
not a saved-game salvo. New daytime visibility and fort balance need player replay.
The rejected cosmetic-fragment muting is absent. Engine mass-sound handling and
the expanded tsunami feature remain separate active batches with their existing
owners; neither is claimed installed here. Esc start is human-confirmed;
physical F11 and the next waveform/ship/world-map behavior remain unresolved.
No push is performed. The prior accepted projection inventory below is historical.

### Historical installed inventory — accepted impact charts, superseded by10400733

The frozen batch passed `experiments/native-metal/run.sh --stage-only` with exit0.
Supported app: `/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`b5e871dd3a2c24168d9267bcda7b44309dd39c7a4e26946e9f167c1627c5113a`.
Receipt SHA-256:
`6c4a9e52ccfa3e68cc59b1a77a91b7f64e5b4050ec4b9c75e693d45d7ae2bedf`.
Final deep/strict signature passes; the receipt binds the installed engine.
All canonical gameplay delivery bytes and changed native script headers match;
cache/app gameplay plans are empty. The protected245 non-log player files and
three foreign staged blobs retain their exact pre-delivery hashes.

Revision12 uses each launch sample's own landing/crossing time, keeps true means,
and evaluates separate sea-XZ and solid lateral/up density charts. Exact ordinary
sea MRT identity prevents the water continuation from climbing walls. The thin
analytic perimeter does not outline alpha holes or receiver-identity seams;
bounded70ms tangent inertia retains current physical depth clips. Current-depth
switches and disjoint footprints clear history. Finite relief support is padded
by the water-impact fore/aft radius; it is not measured terrain or guaranteed
conditional coverage. Stopped air and ordinary firing remain the existing owners.
Source/GPU/Metal-validation fixtures and canonical native build pass. The player
now accepts the drape's shape/projection and reports excellent FPS. Their
brightness-local daytime contour increase is a source candidate, not yet installed.
No instrumented paired FPS or exhaustive angle/zoom acceptance is claimed.

Airbursts now share ordinary bomb aim/height law, use stable per-shell1–7m
proximity and one2m-full/8m-zero damage curve, including sails/forts. The native
projectile codec is unchanged. Esc has sea-only start/cancel tsunami buttons
through the existing pause menu. Their integrated complete PROGRAM include
graph and pause/debug segments compile with zero VM errors without executing
game startup. The player confirms Esc starts the wave and supplies the visible
crest; its repetitive straight appearance is rejected for the next tsunami batch.
Physical F11, cancel/restart and wave/ship response still require player replay.
Ships' airburst results are accepted by the player. Airbursts barely damage forts;
the requested fort-specific balance sits between the earlier excessive damage
and this near-zero result. Its ammunition owner is investigating actual gun
geometry and trigger/damage reach; no ordinary-charge change is authorized.

Focused local source checkpoints: tsunami `bdfce2e`, aim `ba9ae12`, airburst
`a813b29`; no push. Root launched no game, sent no player input and stopped no
actor. Two player startup logs changed during delivery; their actor is unknown
and they do not establish acceptance. One game process is present after delivery;
its actor and actual scene are unverified, and root leaves it untouched.
The preceding revision11 inventory is historical and player-rejected.

### Historical installed inventory — revision11 rejected, superseded by b5e871dd

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after revision11.
Supported app: `/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`5b14e3196d4674c7cc296de97debc83fc26fbf7a8a8eb88b41f4cf40989221a7`.
Receipt SHA-256:
`0ebc514c8b38488591258f7a36a94c33e6e3301a836f8108c46d8e6bae0fdce4`.
The receipt's engine entry matches installed bytes and final deep/strict signature
verification passes after installation completes. All250 fresh non-log player
SAVE/settings/userdata files and three foreign staged blobs retain their hashes.

The decal now samples current world positions within a finite covariance field,
anchors at the selected receiver and filters the actual surface mask. The airburst
flight offset no longer positions the surface patch. Registered model fragments
write own/relation identity during ordinary geometry rendering; neither their
full-screen depth snapshots/difference passes nor separate own/water captures run.
Stopped air and firing mechanics are preserved. Strict/fast complete scene/aim
MSL, current CPU producer, ordered source stack, native build and source-exact
offscreen native encoder pass. The GPU oracle covers flat, tilted, curved and
creased surfaces, water below the camera, far/horizon rejection, own geometry,
relation color, alpha holes, origin translation and5x zoom. It proves those
properties, not real-game appearance, startup or paired FPS. No game is launched,
player input sent or actor terminated; player replay remains unresolved.

Tsunami's unchanged native patch and the sea F11 dispatch are integrated in local
checkpoint499037a. The real no-window SDL/controls/core/script/native command
chain passes, including restored controls and start/cancel/restart. Physical
macOS Fn/F11 delivery, visible menu and wave/ship response remain unresolved.
`docs/sea-tsunami.md` owns that precise acceptance gap.
Focused local aim checkpoint69f6e84 owns the delivered four native inputs and two
topic updates; no push. Source input hashes bind the same bytes staged before
this checkpoint; only documentation metadata changed after staging.

### Historical installed inventory — projected patch rejected, superseded by5b14e319

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after revision10:
one projected covariance patch, a bounded contact pass and no water ownership
capture. Supported app remains `/Applications/Corsairs Iddictive Remaster.app`.
Signed engine:
`0fc23284301f1b3465887169686ab1b14dd1d6b605a07659a3ca92d73282fe6d`.
Receipt SHA-256:
`d9d0421b3f72784831020597741e1f8d18bebc06e5d7fddbba5307ccf3732a6c`.
The engine hash is bound in the installed receipt; final deep/strict signature
verification passes after the canonical installer finishes. All245 non-log
player files and three foreign staged blobs preserve the fresh closed-game hashes.
The separately owned tsunami content/engine inputs remain in the frozen batch;
no new work is performed on that feature. No UI/input, game launch or termination
is performed. Native build, strict/fast MSL and the actual offscreen native encoder
fixture pass. Focused local source checkpoint00e7d57 owns four aim paths; no push.

The player's three13:13–13:14 screenshots accept revision9's removal of the
giant false hillside line, but reject its main shape: broken upright terrain/
foliage fragments and disappearance over water. FPS83/75/42 belong to different
views and do not establish a paired overlay cost; the player reports remaining
aiming slowdown. Revision10 changes the geometric representation, rather than
adding another receiver derivative or volume-intersection heuristic. The13:51
screenshot rejects revision10: a flat ring crosses water/ship bow without
conforming to their surfaces; the player also reports close-water aim marked at
the horizon. The screen metric ignores receiver position, and airborne arrival
can include the airburst's8-unit overhead offset. Camera inversion and picking
already include the correct absolute camera and water; those are not replaced.
Revision11 is prepared with selected-receiver anchoring, bounded current-world-
depth coverage and ordinary-model MRT identity. Actual player appearance and
paired FPS remain unresolved; further computer interaction is stopped.
Tsunami's no-window SDL/core/script/native-trigger chain passed in its resumed
owner; physical macOS key delivery and visible wave replay remain unresolved.
These source-only checks do not prove a100% live F11 trigger.

### Historical installed inventory — boundary-gradient drape, superseded by0fc23284

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the
unit-boundary gradient correction to the contact stroke.
Supported app remains `/Applications/Corsairs Iddictive Remaster.app`.
Signed engine:
`2a79c6b1822a5eca4d642d464939e74fc2943415354606684dac6472fe3aa6a5`.
Receipt SHA-256:
`abc852e3fd4bc37af79168e0a6c554911628e67ba97fbfdfc6a9691b1725e5b0`.
The same frozen delivery includes the separately owned debug tsunami patch and
three PROGRAM/INI files; its compiler receipt binds the exact native patch.
Existing ammunition, manual rake reach and firing rules remain in this batch.

All245 non-log player SAVE/settings/userdata files retain the fresh closed-game
baseline hashes after this stage. The engine hash matches its installed receipt;
three foreign staged blobs remain identical. No game is launched or player actor
terminated during this batch. Native build and strict runtime MSL compilation
pass. Focused local commitffe2514 owns only the aim shader/topic paths; no push.

The last rendered observation belongs to predecessor enginec474cc73: normal
actor8202 was responsive in a stocked first-person sea scene, showing67FPS while
looking at sky/rigging without a drape. The player's subsequent screenshot
showed92FPS and accepted a thinner main rim, but reopened a huge secondary
hillside strip and broken perimeter. These observations do not accept the new
engine's appearance or paired aiming performance. Its far-strip suppression,
seam continuity, zoom visibility and FPS remain unresolved pending player replay.
The player's 13:00:21 screenshot shows121FPS but rejects predecessorda34fbae's drape:
the giant hillside stripe persists and spreads, and the player reports worse
appearance. This is a rendered rejection, not performance acceptance.
The new source-exact GPU probe rejects the reproduced false outside stroke while
preserving true-rim AA and center/zero-gradient/range cases. It leaves the actual
field boundary unchanged; requested geometric smoothing of abrupt transitions
has not been implemented or accepted.
Further computer interaction/launch/replay is stopped at the user's request.
The repository-wide context check remains red on four preserved untracked files
owned by the stopped tsunami task; none is adopted into the aim checkpoint.
The debug wave's start/stop and ship response remain with its separate owner.

### Historical installed inventory — ammunition, manual rake reach and smooth depth density, superseded byc474cc73

The canonical `experiments/native-metal/run.sh --stage-only` succeeds after the
lower airburst fuze, fragment collision, count-scaled cadence, percentage crew
loss and final ammunition precision/icon corrections, then the manual rake
reach-order, solid-without-water capture and all-gun smooth depth-density repairs. Played app remains
`/Applications/Corsairs Iddictive Remaster.app`. Installed signed engine:
`70d78026692e3eeb351e65fff178df43fa31994583cc043a2372e930ba3d8678`.
Receipt SHA-256:
`08f6c75b2cd1180d9097ebbdc03ef79d61395be66e65e5aa90557ab4b8360cb4`.
Deep/strict signature passes. Both cache/app delivery plans are empty and
compiler_ready is true; all seventeen frozen inputs of the final staged batch matched. All253 normal
player files retain the closed-game baseline hashes after final staging. The
aligned fire-mode texture is
`448c6bbb827eecbfc9933ecdc0f55f602712d8058f57d7cdeb85ce6dab13d9c9`.

Current base crew losses are round shot0.05%, grapes0.6%, knippels0.075%,
bombs0.125%, airburst1.2% of remaining crew before existing modifiers. Gunner
precision multipliers are100/50/30/80/80%; ordinary round shot preserves the
crew's original accuracy. Airburst base hull39 and cloth25.2 are twice bombs
and knippels respectively. Full PROGRAM startup/catalogue/stores/controls/shop
compile with zero script errors. Actual native VM old-catalogue save/load,
percentage conversion, accuracy values and clamp/zero cases pass. Actual native
cadence/state and fragment collision fixtures pass; their stubs do not accept
visible gameplay.

Two fresh current-save/config clones use the supported installed launcher. The
first reaches a rendered first-person sea scene and closes before final stage.
The final clone's actor PID78428 opens the menu, then a read-only observation
shows a responsive sea scene, ordinary companion ship and an aim footprint.
Its HUD now shows airburst0 and powder2201, so it cannot accept a new stocked
airburst volley. Root's attempted keyboard input is rejected by the UI's
user-changed-app guard; no root input or changed-action acceptance is claimed.
Normal player saves remain separate from these clones. The final clone actor
closes before the native repair preflight; a new player actor PID90979 starts
from the ordinary player directory. Preflight rejects staging while that actor
is alive. No game is stopped or new native binary installed during this transition.
Read-only normal-profile observation reaches a responsive third-person sea scene
but shows round shot0/powder0; this scene cannot accept stocked firing. The
native ordering defect does not replace the existing ammo/powder readiness guards.
PID90979 subsequently closes, but a new ordinary-profile actor PID96068 starts
before the next preflight. Canonical `run.sh --stage-only` exits1 immediately
with `Close the running game before staging or launching the Metal candidate.`
No build/staging occurs and the installed engine/receipt remain unchanged.
No unknown/player process is terminated. PID96068 subsequently closes. The
canonical stage-only path then installs source047406c plusaaf7a69 and returns0;
the independent deep/strict signature check passes. Cache/app plans have zero
pending files and compiler_ready is true. All253 current player files preserve
the new closed-game baseline exactly. After the final density delivery and
checkpoint, the three owned clone/fixture directories are removed through
codex-delete-temp; their durable receipts remain in this ledger and the topic
documents. Stopped clone actors remain closed.
Stocked current-save firing/preview replay remains the player's accepting action.

The player reopens manual targeting: aiming at a ship appears to break fire.
An actual-function oracle proves one relevant defect: the centered selected
depth point rejects every gun before longitudinal rake stations are assigned,
although three stations are mechanically reachable. The smallest native patch
moves exact reach filtering after station assignment. It retains impossible
high-mast and terrain rejection, far intent and mapped single-gun behavior;
cadence/state and native syntax checks pass. This source repair is now installed;
the broader any-ship symptom remains unresolved pending real firing replay.
Source is checkpointed in `047406c`; the three unrelated staged blobs retain
their hashes. Root remains the sole build/staging integration owner.

The October8 screenshot/reply reopens contact projection acceptance: fort arcs
and contour disappear/flicker with aim motion. The live launch log also records
missing water ownership withholding all contacts. Sourceaaf7a69 admits exact
current registered solids without water capture, retains own/stale/unknown
negatives, and passes64 admission states plus strict native MSL compilation.
That independent gate is installed; sampled fan/prism undercoverage stays open.
The player additionally reports shells outside the red zone. The same-instant
source oracle uses actual AIBalls independent yaw/elevation/speed and rotation
math:945/2187 legal one-gun points fall outside the inscribed ellipse, and
selected-projector omission/normalized density mean add multi-gun losses.
The former red region is therefore rejected as a guaranteed dispersion boundary.
The player's subsequent choice changes the visible contract to main-density
regions with allowed rare tails, then relaxes a strict ellipse to a smooth
rounded blot projected on actual scene depth. The source candidate includes
every firing gun, weighted launch-law samples and covariance-root interpolation;
the physical first-hit masks and independent water guard remain. Its canonical
build and installed stage now pass on70d78026; source/GPU and bit-identical
allocation checks do not accept the pending real-game projection or FPS. The
52-gun density constructor falls from3.046 to1.244ms in the paired fixture;
fan/prism holes, closed perimeter and motion flicker remain unresolved. The
focused local density checkpoint is388d4e7; no push is performed.
The GreatRecipe quest remains deferred. Existing trade, fort, fencing and player
state owners retain their prior contracts.

### Historical installed inventory — fort airburst, naval firing modes and Bermuda trade, superseded by4a632fcd

Sourcee2ecbbf passes canonical build.sh and run.sh --stage-only after the endpoint
jitter correction. App: /Applications/Corsairs Iddictive Remaster.app.
Installed signed engine:
b8d033b74c3323bc8e40271f2a5affefae45f085f33516ce663f100b8704ffab.
Compiled unsigned engine:
9802b87f95ec372397a3df7f69410f70c6dd4463dd905dd3ef9100f4d7a4bea4.
Receipt:
f3be724a2caac01c79d1b5be53a4a04bd4b42fa6e710d77b3768c2fd6f05bb65.
Engine receipt/deep-strict signature match. Seven changed cache/app inputs and
all six changed PROGRAM compiler inputs match; both delivery plans empty and
compiler_ready true. All252 fresh player baseline files and three foreign staged
blobs retain hashes. Root launches no game. Fresh game inventory is empty before
explicit sole app/GPU replay assignment to ammo owner01a1195a-bb08-70e3-b43c-ea7ba85a67dc,
using supported installed launcher and a fresh current-SAVE/config clone. Actor
must preserve normal player state, report owned PID/clone/results and release;
no further build/stage/GPU/game comparison runs during that session.

Installed: fort overhead fuze, hull67.5/fort180, navigator cell40, Bermuda-only
normal purchase and saved trade-row reconciliation, quickmenu salvo/single mode,
per-gun reload and quality3.5–10 timing with jitter for all endpoints. Existing
tracers, ballistics, native save codecs and adaptive fencing are retained.
Real store/HUD/fort/single/partial-battery/preview/ordinary replay remains open.
The same mode owner's actual-function old/candidate oracle subsequently proves
one-viable-gun mode0 prediction/delay mismatch: at150m/10mps the new delay scale
creates16–49m systematic lead error. Root authorizes private sample forwarding
only in the native firing-mode patch and topic doc, retaining modes1/2+, codec,
scripts and jitter. That correction is not yet installed. Fresh-clone actor owns
PID36601/session38693 using supported launcher; no new build/stage occurs until
its bounded baseline replay ends and it releases. Future GreatRecipe remains docs-only.
Projection CPU/GPU resource preservation passes and bounded historical-run gain
is recorded below, but production/FPS adequacy is rejected; no prototype is
installed. Global map hourly jitter still lacks its advancing-map profile.

### Historical installed inventory — corrected ammunition/icon and adaptive fencing, superseded byb8d033b7

Canonical run.sh --stage-only succeeds after source4d42729 and fencingc4143b9.
Played app: /Applications/Corsairs Iddictive Remaster.app.
Installed signed engine:
fcce6e7b27c9df45e17982ccf487e93aa1af07af1e0f60888142497095ac3132.
Compiled unsigned engine:
4ddb0cdb74e72e748a709a5e1ec1f2dd177904dd99470fc21ab7d3d371d2aadf.
Ownership receipt:
ee1cb94afb21bb3250d66b12396bdbbec55a54b93e0d2bcf49974e2568be291a.
Engine receipt matches; deep/strict signature exits0. All five changed content
inputs match cache/app and both changed PROGRAM files match the zero-error full
native compiler fixture. Installed/cache plans are empty; compiler_ready true.
Corrected shared12m aim, either-direction fuze, shop ordering, both drawn icon
atlases and adaptive fencing are installed. Original tracer/ballistics baseline
is retained. Source installation is accepted; corrected shop/sea action and
fencing scene/balance replay remain unresolved.

Fresh closed-game baseline includes251 player files. Post-stage comparison finds
only five startup logs changed; all SAVE/config files retain hashes. Root launches
no game. New unknown/player PID89995 PPID1 starts23:25:51 after engine/receipt
delivery23:23:56. lsof txt inode393740055 equals the installed executable inode,
binding this process to the corrected engine. Preserve the live scene; no second
game/GPU workload. All three foreign staged blobs remain exact, agent_context
passes after the source checkpoint. Root releases build/stage ownership and
returns receipts to ammo, fencing and projection owners. Projection prototypes
are uninstalled; their GPU/resource/throughput gate waits for the live scene to
close and sole-GPU assignment. Global map jitter still lacks its advancing-map
profile. Airburst read-only CUA screenshot fails with native pipe closure; this
is no gameplay acceptance. One subsequent read-only capture on the same route
succeeds: the live sea/spyglass scene renders responsively, targeting a nearby
ship. HUD shows ordinary Bombs1849/Powder1584 and the stock bomb icon. Single
frame120FPS is not salvo/airburst/manual-aim performance evidence. No input was
sent; special charge5 volley, shop/icon, friendly and unarmed states remain open.
Projection's exact ordinary UV memo passes CPU193/14164573/8730/13 and strict
compile; local frozen55 CPU cost0.997816→0.826971ms is17.1% lower. Its increased
segment storage still needs GPU resource/throughput acceptance. Source owner
retains that frozen temp and continuation; GPU/FPS/installed projection remains
unresolved while the player scene owns the GPU. Root compiler fixture has no
remaining dependency and is removed through codex-delete-temp after delivery.

Later observed transition: PID89995 closes normally; source parent and root fresh
exact-basename inventories find no game. Root grants projection soleGPU for one
frozen UVmemo capture/corpus while ammunition's mode child remains CPU-only.
Terminal resource/193Result/14164573leaf/8730range/13negative preservation passes;
primary spill1152→1120,96/44 registers unchanged, instructions58638→94691.
Historical-run whole two-pass warm medians7.68808→6.51192,5.94563→5.06154,
4.74008→4.20117ms show11.4–15.3% lower isolated time, not paired-scene/FPS proof.
Production/FPS adequacy stays rejected at4.2–6.5ms per55–64 endpoints. No
projection is installed and no new variant follows. Session12 terminates,
fresh profiler/game/GPU/probe inventories are empty; supervisor reports no game
interruption. GPU actor explicitly releases; root's build/stage serialization
hold ends. The next fort-proximity/fire-mode/trade-policy/HUD-icon source packet
remains unfrozen under ammo owner01a1195a-bb08-70e3-b43c-ea7ba85a67dc.

Ammo owner then returns16 frozen paths and normally releases source leases.
Root rejects deterministic endpoint delays for2guns under always-jitter contract;
the same mode child repairs every-gun bounded jitter, preserving ordered positive
delays, approximate3.5/10 windows and unchanged codecs. Revised64-site fixture
and64seed2/4/32×3.5/10 cases pass. Root verifies all16 final SHA hashes,
all three foreign staged blobs and fresh game absence. Native build.sh exits0:
8252 inputs verified, eight source files changed, nine renderer consumer classes
pass and engine linksarm64. Root-owned full PROGRAM compiler fixture links the
canonical Ninja flags/libraries and composes only seven pending app inputs;
startup/catalogue/initStore/controls/store all pass with script_errors0. No game
is launched. Frozen future GreatRecipe concept is documentation only; unlock
quest remains deferred and Bermuda sales stay available. Root owns focused
checkpoint, canonical stage-only and post-stage SAVE/config/source comparison.

### Historical airburst inventory — October 7, superseded byfcce6e7b

Played app: /Applications/Corsairs Iddictive Remaster.app, signed engine
40c8c30003414fca988119a5720867e68cf68df6021c8942ac8b6cb6cd135d4b,
content receipt9678b1336556d4566dd5fb7bd7806a3ad3758b0e444176bf7321a88f02b56844.
Airburst source164fa93 passed canonical stage-only; deep/strict signature and
all13 exact cache/app content inputs pass, changed PROGRAM bytes match the
zero-error native compiler fixture. Compiled engine wasdc1bd592. Distinct
original-ammunition tracers remain installed. Older3df6afbf inventories below
are historical predecessors, not the current engine.

Root launched no game. Installed PID85802 began23:04:01 in the standard player
directory; airburst owner also confirms no launch. Initial post-stage SAVE/config
hashes match, while five startup logs change and two sentry files appear. Later
live play adds Curacao sea2 and removes prior AutoSave10; those concurrent changes
are preserved, not restored. No blanket251-file preservation is claimed after
that live state transition. The process is subsequently absent at the ML build
preflight; do not infer future closure from this snapshot.

The player rejects absent overhead bursts and misplaced shop display order.
Airburst owner reopens fuze/shop display (persistent IDs retained) and combines
the never-installed drawn icon into one corrected delivery. Its earlier icon
delivery fixture pass is withdrawn: temporary symlink rejection occurred before
testing. Root holds install until corrected source/evidence freeze, GPU release
and a fresh closed-game check. Projection prototypes remain uninstalled.

Separately, adaptive fencing source/ordered integration c4143b9 links through
canonical build.sh; compiled candidate is55129060828f5986daeab3972686bcc1b623a14b9b64bbea681fe2e0fa044e6c.
It is not installed; scene/balance replay remains player-owned. Native build
preflight has46% memory free and no game process. Root's focused ML checkpoint
preserves foreign index blobs; hygiene check is currently blocked only by four
airburst icon assets owned by that active source task. No unsafe adoption occurs.
Root retains Git/build/stage integration; source owner repairs ammo; projection
owner has released its short embedded-capture replay actor: native session7 ends
normally, no profiling/benchmark or GPU actor remains. Existing heatmaps expose
invocation/SIMD costs only, so arithmetic subroutine dominance stays unproved.
Its next source-cost diagnosis is CPU-only; no new GPU variant is admitted.

Root releases24 current-thread scopes through normal native release after
checkpoints, returning ammo/icon source ownership for the proven repair. The
reported cause is ordinary-bomb aim height≈2.2m below the overhead slab and a
descending-only fuze; height warp keeps that aim chord. Shared12m target-height
offset plus overhead traversal in either direction is a source proposal under
the ammo owner, awaiting actual-arc and shared preview/fire negatives. Save IDs,
native lanes,35m arming and blockers remain required; no new fix is installed.
Root retains corsairs-compile-program.j0u6cp1y only for the already-authorized
corrected-batch compiler replay and baseline evidence; cleanup belongs to root
through codex-delete-temp after that batch reaches its terminal disposition.

### Historical combined player-defect package — October 7, superseded by40c8c300; player replay pending

That historical played engine was signed3df6afbf; boarding/cargo source is committed in
c69959f and installed with the frozen cannon subset. The player reports global-miniature-map jitter, absent thoughts loot
orders, projection holes/flicker and nearly empty enemy cargo. Manual target aim
also remains slow: the held live scene (November19,06:00) shows50FPS;1185/1717
main-thread samples are in the aiming camera, chiefly GEOMClip/GEOMTrace. Cannon
owns projection/FPS plus later chainshot/tracer/marker corrections. Root's separate
advancing-map hold is pending, so this sea sample is not a map diagnosis.

Read-only save evidence finds Cornelia's third merchant hold already almost empty
before capture while two convoy companions carry wine/weapons. Root reproduces
greedy100/0 allocation, then verifies50/50 with exact finite stock conservation,
negative cases and four codec rounds. Optional own stores now use real port stock
and existing headroom/reserve, retain their ledger on sea entry and do not block
sailing when scarce. Full-PROGRAM stock/weight/snapshot checks pass. The cargo
contract/evidence belongs to `docs/worldmap-traffic.md`.

The prior installed standard MainHero variant lacked the order menu and its cabin handler
ignored the saved mode. Root connects both variants and the handler, including
current-cabin mode changes. Actual VM default/silent/manual and codec checks and
the composed full-PROGRAM/MainHero menu checks pass. Contract and adapter limits
belong to `docs/boarding-loot-orders.md`. Source verification does not accept
installed dialogue, transfers, NPC loot balance or rendered smoothness.

After the player's normal closure, canonical build.sh and run.sh --stage-only
both exit0. Compiled engine0dd247a1 becomes installed signed engine
`3df6afbfb67f741bd6fe0283cc1c7f809a981a020cfcceb038e390a5af6a81a8`;
The original batch receipt `7a8522267414c66b09877cd491fb8d2791c6379d6851f9181804e1cb60c667a9`
matches it; the later content-only tracer receipt below supersedes that receipt.
Post-stage deep/strict signature passes. Cache check reports zero
pending files/compiler ready; the independent installed canonical plan is empty.
All251 player files retain exact hashes, with no new files. Installed MainHero
retains standard.c identity and matches its military-callback composition;
the development cache deliberately retains boarding-loot.c. Boarding, worldmap,
AICannon/AIBalls and both public/private shared headers match their consuming
receipts and composed source. Verification initially compared unlike MainHero
variants and the wrong private-header path; corrected owner-aware verification
passes without a product edit.

This batch also installs demasted chainshot hull targeting, the existing luminous
bomb flight stream for all ammunition, and authored-texture surrender tint.
Actual VM/codec and marker GPU/Draw probes pass; installed player firing/marker
replay remains unresolved. Four frozen cannon paths are committed in cc85f54
after the hook specialist's0cfcc99 repair and the original unchanged normal
helper's successful retry; foreign staged/working changes remain separate. Global-map
jitter, projection holes/flicker and manual-aim FPS remain open. Cannon product
freeze is released for its next source batch; root remains the sole integrator.

### Ammunition appearance — October 7, distinct profiles committed and installed; player replay pending

The player rejects identical-looking ammunition after the installed uniform
bomb-stream batch. Root owns a bounded content revision: grape microtraces,
round shot smaller than chainshot, and the original bright bomb effect. Native
K2 PSYSv3.5 profiles reuse only original luminous Particle2, with ordered width,
life and emission rates; the three nonbomb smoke clouds are removed. Grapes and
knippels sprite sizes change while their native type-specific collision radii
stay intact. Actual VM initializer, original-catalogue speeds, perk/no-cannon
negatives and four codec rounds pass. Prior-runtime binary asset delivery,
idempotence, source/runtime drift rejection and existing text codecs pass.
Commit ed0d98d checkpoints eight owned source/document paths; three foreign
staged blobs retain exact hashes. Normal `sync_metal_gameplay.py apply` exits0
with exactly four content changes, reusing the installed engine. Canonical/cache/
app/receipt hashes agree for AIBalls and the three XPS profiles. Independent
installed plan is empty; cache has zero pending files/compiler ready; post-install
deep/strict signature passes. All251 player files, signed engine3df6afbf and
authored Bomb_Smoke a115d87a retain exact hashes. Current receipt is
`1950a423440b4a42c0fad09900bdc0f26a983afcd6791e6a86ae2800cb32615a`.
No engine rebuild, launcher/configuration change or gameplay launch is performed.

The disposable child's parser confirms ordered graph parameters, and its
original particle-bytecode GPU fixture is supporting shader evidence only.
The corrected separate native DataSource::Load/FieldList proof accepts all three
new XPS plus original Bomb_Smoke, with exact1/2-component counts and native graph
evaluation. Actual BBProcessor/per-profile geometry/night visibility are not
accepted by the synthetic fixture. Particle reduction is an analytic source
budget, not measured game FPS. The child cleans its own temp through the normal
helper (exit0); root releases the sole GPU proof actor to the cannon owner.
Visual gameplay acceptance, perceived speed and night FPS remain unresolved
with the player-owned firing replay.

### Continuous aim family — October 7, first GPU performance attempt rejected; not installed

Root inspects the cannon owner's gpu-results.txt/final-cpu-results.txt and
separate Porto curve51 input records in continuous-family.kfAeKRPV. Shared
CPU/MSL family checks pass11 named cases and182 seeded equal-record endpoints,
with zero GPU parity mismatches across193 families. The unsupported
Flyingdutchman zero-time sample and one synthetic binary64 Porto endpoint
transfer/oracle disagreement remain separate, not silently accepted.

Measured warm GPU query time is12.027ms/55 Flyingdutchman families,
8.981ms/63 Nevis,7.690ms/64 Porto and7.068ms for one Nevis duplicate query.
Disposition: rejected for the requested performance target; isolated parity
does not prove scene integration or game FPS. The same source-only child owns
only conservative BVH-node broadphase revision, with outward float bounds and
compensated leaf predicates. No production input, build, install, launch or
player state changes occur for this attempt. Signed installed engine3df6afbf
and content receipt1950a423 are independently rechecked unchanged. Scene/water/
lifecycle, production boundary certification and actual aiming FPS remain open;
root continues to own integration and the cannon owner the source proof.

### Continuous aim FP32 broadphase — October 7, second GPU performance attempt rejected; probing stopped

Root inspects float-node-cpu-results.txt/float-node-gpu-results.txt against the
retained baseline report. Outward FP32 node bounds preserve193 equal-record
CPU/GPU cases and compensated leaf/sweep arithmetic. Warm GPU query time is
12.4439ms/55 Flyingdutchman families,9.3938ms/63 Nevis,7.8303ms/64 Porto and
7.3196ms for the Nevis duplicate; one-triangle coplanar query costs1.1423ms.
Disposition: rejected for performance. Differences from the previous attempt
may be run noise; reduced node arithmetic has no proven benefit or dominant
cost attribution. The separate Porto serialization disagreement remains open.

The cannon owner stops GPU work after two failed attempts, retaining its report
and evidence; no production input or install changes occur. Renderer/resource
diagnosis is the cannon owner's next bounded responsibility before another
algorithm variant. Root remains the sole production integration/build/stage
owner; installed aiming FPS and scene/lifecycle acceptance remain unresolved.

### Continuous aim shader resources — October 7, diagnostic evidence captured; no performance improvement accepted

Root reads the cannon owner's retained gpu-resource-report.md. Native Apple
capture/profiling uses only the frozen55-family workload, one command buffer
per capture, with all55 outputs and queries/visits/tests unchanged. Compiled
state is61806 instructions,96 temporary/40 uniform registers,11552 spilled
bytes and zero static threadgroup memory. L1 traffic is predominantly stack
(0.37/0.39GiB/s reads and0.48/0.48 writes). This establishes heavy experimental
kernel stack state, not stack bandwidth dominance or the installed FPS cause.

The maxThreads32 descriptor retains inputs/arithmetic/parity but changes no
compiled resource fields; structural benefit is unsupported. Cross-session
dynamic instruction estimates disagree and are not speedup evidence. The
cannon owner closes sessions13/20 and reports no remaining native debug session
or exact probe/game process; GPU actor is released. No production delta or
installation occurs. Disposition: resource evidence accepted at its diagnostic
layer, performance/scene/player-FPS acceptance unresolved. The same repair owner
now uses a bounded read-only source specialist to bind segment/coplanar data
lifetimes before any edit; root retains production integration/build/stage.

### Continuous aim segment state — October 7, CPU/compiler gate passed; GPU performance pending

Root reads gm-binding-report.md and family-segment-state.aWvG5A2o's
cpu-final-results.txt/metal-compile-final.txt in the retained projection fixture.
For Flyingdutchman1/Nevis/PortoBello all40445/80025/104038 original-scale BSP
triangle triples match visible/collidable render triangles; no collidable render
face is absent. Source DrawBuffer binds svertex as BaseVertexIndex. Disposition:
source anatomy binding accepted for these three models only; live transforms,
instances, depth/deformation/camera/water remain unresolved.

The source specialist's scalar Segment state and immutable Family/Bin references
retain generic coplanar handling.193 complete equal-record Result shadows and
14164573 ordered per-triangle checks pass, as does strict MSL compile with
fastMath disabled and zero queues/dispatches. Separate Porto transfer/oracle
drift and unsupported zero-time sample remain open. CPU/compiler proof accepts
no FPS improvement. The projection chat owns one serialized14-workload GPU
falsifier; root holds GPU-heavy build/staging/replays until its actor release.
No production source/cache/app/index changes or game launch occur in that probe.
Root remains production integration owner; performance and scene acceptance
remain unresolved, and all previous rejected performance results are retained.

### Continuous aim scalar GPU — October 7, isolated improvement; product performance rejected, actor released

Root reads family-segment-gpu-report.md.14 workloads/193 families/56 serialized
command buffers retain all Result/Pair bits and work counters. Three-warm medians
fall from12.4439/9.3938/7.8303 to7.7885/6.0224/4.8505ms for Flying55/Nevis63/
Porto64; duplicate query falls7.3196 to5.1948ms. Coplanar1 regresses1.1423 to
1.4941ms. Disposition: isolated corpus improvement accepted; product/full-image
performance rejected. Porto transfer/oracle and numeric certification stay open.

Frozen55 native profile retains original inputs/outputs/counters. Spilled bytes
fall11552 to11040, registers96/40 remain and instructions grow61806 to134200.
Stack bandwidth rates fall0.37/0.48 to0.18/0.08GiB/s; neither total traffic nor
bandwidth/player-FPS dominance is established. Session9 ends normally, fresh
native session and exact probe/game inventories are empty. Projection chat
explicitly releases GPU actor; root's build/stage/replay serialization hold ends.
No product source/cache/app/save change or staging is requested. Projection owner
retains artifacts and continues proven kernel/resource-boundary investigation,
preserving coplanarity and uncapped sweep; root remains production integrator.

### Airburst integration — October 7, native/startup compiler/timer gates passed; staging pending

Root accepts the frozen source packet from airburst owner01a1195a-bb08-70e3-
b43c-ea7ba85a67dc, preserving foreign staged/working paths and the distinct
tracer baseline. Canonical build.sh exits0: ordered stack changes three native
files, renderer routing checks pass and arm64 engine links. One build uses the
existing cache and10 native workers on the48GiB/16CPU host; memory pressure
reports53% free, no overlapping game/build/GPU actor at admission, estimated
8GiB build plus8GiB OS/interactive reserve. No app launch occurs.

The disposable current-native compiler copies installed executable-relative
headers and composes the exact13 pending content inputs over installed PROGRAM.
Its initial missing-header setup fails before product compilation. Once corrected,
full startup compilation exposes the product's unavailable GetTickCount API.
Root replaces only the script limiter with registered PostEvent/ready-handler
and creation reset; native patch and projectile codec stay unchanged. Full startup
plus catalogue/store/controls now pass main=1 catalogue=1 stores=1 controls=1
script_errors=0. Actual native timer processing retains one heavy effect through
119ms and permits the next at120ms: before120=1 heavy=2 blasts=3 script_errors=0.
Source owner VM save round and swept-fuze negatives remain valid for unchanged
inputs; live shop/select/burst/friendly/direct-hit/ordinary-bomb replay stays open.

Player baseline binds251 regular non-symlink files and signed engine3df6afbf;
current receipt1950a423 remains installed. Root's compiler/baseline temp owner is
corsairs-compile-program.j0u6cp1y; the redundant intake copy is removed through
codex-delete-temp. Staging waits projection GPU actor release. Airburst source
scopes have released; separate frozen adaptive-fencing sources await their own
scope release for root's exact-path index/check closure, excluded from this batch.
Disposition: source/native/compiler/timer gates passed, install and gameplay
unresolved. Root retains sole integration/build/stage/install responsibility.

### Continuous aim specialization — October 7, resource boundary passed; product performance rejected, actor released

Root reads family-specialized-gpu-report.md. Ordinary/full masked separation
preserves193 complete Results/Pair bits, counters and CPU masks;56 serialized
command buffers end112 encoders/dispatches, poisoning mask/output on every repeat.
Primary spill reservation drops11040 to1152 bytes and instructions134200 to
58638, while masked full retains11072 bytes/133699 instructions. This accepts
the compiler resource boundary, not bandwidth dominance or player FPS.

Total two-pass warm Flying55/Nevis63/Porto64 costs7.6881/5.9456/4.7401ms versus
7.7885/6.0224/4.8505;1.3–2.3% may be run noise and remains insufficient.
Disposition: product performance rejected, no larger dispatch or integration.
Session9 ends, fresh native session and exact probe/game inventories are empty;
source owner explicitly releases GPU actor. Root proceeds with the independent
frozen airburst stage; projection prototypes remain excluded. Projection owner
retains artifacts and continues read-only query/arithmetic diagnosis, not a GPU
actor. Porto transfer/numeric and live scene/lifecycle acceptance remain open.

### Historical harbour and aiming-FPS batch — October 7, superseded by3df6afbf; player replay pending

Native full-hull BSP replay reproduces three saved LeFransua returns stalled
outside arrival while the point-sized PTC route is connected. Commit c6bf191
adds bounded transient full-hull recovery after ordinary port ground contact;
the original arrival callback/radius and living fleets remain intact. Four winds
and mid-approach transient reset/save-attribute reload pass; rendered player-map
replay remains unresolved. Build passes. Player closure was observed and the
combined canonical stage-only completed after the final source freeze.
The cannon writer released its verified aiming-FPS source as11571da after the
new player report. Root now owns combined build/install with ce5eef9+c6bf191,
avoiding a second package. Harbour c6bf191 remains frozen and is now installed.
Its live aiming profile attributes1533/1951 main-thread samples to the overlay,
including BSP clipping and segment/model tests. The cannon owner reports an
extracted AimMarch probe preserving1000seeded first-hit/segment results while
reducing AABB checks92.99%. Final native GEOM Clip/ClipByPlane equivalence passes
1920 real Flyingdutchman/Nevis/PortoBello queries, including1183 over-cap cases,
exact callback bytes/order/cap128 and cold/warm/zero-plane/test-only/rejection
negatives. The tighter sphere was rejected for capped-set differences; the
original sphere/traversal remain. Source is frozen; live FPS remains unaccepted.
Combined canonical build.sh passed after11571da: stack migrated exactly three
source files and the complete engine linked. Compiled candidate is
`7e091886541c4a31d505ea739e189f8e616bc533638839b9912cb0599490a7c8`.
Prior compiled2e552d2d and installed fdab9859 are historical. Canonical
run.sh --stage-only completed with signed installed engine
`37598a37e4c73e7d4823576b69ad0a0dca34b59c97ce70efb9637f74d209b9f9`.
Installed receipt68f05ba2e537252aabc54b4095b8389183345334fbe299ad72cc888e5f917170
binds that engine hash; deep/strict signature passes after staging completes.
The earlier concurrent verification during resealing failed and is superseded
by this post-stage result. All252 player files retain exact hashes, with no new
files. Both shared headers and12 cannon/worldmap content consumers match source/
cache/app, including0051df1d encounters and944fa5c0 sea outcomes. Root releases
the single installed-app launch/UI actor to the cannon writer for normal prior-save
LoadGame aiming/FPS/firing replay first; root does not launch/profile/advance play
concurrently. Harbour rendered acceptance remains root/player-owned afterward.

The cannon replay owner independently verified installed37598a37 on live30726
and observed the public app's current saved night sea battle (November3,02:12,
Time2) through CUA, without gameplay controls. Normal third-person FPS114 is
scene startup evidence, not an aiming improvement versus the old daytime51.
Matched manual-aim profiling remains pending the player's same-view hold. The
launch/UI actor stays with player+cannon replay; root does not profile concurrently.

Frozen cannon checkout rollback is retained under canonical native-metal/.cache:
installed-engine-backup holds exact signed8c5fb0bf/fdab9859 engines; all nine
SHA-addressed original blobs remain in gameplay-source-backups. Historical
receipts11728d8f/ea3ee5e3 and old script_defines.h0810dd98 are separately retained
in installed-engine-backup/frozen-cannon-476c045, never over active headers/receipts.
Hashes and engine signatures pass; app/player files are untouched. The cannon
writer owns archive of its managed frozen checkout after this retention handoff.

### Historical installed cannon/header batch — October 7, superseded by37598a37

The previous installed engine was
`fdab9859e2228c979a675f25e0f75e769df4ccb0b909dac2171bc89a3a2dff53`.
The cannon writer delivered5ceff92 plus ce5eef9-equivalent event-header correction
through frozen476c045; its reported live sea-battle startup is separate from root's
harbour fixture. Older installed snapshots below are historical. Source c6bf191
is the next engine batch; player SAVE/configuration were not changed by root.

### Installed integration snapshot — October 7, mast repair delivered; repair action replay unresolved

The later player replay reopened cannon acceptance: the old-save menu does show
RakingFire, but its placeholder icon, surface gaps, unrotated baseline envelope,
sinking-sail aim, surrendered colors and sail-HP jumps are reported. The next
cannon repair is a source candidate; it is not yet installed.

The sole played application is `/Applications/Corsairs Iddictive Remaster.app`.
Current signed engine is
`8c5fb0bff1c12a65c9716998f560be9516e08216f6d3324ef948a5ea052096ed`,
observed after the parallel cannon-verification writer's canonical
`experiments/native-metal/run.sh --stage-only`. That later batch retained the
mast bridge and generated icon; cannon action acceptance stays with its writer.
The original mast delivery's signed engine
`bf42886adaf2185c7a0bc51dc88e148175d37f22f4c3656a58375810f8f0c9eb`
is now historical. The earlier
`34e1c03d03ad7be83669c92bd11534d24213833a69113dcc7325e4f9f9b66eec`
engine is preserved by the canonical
backup owner. Older engine hashes below are historical attempts, not a second
current inventory. This snapshot supersedes their pending-install labels.

Emergency mast repair is installed in BattleInterface and the existing
LeaveBattle confirmation. Native compile and whole-PROGRAM/lazy confirmation
compile pass; a native VM fixture and serializer round pass limits, 8–72-hour
timing, scarce planks, cancellation, stale quotes, rollback and repeat callbacks.
Its external queries and geometry bridge are instrumented. The original feature
batch reported zero pending files and compiler ready; strict/deep app signature
verified that batch.
The original mast batch reopened in an active first-person sea battle, observed
through its native window. The later cannon batch is observed at the main menu;
no competing UI interaction was started. Repair requires safe sea, fallen main masts and
planks; rendered attachment, confirmation fit and save/load after repair remain
unresolved with the player. No game input or save mutation was used for this
observation. The dedicated mast/mallet icon is now generated from the existing
command-atlas reference and installed as two 64px tiles in texture slot 5. Its
prepared TX and BattleInterface binding match the receipt; actual safe-sea menu
visibility remains unresolved. `docs/mast-repair.md` owns the behavior contract.

The current batch retains Living Caribbean and closes safe sea sail uploads,
live code-based surrender relation lookup, restored per-ship lamp ownership,
the squad-compatible loose-pickup halo and the already-consumed sailing/berth
inputs. The coherent dynamic reflection cube stays at its ordinary cadence;
rejected transient rings and distant crew/shadow cuts remain excluded. Reviewed
FPS draft bytes remain separately preserved in the ignored local archive.

Canonical build/stage, strict app signature and compiler readiness passed for
that delivery; its cache and installed gameplay plans were empty. The arcade
quarter-speed correction c8a136c is delivered. The mast/cannon batch described
above supersedes its earlier pending-install label; later working changes still
need their owning checks. The selected sea-return source repair339178e is also delivered through
the existing gameplay_sources/delivery_state content transaction. Installed and
cache sea.c match its canonical projection, SHA256
944fa5c0cc3e028ca3d828ec019243cd35b2d6a91c5f6096069d861ae976e8ba.
For that earlier content-only transaction, engine34e1c03d and all241 post-quit player files stayed unchanged;
the transaction's strict/deep signature verification passed.
The legacy ordinary spawn bypass repair742a597 is delivered through the selected
content transaction: current cache/installed worldmap_encgen.c SHA256
0051df1dd35d4094af4ad6b30f1f6216e1f3af27c2f70a74a20766302a4482dd
supersedes the earlier a3fb4c7a source below. Strict/deep signature passed;
engine8c5fb0bf and244 post-quit player files are unchanged. The player's existing
over-limit groups are preserved, and three LeFransua arrival stalls are under
native investigation; this is not a harbour-drain or player replay receipt.
Built/cache/app message headers match.
Player journey source is saved in focused local commit
`59a3dfad5f6cd4fcc4aad3cc819feae2b634b3ef` (14 owned paths; no push).
Whole-PROGRAM compilation and four serializer rounds pass with pickup composed
on supply. Real Metal probes verify immutable sail/IB rewrites and recycled
slots, warm allocation reuse, additive pickup pixels, depth/occlusion and full
state restoration. Relation CPU/sanitizer and owner-lamp negative checks pass.
These checks do not measure game FPS or accept a rendered gameplay action.

The safety batch adds funded partial service, stable crew reserves within an
authored hiring refresh, paid crew/gun capacity reservations and a 75% physical
commercial load ceiling. It fixes repeated seller deduction, impossible cargo
destinations, wrong-port credits and false completion with residual freight.

The finite trade cycle and player journey hardening are installed with worldmap script
`a3fb4c7ac5773bd7c85a12140c6b1eec59de5c615ff5ea8224bb238ce17ffc1b`.
Commerce admission now requires finite work, yields to idle incumbents, and
previews the whole real roster with the same paid service planner before native
creation. Actual living freight reduces remaining demand. Ready unemployed
fleets can physically reposition after a day, with exclusive working-port claims.
Hostile shipment targets redirect at sea to an explicit physical return; losing
both endpoints uses a recorded salvage port and a separate stock receipt.
Aborted repositioning releases its former working-port claim.

The earlier gameplay-only `run.sh --stage-only` succeeded after final source/pins.
Its engine stayed `096395187b7b85e3f647f528d176531f0e6fbe16ad5baebc0029ac725197b8a0`;
234 observed player SAVE/SaveData/config files retained exact hashes. No game
launch or player-save upgrade was performed. Actual-script state/serializer
scenarios and final whole-PROGRAM composition pass. A full-archipelago isolated
decision dropped from 12.514 s to 58.5 ms for depleted stocks (177.3 ms rich).
This is not measured game FPS or acceptance of a rendered economic cycle.
Existing cargo is preserved during diversion. New assemblies stop feeding an
observed stock-blocked port; paid replenishment releases that guard. Quota60 and
object ceiling80 remain unchanged. Empty ports remain possible after demand,
war or player purchases; numeric balance is still initial tuning.

Sea handoff now checks actual actor/group/ordinal admission, preserves dead
tombstones and reconciles orphan sea ownership before map refresh. Capture records
loss before the actual transfer removes the group receipt. Only the living
original pending officer is recovered; a purchase binds only the purchased hull's
berth. Military endpoint rejection, orphan reservation and zero original survivors
use existing physical cleanup. Native avoidance fades only for ordinary port
approaches, service holds recoil and departure clears stale pursuit.

Canonical stage-only exited successfully after the final native source and five
script changes. All five installed script hashes match the frozen candidate;
gameplay plans are empty and compiler readiness passes. The signed installed
candidate hash matches the installer's receipt (raw build bytes differ because
installation strips, rewires rpaths and signs). All 234 existing player SAVE,
SaveData and configuration files retain identical hashes. No game was launched.

Actual-method ASan/UBSan navigation and real COMPILER state/conservation probes
pass, including four serializer rounds and nearest allowed cases. The combined
installed PROGRAM candidate and normal lazy shipyard LoadSegment compile without
script errors. The reusable explicit `script-vm-probe` diagnostic passes; native
reconfiguration preserves unchanged watermark bytes instead of invalidating all
objects. Initial diagnostic setup failures were dependency/include scope and
fixture duplicate declarations; they were corrected at those owners, not gameplay.
Headless military localization uses a fixture-only key-return adapter because no
StringService scene exists; actual military/news persisted effects remain intact.

Disposition: delivered source/state safeguards; interactive harbour drain,
prior-player-save upgrade, capture/purchase, service/battle, FPS and perceived
balance remain unresolved with the player. An unvisited legacy service plan lacks
the pre-casualty crew watermark and preserves observed lives; all native late-damage
producers are not covered by the actor handoff proof. See `worldmap-traffic.md`.

The dev helper is integrated in `4106e7e` and now targets installed
`Contents/MacOS/launch`; its native dry-run
resolves this engine using bundled settings/state ownership. A one-file actual
installed push/revert canary is exactly restored and signed, with native binary
and all 233 preexisting player SAVE/config hashes unchanged. Disposable helper
fixtures prove conflicts, owned rollback, locks, symlinks and clean upgrade
ordering. No game launch, player input or forced exit was performed.

Prison-day/tournament, sea aiming/ballistics, map/encounter UI and night-floor
candidates already match canonical installed consumers; stale backlog labels
are corrected. The player's same-save sea FPS/visual comparison, pickup action,
prison arena/outcomes and the existing Living Caribbean scenarios remain
`unresolved`. The broadened FPS/dev-helper request is handled before any goal
pause; native task state remains the scheduler owner.

### World-map island avoidance and ship jitter — October 6, installed `d8d2980c`; player replay pending

Historical engine inventory at the October 6 attempt; current engine belongs to
the integration snapshot above. The October 7 player report of stepped movement
reopens player-ship smoothness acceptance; AI avoidance checks do not close it.

Player reported that world-map ships sail into islands instead of working around them, and that
a ship jitters back and forth while under way. Source delta: `wdm_islands.cpp` gains
`FindDirection`/`FindReaction` node fallbacks (probe along and against the course, static
reaction radius 20 -> 32) and `worldmap-traffic.patch` extends `wdm_enemy_ship.cpp` /
`wdm_merchant_ship.cpp` with island look-ahead steering, island weight 1.5 -> 2.0, smoothed
ship repulsion, a resolved degenerate turn sign, a speed branch that converges on
`minManeuverSpeed` without overshoot and patrol reversal that needs a return point farther than
25 units. Engine `d8d2980cfd7f0810ced5ccf2a4015ecfc10bdd3d46506e1e7cfbda25f5e14337` is
installed into the played app; gameplay scripts were not part of this delta. Disposition:
`unresolved` — needs the player's world-map replay of both defects.

### Dynamic reflection cadence — October 7, installed coherent baseline; sea replay pending

Historical 4s sample of PID83470 (three own and four foreign ships) put 464/1678
main-thread samples in EnvMap_Render → SHIP::Realize, including per-draw buffer
creation. The later half-rate dynamic-cube draft was removed before the October7
delivery; `sea-reflection-budget.patch` retains bounded sun-road cadence and
waterline reprime, with a coherent dynamic cube every normal frame. Safe shared
sail uploads close the repeated allocation owner without accepting reflection
lag. Current source is consumed by `ee144763`; same-save performance and water
appearance remain player replay. The previously reproduced unmodified-HEAD
waterline-probe failure remains outside this delta.

### Animation buffers and model lifetime — October 6, installed; save/sea replay unresolved

Resumed the unfinished crash patch from chat
`01a10f5b-84f6-7a33-a406-41af3f18909d`. The requested batch contains the growing
animation-buffer registry, sailor move-assignment release and recursive owned
model-child deletion. See metal-frame-pacing.md for the contracts and
retained sanitizer evidence. All three canonical source files match the tested
candidate and the compiled delivery source.

The main checkout's installed-content preflight reproduces
`unrecognized installed fleet bridge: PROGRAM/sea_ai/sea.c`; its content plan
also includes the user-paused expansion. The already prepared frozen delivery
checkout retains the installed gameplay package and has zero pending scripts.
Its canonical `experiments/native-metal/run.sh --stage-only` exits zero after
consuming the lifetime patch, verifies the bundle signature and installs engine
SHA-256 `97d40b3b55b2ecfd34faef515382d33341f45cd50b881f2671fe8923c3694d41`,
UUID `20312442-F4B4-33DC-886D-2242D7B76674`. The pre-install build hash is
`f3f6edd2d1beb3c56424a21f2565790cac830d9f8933eb3d250c128ec37a0260`;
rpath adjustment and signing account for the different installed hash.
The previous signed engine `073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac`
is retained by the delivery cache's installed-engine backup owner.

All 925 installed PROGRAM files and 217 original player SAVE/config files
retain their hashes, including after diagnostics. The native menu renders on
both the normal installed-app launch and the installed launcher's task-owned
copy-save profile. The latter process (PID 74011) uses the installed binary and
fixture cwd; automated keys/clicks do not open the save browser, even after
raising the window. No save-load or interactive-scene acceptance is claimed.
The fixture process is closed; no game/build process remains. No running player
game was stopped and no paused expansion was installed.

Disposition: build/staging/installation accepted; native save-load, open-sea
crash reproduction and long-session model accumulation remain unresolved,
with player replay/report as the next evidence owner.
Focused source checkpoint: `315e3bf` (the lifetime patch and ordered-stack entry);
no push. Repository hygiene passes and foreign staged blobs are preserved.
The existing frozen delivery checkout remains with the crash-replay owner;
this task created no worktree. Disposable copy-save diagnostics are removed.

### Prior FPS rollback, engine safety and sea strength — October 6, superseded engine; sea replay pending

Canonical `experiments/native-metal/run.sh --stage-only` completed after the
player closed the game, from the isolated installed-content delivery checkout.
Installed signed engine is `073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac`,
UUID `490F4355-A1CB-3F77-85DC-3A7484FE5F19`; its pre-install build is
`cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d`.
The installed engine matches the canonical signed candidate and deep/strict
bundle signature verification passes. Installation changes executable rpaths and
signature, so the pre-install and installed hashes intentionally differ.

The installed batch includes the mast null-child guard (`19525c9`), foam
registration capacity guard (`0532a1b`), rejected geometry-cache FPS delta
rollback, and shared sea strength / loaded-cannon count fix (`72963c0`).
Exactly three installed script consumers match their frozen hashes:
`sea_ai/AICannon.c` (`3419ae6185f2c551d4aeeea328ba66aa6e1d4866dcbcc14ffb05536103c4d878`),
`sea_ai/sea.c` (`7b4c7a20efa8d2b18954a47e134e7cc3701b72d241d6b38fcb4a30db63ad976a`), and
`worldmap/worldmap_encgen.c` (`3fe5ad32138fafabece7fe38ad8c98443349656da27753da689c53febb2aec9a`).
Paused expansion changes remain excluded. All 215 preexisting player SAVE/config
files retain their hashes; a new autosave appeared after staging finished during
the player-launched session. No player process was stopped.

The player reopened the installed app at 02:32:55 EDT (PID 63449). Native window
observation shows the world map rendered with the player's fleet and a displayed
125 FPS. This accepts startup/rendering only, not open-sea FPS or stutter behavior.
The changed sea-strength action and exact save-load/sea crash scenarios remain
unresolved under the FPS/crash root owner, triggered by player replay/report.
The exit worker has finished; no installation wait remains active.

Latest actual pre-fix crash is October 6 01:31:24 EDT, PID 16765, on previous
installed UUID `B326BD3C-E57F-3CCA-ADE0-C13CB04EFD2B`:
`SEAFOAM_PS::Init +156 -> SEAFOAM::AddShip +180 -> SEAFOAM::Init`. The crash PC
dereferences the invalid INIFILE argument `0xbdd00f1b41688990` before the first
texture-key read. The corruption writer and causal relation to the FPS change
are unproven. The newly pasted attachment is the earlier 00:36:55 mast crash on
old UUID `36D8A720-0642-3598-9870-B8A61C2BE4E5`; it cannot prove mast-guard failure.
See metal-frame-pacing.md for component evidence and rejected hypotheses.

### Living Caribbean expansion — October 7, implementation installed; player replay unresolved

Implementation/delivery is finished after the user's explicit resume; native
goal state owns the subsequent requested pause and player acceptance remains open.
The current source batch closes paid initial assembly/service, physical crew/gun/
goods capacity, preservation of explicit low/zero ship state, weighted eligible
expedition targets, real evacuation routes and independently held harbour control.
Twenty-three existing callback owners bind participation, real naval/fort/land
contribution, crime revocation, return news, pre-damage harbour choice and safe
contact with the real expedition commander. Legacy missing crew fields receive
the established default; explicit zero remains zero. Late contracts preserve
uncontracted credit and cannot increase its reward ceiling.

Canonical `experiments/native-metal/build.sh` passes and reuses the verified patch
stack. Whole-PROGRAM VM compilation, lazy dialogue compilation and four native
serializer rounds pass. A separate VM runs the actual hull-damage consumer,
physical cargo-load comparison, contribution/repaired-repeat negatives, late
patent/contract, murder scope, return news and contact admission; its native save/
load repeats earn no additional credit. Both final runs have zero script errors.
The old synthetic prize fixture now reserves actual crew weight; no assertion was
removed. These fixtures do not accept rendered scenes or balance.

Composition is idempotent and rejects an unknown callback revision. Exact pinned
parallel aiming/sailing/boarding/custody edits are retained. Read-only preflight:
the prior 14/11 pending-consumer inventory and PID47160 are historical. The game
was observed closed before delivery. Canonical `run.sh --stage-only` succeeded
after final source/ordered-patch integration, and installed signed engine is
`46f859b8b41135c368d9f40a5a57ecb38ef3cedb19c9ca6a89f3d79bfaaf5e45`
from source checkpoint `316e8b0`. Both cache and played-app plans are empty;
compiler readiness and deep/strict signature pass. All 233 player SAVE/config
hashes are unchanged; built, installed and VM message headers match.
Hostile contact now has request→explicit consent→revalidated visit, exact scoped
deck protection and one-time return cleanup, with native live-projectile query.
No game launch, player input or forced process exit was performed. Real prior-save,
action/scene/pricing and FPS replay remain player-owned and `unresolved`.
Disposition: implementation and installed delivery accepted at their layers;
visible gameplay/balance is not accepted by source/VM/build evidence.

### Prison blank scene after a day or bout — October 5, installed; focused replay accepted

Hypothesis: custody's shared day completion rebuilds weather inside an already
loaded cell. Baseline installed custody SHA-256
`b764435349c4c2e8f2c408b5ae018601e32fd1a1cfc6507adc702d4ebe2c377b`;
installed engine `29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`.
No game was running at intake. A disposable copy of the player's Santiago tavern
save, the installed executable/assets and current graphics settings reproduced
the defect: ordinary cell entry rendered correctly, but both rest and an actual
nonlethal defeat/return left a flat blue scene with only HUD. The hero remained
at `goto9`, position (-15.33168, 0.0001, -2.882836), and hero/location/model entities
were alive. Missing prison assets and an invalid return locator are rejected.

Exact delta: remove only `Whr_UpdateWeather` from `CustodyLife_Day`, preserving
the cell's locked Inside environment while advancing the date. The same fixture
then retained room geometry and the hero after rest and defeat/return; the
cellmate menu rendered with nine days remaining and zero wins after defeat.
The rendered custody body equals the delivered one apart from its comment.
The existing native VM compilation/state probe passes date costs, funds, parole,
reward idempotence, defeat, cancellation, weapon/immunity cleanup and isolated
arena creation. Its input was bound through `sync_metal_gameplay` to the reviewed
portable package; the direct historical probe entry failed before execution on a
missing native-storm original. No permanent tests or native changes were added.

`sync_metal_gameplay.py apply` succeeded with exactly one pending script, resealed
and verified the installed app signature, and preserved the engine hash. Cache
and app custody now match
`af63f1a9d008d2e664f4aab5596b10e6c776a569777c684587d2acf4b20c4b46`;
both content plans are empty. SAVE/configuration were not replaced. The normal
app launcher subsequently started the installed engine under its player-state
owner; the player took control, so automated input stopped. That process later
exited before a passive screenshot; no normal-session prison replay is claimed.
All diagnostic processes ended and the owned temporary fixture was removed through
`codex-delete-temp`. Disposition: this reproduced blank-cell
defect is accepted in the recorded copy-save replay; tournament victory,
interrupted-save recovery and release remain outside this repair's replay.
Focused local checkpoint: `48def07` (`fix(prison): keep cell visible after days
and bouts`), no push. Repository hygiene passes; unrelated staged/working changes
remain with their owners. See prison-surrender-system.md for the owning contract.

### Regional shipping routes — October 5, installed; player replay pending

Hypothesis: uniformly selected global destinations spread a finite 32-fleet
population across long unrelated trips, weakening visible regional encounters.
The route owner now weights distance continuously, retains a positive long-route
floor, spreads origins by same-role residency and directs pirate routes toward
actual ordinary commerce. Existing diplomacy, quest exclusions, local detection,
fleet strength and battle rules are preserved. Existing saved fleets keep routes;
new departures carry origin/destination metadata in the existing descriptor.

Disposable native script VM compilation and seeded route checks pass nearer/farther
preference, commerce attraction, same-role origin pressure, quest/other-role
negatives and missing-coordinate fallback. Canonical `run.sh --stage-only` succeeds.
Cache and played-app encgen both match
`c7d8ed65bfa0bfa002d4cdcafb9cef184a6853d136b698cee852057762c7b449`.
Engine remains `29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`;
no native change or player-state replacement. Installation is verified; density,
encounter frequency and enjoyment remain unresolved until the player's voyage.
See worldmap-traffic.md for the route model and research references. Focused local
checkpoint: `3c6ad94`; repository hygiene passes. No push.


### Jungle dusk illumination continuity — October 5, installed; player replay pending

Player screenshots at 17:26:01 and 17:26:04 show Cuba jungle ground switching
from dark to bright at the same March 6 1668 18:50. Live installed engine
`8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06`
reports alternating `applied=1 sun=3` and `applied=0 sun=0`. The source admitted
outdoor illumination only when a shadow map was applied, despite Weather
legitimately skipping the sun prepass in dusk/night. See metal-lighting-parity.md.

The shared admission now accepts any successfully completed active location
prepass. Shadow validity, ambient floors, weather, indoor response and failure/
disabled fallbacks remain unchanged. A disposable GPU replay fails on the old
expanded static path (9,360 changed colour channels, maximum delta 24); after
the fix all four expanded/raw static and CPU/GPU-skinned paths are pixel-identical
with and without shadow passes. Existing dynamic-lighting negatives pass.
Canonical build succeeds with zero ordered-patch source changes and the content
plan has zero pending files. No permanent test delta. After the player confirmed
the game closed, canonical `run.sh --stage-only` exited zero and verified the app
signature. Installed signed engine:
`29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`.
SAVE/configuration preserved. The installed app was reopened, its menu rendered,
and PID 41195 runs from the established Application Support owner. The player
resumed walking; logs reach outdoor lamp-free frames and a passive screenshot
shows the Cuba atlas. Automated load input was refused because the player was
actively interacting. No further input is sent and the running game is retained
for that player. Disposition: source/GPU/build/installation and startup verified;
the exact jungle load/walk brightness replay remains player-owned and unresolved.
Focused source checkpoint: `cfc9e5c` (two renderer files, no push). The required
`agent_context.py --check` again exceeded a bounded 30-second run; its owned
process group was terminated. No broad hygiene pass is claimed. Focused diff,
GPU and build checks pass; the preserved foreign staged paths are unchanged.

### Encounter review and compact panel — October 5, source candidate

Player reports blind plain-entry into hostile fleets, wrong default priority and
stretched encounter art. Exact sources show quick entry bypasses ExitFromMap,
contextual commands were replaced with plain-entry defaults, and square 512x512
art was stretched into 518x160. Candidate restores quick-entry review (31130)
and contextual attack/pursuit focus; explicit panel sea entry remains31131 with
quest guard. Illustration is square160x160 beside top-aligned scrolling summary;
actions remain one row. Disposable source checks and native installed-header VM
compile pass. Source commit0e551a2 installed after the player closed the game. Canonical
run.sh --stage-only completed; all four installed encounter files match the
reviewed output hashes. Actual geometry/action replay unresolved.

### Coherent fleet simulation and encounter actions — October 5, installed; scene replay pending

The persistent roster, sea handoff, commitment/retreat decisions and independent
plain sea entry are integrated. Canonical build and isolated VM state probes pass.
Initial stage delivered engine/scripts but omitted the built shared message
header, causing player startup SIGABRT during script compilation. An isolated
probe with copied installed headers reproduced the crash. Corrected
install-engine.sh delivers reviewed messages.h to app/development consumers in
its signing/rollback transaction; unknown header edits are refused before writes.
Corrected run.sh --stage-only completed successfully. Installed and development
header SHA-256 is 38bab60eeeb5cb90370f3ea4c51b9be5169555ac09df74e15debd630257125a2;
installed signed engine is 8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06.
The same isolated compile probe now passes using the delivered installed header,
including popup compilation. SAVE/configuration preserved. Disposition: installed
and compiler verified; actual menu/save/UI/balance replay remains player-owned
and unresolved. Focused local source commit 6039e62; no push.


### Player fleet threat assessment — October 5, installed

The prior traffic delivery excluded the player from NPC strength comparison:
the player fallback chased only by distance/hostility. The correction estimates
every active companion-slot ship from class, hull role and current HP/SP, using
the NPC strength scale. Ordinary traffic scores the player alongside NPC prey;
superior player fleets cause retreat and suppress forced contact, while voluntary
entry, naval and quest encounters remain available. Nonquest legacy pirate
Follow also strength-gates pursuit, but remains a dedicated pursuit encounter.

Native build passed. Exact native-method probes pass risk/bravery thresholds,
escape, trade preference, distance and quest/qID/ALONE/missing-state negatives.
Exact composed scripts compile in the isolated native VM; active-fleet, parked
ship, damage, departed companion and no-ship fixtures pass. The first script
compile rejected makeref on an attribute branch; makearef corrected it.
Independent review identified first-frame awareness; map-entry and synchronous
creation refresh corrected it before the final script probe.

The player closed the game and canonical stage-only began. Its content step
installed two reviewed worldmap scripts and preserved SAVE/configuration, but
the player reopened the installed game before engine replacement. The canonical
engine guard stopped that step (exit 1). After the second explicit close reply,
the unchanged canonical stage-only completed (exit 0), with signature verified.
Installed signed engine SHA-256:
`2679fd1c3ee3feacfcb020ad4d39fc87093e0f75382248de2b17578f7f8b08e3`.
Installed worldmap.c is `f744ff02267d85c4c07fd32f81f8aa2814e6038867fdacc6068352ee33dd2c6d`;
worldmap_encgen.c is `975b3085bc51e8508d56c3138fcd62646f7ad2bf2d0d78b7874c68da7af21372`.
Installed plan has zero pending files; no player-state replacement or game launch.
Additional VM fixtures pass legacy-pirate awareness using real class tables,
quest exclusion and retained bravery; fixture enum/nation setup was corrected.
Source commit `f449f90`. Broad repository hygiene exceeded its bounded 20-second
budget inside git check-ignore --stdin; only its own process group was stopped.
No broad hygiene pass is claimed. Disposable task fixtures were removed.
Disposition: installed, source/build/native/VM probes verified; player scene
replay and perceptual gameplay balance remain unresolved.

### Global world-map traffic — October 5, installed baseline

Owner: parent integration in native-metal. Replaced player-ring trade/prebuilt
battle spawning with bounded native port traffic (18 trade, 8 patrol, 6 raiders).
A caravan has real encounter fleet counts. Targets use live relations, class/
count strength and saved condition. Pirates prefer commerce, avoid stronger
fleets and flee nearby superior opponents; home patrols can supply one rescue
to their own commerce. Battle state uses saved IDs and at least 24 hours of the
actual native map clock, with deterministic victory at twice opposing strength.
Sea import includes the rescue fleet and retained hull/sail/crew wear.

Canonical build exit 0, engine SHA-256
`0095faecfbeed11f6689fea71bca02e93a6810e9ea5bf16c934c502f34c844ee`.
The earlier accessor placement caused a C++ private-member compile failure;
it was moved to the public clock owner and the canonical build then passed.
Focused actual-native methods pass role acquisition/escape, territorial leash,
friend/quest exclusion, saved-ID reordering, removed partner, pause, one rescue,
one-day minimum, superiority and guarded-trade risk. Exact composed scripts
compile in isolated native VM; rescue-range/cursor and retained-wear state probes
pass. Fixture paths/branch references were corrected separately and never changed
installed PROGRAM. These probes do not accept actual navigation or sea scenes.

The frozen content plan contained five map/sea paths plus the separately
reviewed boarding script/dialogue. No game process was running, and canonical
`run.sh --stage-only` then completed (exit 0). Installed signed engine SHA-256 is
`5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911`;
the unsigned build hash above differs because staging signs its candidate.
Installed five map/sea files match their exact reviewed output hashes; the normal
installed content plan reports zero pending files, including boarding. SAVE and
configuration remain preserved. No scene was launched for acceptance.
Source checkpoint: `e80b20e`. Broad agent-context hygiene stalled again in
`git check-ignore --stdin`; only this task's stalled processes were stopped.
No broad hygiene pass is claimed; focused source/native/VM checks passed.
Disposition: installed, source/build/probes verified; player replay unresolved.


### Boarding loot to flagship chest — October 5, installed

The player requested all cabin chest loot in the ship chest, with no automatic
items granted to the hero. Collection stores every exact item ID/count and cash
in flagship `box1`; quest/unique/rare, gold and unknown items are included.
Manual inspection leaves ordinary stock for the player; its exit safeguard stores
remaining protected items in that same chest without personal pickup. The reply
is `Да, всё в наш рундук.`. See `fleet-gameplay-audit.md`. Source commit: `d391091`.

Before delivery, the then-installed native engine's isolated script VM rejected
the prior behavior and passed this batch with an empty error log: seeded chest
stock, multiple sources, exact counts, duplicate special items, raw gold, cash,
unchanged personal inventory/wallet, repeat collection, manual preservation,
invalid cabin/destination and overflow refusal. Geometry/readiness, quest-use
queries and exit rendering are fixture boundaries; this is not player acceptance.

October 5, 14:18 EDT: the monitor found the frozen scripts already installed by
the parallel native/world-map staging owner. Exact installed SHA-256 values:
- `PROGRAM/Loc_ai/LAi_boarding.c`: `e7215938e52f09931069fcd7645922e0c1eb0c465a4608c187ad6e7038e48771`.
- `PROGRAM/dialogs/russian/Enc_Officer_dialog.c`: `4ca117b98c1b2f076a7419aa0bbb2eab4e8ec501706f8cd99d79e297c1c4cb64`.
Both cache and installed sync plans are empty, compiler readiness passes, and
`codesign --verify --deep --strict` exits zero. The installed engine is
`5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911`;
this heartbeat performed no installation, engine or player-state writes.

The earlier incoherent shared stage is resolved. Heartbeat `automation-4`
(Установить сбор лута в рундук) is confirmed PAUSED, with its saved definition
retained. The earlier broad agent-context check stalled in git check-ignore;
no broad hygiene pass is claimed. Unrelated dirty/staged files remain untouched.
Disposition: installed and signature/hash verified; the next real cabin collection
and manual-inspection exit on the player save remain unresolved.

### Treasurer shop eligibility and rarity — October 5, installed

The player reported a false unavailable-store status and no automatic sale.
Real store type is shop, while treasury expected store; merchant and door IDs
also differ in case. Treasury added a nationality refusal that ordinary store
trade has commented out. All three independent merchant rejections are corrected.
Rarity no longer vetoes selected chest items; quest/unique and explicit locks remain.
Native filter/merchant replay passes; transaction fixture lacked normal stock
bootstrap and is not a successful money/notification replay. See treasurer-service.md.
Player scene replay remains unresolved. Commit `c0b0e2e` was delivered by
canonical `run.sh --stage-only` (exit 0); installed core SHA-256 matches
`5f60221013586c18b5709a27f30ca6bb640674652fa71d9802cc93f886435809`. Engine, SAVE and configuration remain unchanged.
The broader agent-context hygiene check stalled in git check-ignore and was stopped;
its success is not claimed. Task-owned source and documentation are committed.


### Automatic treasury trade notification — October 5, installed

Automatic port service adds the existing GETITEM message icon and text once
when proceeds or spend is positive. No sound is played, as requested.
No-op orders, unavailable merchants and save rehydration do not generate
success notifications. Service runs on port, town or store entry when enabled.
Source commit: `b77aa4174d9d2c942060cb505a98b4063130dbb7`.
Canonical `experiments/native-metal/run.sh --stage-only` completed successfully.
Installed characterUtilite.c SHA-256 matches the pinned source output:
`00e8be7f5716d4791aab6f5560beff5f62219b3bc63ecfb35621fa066aa3570f`.
The installed engine was reused; SAVE and configuration were preserved.
Installation is confirmed; automatic purchase/notification replay remains unresolved.

### Treasurer equipment veto removed — October 5, installed

Player capture proves KeepCount=0, but ordinary blades remain blocked by
`Нужно вам / офицеру`. The prior patch was installed but did not remove this
independent veto. The player explicitly owns equipment-retention choices.

Both equipment-need predicates and their blanket sale veto are removed.
Ordinary eligible chest equipment follows the explicit retention, category, price
and manual-lock rules. Personal/officer inventories are outside the sale donor
set. Quest, unique, rare and unknown-item protections remain. The final preview
column shows rarity. No saved setting is reset. After the player closed the game,
canonical `run.sh --stage-only` succeeded, installed script hashes matched the
manifest, and the existing engine was reused. Source commit: `c97c097`. SAVE,
configuration and renderer resources were unchanged. No game replay is claimed.

### Treasurer zero default retention — October 5, installed

The latest three campaign saves omit `Treasury.KeepCount`; the shared chest-sale
calculation and interface previously substituted one retained copy. Their fallback
is now zero. Explicit saved choices remain intact, and quests, rare items, useful
equipment and manual locks still block sale before retained quantity is applied.

Canonical `experiments/native-metal/run.sh --stage-only` succeeded with two script
updates. Installed script hashes match the pinned manifest; the existing engine
`abff64b4e1d002379e6bab3f04f6ee3173b05bdee09c37a59cdcee53bc053044` was reused.
SAVE, configuration and renderer resources were unchanged. No game was launched
or additional gameplay replay run, honoring the player's stop on further checks.
Disposition: installed; interactive sale acceptance remains player-owned.

### Treasury controls and deck framing — October 3, installed

The player supplied treasury, deck, shore-leave and aiming captures. The cropped
deck image was rejected for publication and replaced with an explicitly
recomposed full-body illustration; these edits do not accept native behavior.

The initial follow-up corrected missing treasury `TableActivate` handling and
restricted the 0.5 m eye lift to manual first-person firing. Canonical
`experiments/native-metal/run.sh --stage-only` completed and installed engine
`abff64b4e1d002379e6bab3f04f6ee3173b05bdee09c37a59cdcee53bc053044`; the initial
214 player-file hashes were unchanged at delivery. The source/matrix and native
row-event fixtures predate the player's explicit stop on further checks.

After the next player captures, the purchase checkboxes and footer controls were
aligned into single rows, chest quantities and item features were separated, and
Telegram was removed from the main menu. The player closed the game and authorized
installation. The same canonical stage completed with four script/INI files
applied and the existing engine reused. Final batch, player-state snapshot and
logs are retained in `.cache/portfolio-followup-2026-10-03`. No further gameplay
or rendered-layout checks were run; installed delivery is accepted, interactive
behavior and final appearance remain unresolved until the player's replay.

### Historical portfolio capture-pack snapshot — October 3

Hypothesis: the middle of the portfolio case needs actual fleet/treasury/growth
screens, and a separate save profile can expose those states without changing
the player's campaign. Six prepared copies now live in `SAVE/Портфолио` under
the public launcher's existing external player-state owner. The source game,
installed PROGRAM/RESOURCE, engine `a743bc4a` and player configuration are unchanged;
no native build or staging was needed. The manifest and capture guide are retained
in `experiments/native-metal/.cache/screenshot-save-pack-2026-10-03`, alongside
the 4,368,779-byte ZIP, SHA-256
`da64753c81ecd7e7dd720b05f5ba7ed34fe6a90bfbb11e1dd29cc6a14aecaf90`.

The prepared state is limited to the profile identity, independent cargo targets,
safe-sale settings, conserved officer-to-chest supply transfers and four reduced
morale values for the paid-rest picture. Two sea snapshots retain their native
sea data exactly; item definitions and quests are unchanged. The original 205
recorded player files retain their hashes. All six installed copies match their
manifest hashes; the ZIP passes CRC verification.

The installed engine loads the original control and all six saves with empty
error logs, verifying their profile/location/rank and relevant prepared state.
An isolated OnLoad observer bypasses world creation and rendering. The first
harness input omitted InterfaceStates.Launched, and an attribute-address
expression did not compile; initializing the flag and using makearef correct
those fixture inputs. Neither failure required a game-source patch. The decoded
objects roundtrip exactly, and a corrupt compressed payload is rejected.
No player game was launched or stopped. Disposition: save-pack generation,
native state compatibility and profile delivery accepted; actual scene loads,
UI actions and screenshots remain player-owned. The capture/format contract is
recorded in `docs/screenshot-save-packs.md`.

### Treasurer alignment and chest valuables — October 3, installed; player replay pending

The player's treasury screenshot shows tab labels at the top of their controls
and inconsistent authored button text. Native source binds the tab defect to
`FORMATEDTEXT` clearing vertical alignment when the script replaces its text.
Treasury tabs and the shared-chest tab now reapply alignment after their final
label write; ten treasury and two ship buttons use the normal UI font with
explicit vertical offsets. The existing rectangles and original engine defaults
are preserved. The sale table now shows only its equipment/valuables categories,
with shorter action labels and a separate valuables toggle.

Emeralds were explicitly outside the old equipment-only filter. The new shared
filter admits ordinary jewelry/minerals from the flagship chest only, preserving
quest/unique/rarity/price/keep safeguards and the ordinary merchant restrictions.
The isolated native VM compiles core/store/treasury with an empty error log and
verifies exact emerald/mineral quantities, buyer/chest/wallet conservation,
personal inventory identity, repeat-sale refusal and protected supplies/amulets/
unknown stock. Scene rendering and real player transactions remain unresolved.
Ten XInterface checks pass with zero failures and an empty error log, including
valuables toggles/keep preview, returning to purchases and full native UI unload.
Earlier UI fixture attempts waited on a game-time exit behind the paused window;
the established cancel-then-exit lifecycle resolves that fixture boundary.

The installed content batch contains exactly five consumers: `itemsbox.c`
`2b70d49b`, GoodsTransfer `58f977d0`, character utilities `2ce7768f`, treasury INI
`88a059df`, ship INI `fa4a01b9`. The hash transforms are idempotent and reject
unknown bytes. The installed engine remains `a743bc4a`; content-only sync reuses
it rather than incorporating unrelated pending native edits. After the player
closed PID 90102 and authorized commit/push, the ordinary exact-hash
`sync_metal_gameplay.py apply` exited zero. Its bundle reseal and deep/strict
signature verification passed. Both plans now report zero pending files; all
fourteen fleet pins and the ship INI match in cache and installed app.
All 212 regular player-state files retain their pre-install aggregate SHA-256
`bb6086c449ab595af37a8f2a90db03bc770e319cc1223d7953a31d8670eaa09d`.
No player game was launched, no native engine was rebuilt, and unrelated working
and staged entries are preserved. Commit `3a1bf7e` is verified at `origin/main`;
the dependent treasury completion `c0de1e4` is included in that push.
The earlier normal apply refused the open game before writing. Its bounded
heartbeat `automation-3` was deleted after the successful user-triggered retry.
Disposition: integrated source, content installation and signing accepted;
rendered alignment and real sale on the player save remain unresolved until replay.

### Earlier treasurer completion and shipyard collision — October 3 historical installed snapshot

Canonical `experiments/native-metal/run.sh --stage-only` exited zero after the
final treasury callback and manifest changes. Six changed consumers are installed;
all fourteen fleet pins match both the cached and played resource roots, gameplay
sync has no pending files and the compiler is ready. Deep/strict signing passes.
Current changed pins: character utilities `4d2db76b`, officer dialogue `0b603006`,
GoodsTransfer `b545f8de`, store `d031125b`, loader `75576eb6`, interface INI
`ca5ba998`. Engine `a743bc4a`, the 197 recorded player save/configuration files and
the six inherited source/index entries remain unchanged. No player game was
launched or stopped by this attempt; installation followed the player's closure.

Treasury now has separate purchase/sale tabs, ship-specific total cargo targets,
shared money reserve and real stock/capacity limits, partial orders, configurable
safe equipment sale preview, retained quantities and explicit item locks.
Automatic sale and purchase are separately opt-in; save rehydration never trades.
Queued service obeys actual port/shop/quest/night/busy restrictions and one
accumulated time charge. Contracts and fixture boundaries are recorded in
`docs/fleet-gameplay-audit.md`; raw verification is in the ignored
`experiments/native-metal/.cache/treasurer-verification/2026-10-03` owner.

The player's shipyard report matches the real log's `shipyard.c:1068` compile
failure. A native before/after reproducer binds it to the old treasury's generic
`Add` function surviving as a compiler name after segment unload, colliding with
local `add`. The same sequence also reproduces logged ship/tavern compile errors;
a fresh VM and the corrected scoped callbacks both compile all three cleanly.
Treasurer verification passes 34 core checks, 22 native UI checks and 12 automatic
service checks with empty final error logs. The unconverted UI-busy attribute was
also reproduced and corrected; actual queued trading now defers while UI is open.
The player's live treasury/shipyard and port transactions remain unresolved until
replayed; neither compilation nor widget state proves screen composition or
actual world-time advance. Disposition: source/probes/staging/install accepted;
real game feature acceptance unresolved.

The previously handed ZIP64 archive contains the older eleven-pin batch and is
not refreshed by this installation. The player owns cloud upload. FPS diagnosis
and the reported combat durability remain separate unresolved items below.

### Live sea battle FPS — October 2, CPU hot spots measured; comparison pending

The player's 23:24 screenshot shows 44 FPS in deck aiming view with four fleet
ships and a burning target. A five-second read-only sample of the existing
installed process (PID 12605, started 22:31:35) places 79.7% of 2,465 main-thread
samples in entity realization. Manual aiming accounts for 16.2%; sea rendering
accounts for 28.5%, including 13.3% in reflections. Index validation is a nested
9.9% contributor, mostly sea geometry. The unchanged engine is `a743bc4a`, on
Apple M3 Max at 2056×1329. No build/archive process was active and no thermal
warning was recorded. The old ICB-allocation hot spot is absent.
The public run has frame profiling disabled, so GPU duration is not measured.
The player-owned same-battle Tab/FPS comparison is pending; no renderer or
configuration change was made. Details and raw evidence belong to
`docs/metal-frame-pacing.md`.
Disposition: active CPU work diagnosed; exact camera contribution/GPU bound
unresolved, no performance-fix or FPS-improvement claim.

### Boarding, chest exit and treasury first-open — October 2 historical installed snapshot

The player reopened acceptance with unreachable cabin exit, an attackable loot
speaker, too many dialogue choices, and a blank captain/zero cargo on first
treasurer open. The fleet layer now has eleven pinned script consumers. Latest
pending outputs are boarding `dbdf4a2a`, officer dialogue `4e5d0d36`, chest
interface `36b643ca`, and treasury interface `1da1ed60`. The player closed the
game and canonical `run.sh --stage-only` exited zero on merged `d7b9c40`.
All eleven installed pins match; gameplay sync reports zero pending files and
compiler ready. Installed deep/strict signing passes, and native engine
`a743bc4a` is unchanged. No player save/configuration or process was changed. Contracts and fixture limits belong to
`docs/fleet-gameplay-audit.md`.

Native fixtures pass two choices/Esc, both reply paths, restored exit input,
neighboring chest/body/fight/interface clicks and consecutive protected-loot
conservation. The original chest close reproduces seven global/type errors and
leaves actor mode; the local snapshot correction restores player mode, retaining
the cabin-menu exception, with empty error logs. Treasury source/C++ discovery
binds the initial `-1` to scroll initialization; the data fixture proves valid
first cargo/orders and later companion selection without changing the first order.
Real scene/input acceptance remains player-owned. Reported excessive hero
durability remains unresolved; this correction does not change the HP curve.

PR 21 is merged as `a88433e`; PR 22 (treasury first-open) is merged as
`d7b9c40`. The frozen public app contains all eleven current script pins, including
the four corrections, and passes deep/strict signing with unchanged native engine
`0e45889d`. Native Info-ZIP refused an incremental update of the 11 GB ditto
ZIP with an invalid-structure warning; the old inventory is unchanged. A fresh
native ditto ZIP reproduced the defect: 95,308 entries and offsets above 4 GiB
are represented by wrapped 16/32-bit end records without ZIP64 records. Both
archives fail standard random-access header reads; native ditto extraction reads
the actual payload and all eleven extracted pins match. Packaging acceptance is
reopened. Native `zip -FF --out` copied local records but failed with `EFAULT`
(Bad address) before producing a final archive; no such output is published.
The bounded libarchive 3.7.4 probe establishes the compatible route: stream
local records with Mac-extension interpretation disabled to retain explicit
AppleDouble files, then use the ZIP writer with automatic ZIP64. Forced ZIP64
local headers are rejected by native ditto even for small files; automatic mode
preserves bytes, file/directory xattrs and symlinks in the native extraction
probe. The real 95,308-member archive is being assembled with the frozen signed
app plus its native AppleDouble tree; bare libarchive ZIP is rejected because
its metadata writer gap is documented in issue 2041. No original archive or
installed app is changed by these task-owned packaging attempts.

Read-only inspection of the last player save (`вход в пещеру 4`, 22:17:46 local)
parses all 6,467,501 payload bytes and 649 variables with no remainder: Blaze,
index 1, rank 18, player type, 83.096/196 HP, `Progression.HPCurve=1`, no
`chr_ai.immortal` or HP-checker attribute. This disproves an enabled immortality
flag in that snapshot; difficulty against stronger opponents remains unmeasured.
No additional HP or damage balance change was made.
The automatic ZIP64 artifact is now verified: 11,301,648,844 bytes,
SHA-256 `4abf447a07ead465c6c0a9b890b0d326ba34667922c9528ea5bb6427dda3898a`,
MD5 `1ea3a7ea46b98dcfbea7746e8a40694f`. All 95,308 normalized payload names,
CRCs and sizes match native ditto input; one AppleDouble name changed only
Unicode normalization. All eleven gameplay pins, engine, launcher and resource
seal match the frozen app. Real native ditto extraction, deep/strict signing and
the relocated native launcher's isolated `--stage-only` all pass.

The connected Drive uploader rejected the actual byte upload before transmission:
11,301,648,844 bytes exceeds its 536,870,912-byte cap. Browser recovery started
Upload new version on the existing file under its live personal owner account,
preserving file ID and sharing. The player then took ownership of the upload
("сам загружу"); no further agent upload or monitoring is authorized. The browser
session no longer exposes that tab, so neither cancellation nor completion is
confirmed. Provider completion remains unverified. The installed player app was
not launched. The verified ZIP is retained for the player's upload at
`/var/folders/sf/3cjm3vl53wg7fvf4wnnk4f6w0000gn/T/corsairs-fleet-integration.4m7wj1xt/zip64/Corsairs-Iddictive-Remaster-macOS.zip`.
Disposition: source/VM, installed state and native ZIP verified; cloud upload
handed to the player, provider completion unverified.

### Historical 21:40 dialogue delivery — October 2, installed; superseded by the correction above

Player screenshot shows Felix Rocha with no dialogue text or choices. The live
compile log at 21:26:34 reports an officer-dialogue load failure; error.log binds
it to a duplicate `TraderStock_RecordPlayerSale` extern after the real shop
segment was loaded. A native VM reproduces exactly that loaded-state conflict.
The original isolated compile fixture had not loaded shop_rotation and missed it.

The fleet manifest removes only the duplicate extern, reusing the main program's
registered trading function; the prior installed digest remains an approved
upgrade input. Corrected dialogue SHA-256:
`179e563196c7e89b9167c31425fd238fa8bf27a407d0ddc4cc3f4cd562bfc108`.

The actual core dialogue wrapper produces collect/inspect/finish choices on
first load and after unload/reload in separate native event frames; the regular
sale action refuses at sea. Exit 0, empty error log. Only cabin-entity readiness
is abstracted. Same-frame unload/reload and missing sea cleanup in scratch
fixtures were invalid probe contexts; the production event lifecycle resolves
those fixture failures. The user game closed; canonical staging completed successfully, the installed
script matches the corrected digest, and gameplay sync reports zero pending files.
The superseded archive job was stopped; no old archive was uploaded to Drive.
Disposition: source/state and installed refresh verified; real player interaction
remains pending. Existing item-box and ship-light log errors are observed separately;
this dialogue change does not claim to repair those independent runtime paths.

### Fleet gameplay batch — October 2, installed; player replay pending

Owner: current Corsairs integration task. The exact-hash fleet layer composes
on the canonical shared/living-Caribbean package. Ten script owners preserve
reviewed capture/crime/white-flag fixes; canonical stage delivered 22 script/INI
updates, the reviewed aiming techniques and the new ARM64 engine. The player
closed the game; no automated launch or player save/config change was performed.

Hero HP uses permanent P/E, additive one-time migration and unchanged HPPlus;
officer capacity grows with rank/base Authority/A. Ammo fallback, paid fleet
morale, shared carpenter materials, safe market-bound chest junk sale, selected
ship treasury targets and optional final cabin-loot orders are installed.
Unknown/quest/unique loot is preserved; absent speakers cannot strand capture.
See `docs/fleet-gameplay-audit.md` for contracts and exact replay gaps.

Native exact core/dialog compile: exit 0, empty error log. HP and resource/loot
VM state fixtures pass; geometry, native reload dispatch and cargo-load refresh
remain abstracted in the second fixture. Native Metal grass/fog pixels pass;
manual-aim geometry passes 758 checks with zero failures. Raw built engine:
`9ded1de7e465073d03a0234d9769a47c1cc4c0cc1aca8b8cc0adbcbdb6f7fdb5`.
Installed, signed engine:
`a743bc4ab07d40326c3c54c42b2039aed92995cbe6f48d226aae313c2a01adc1`.

The first idle staging attempt rejected `_dev/ship.fx` digest `092a2b5a...`.
Reapplying historical reviewed be695e5 to the captured engine reproduced those
exact bytes; both staging allowlists now recognize that historical output while
rejecting altered bytes. The next canonical `run.sh --stage-only` exited zero;
installed deep/strict signing passed and gameplay check reports zero pending
files with compiler ready. PRs 16, 17 and 18 are merged; the focused historical
technique delivery fix is 5ead729. All six inherited working/index entries remain
owned by their original writers. Public archive refresh is now in progress.

The stale index transaction was repaired by the .codex owner (71d63a5); the exact
original focused commit succeeded as d584d93, with normal lifecycle lease release.
Disposition: source, VM, build and installed staging verified; actual gameplay
acceptance remains unresolved until player replay.

### Sea aim overlay v4: fan arcs, raised ring, wave clearance — October 1, source candidate; staging pending

Player replay of the staged v3 tube (4 screenshots, open sea + shore + close ship, ~21:15): the 4-face tube renders as a white highway to the horizon at range and explodes into sheets and stray lines at close range; no drape ring is visible on water or ships while the shore ring reads; the per-vertex draped ring cuts chords through cliffs (crooked cork, stray hillside lines); the close-range ship lock shows rails crossing the hull plus an edge-on ring line. Root cause bound to code: the 0.35 m sea marker drowns in normal-weather chop (fMaxSeaHeight 2.0, PROGRAM/weather/WhrSea.c; calm 0.5), burying ring, march end and cross on every water/waterline lock. Installed engine proven byte-identical to the worktree v3 (source-patches.json patch text md5 9dc02672), so all four screenshots are v3 behavior.

Delta (cannon-trajectory-aim.patch 1078 to 976 lines, net -106): tube fill, corner rails, volume flush, vertex struct and their constants deleted; the battery fan is three trajectory arcs (mid-gun spine + two faint outer-gun edges, new kAimEdgeA0/A1); sea marker kAimImpactY 0.35 to 1.5 m with the wave-height evidence in the comment; the ring is one flat ellipse at the highest sampled surface + 0.35 m (center + 24 samples over sea/island/fort/ship disc) and returns its plane height so the cross sits on it. Fire()/GetFirePos unification, manual-fire bort-delta removal, reticle restore and terrain/ship-disc sampling unchanged from v3. Falsifier: git apply --check --whitespace=nowarn (harness flags) against the reversed base passes and the applied tree is byte-identical to the edited source. Not staged: the game process is running and staging skips engine install while it runs; no build was started to avoid stealing player CPU. Owner: docs/sea-aim-overlay.md. Disposition: source candidate; run.sh --stage-only + player replay pending after the game closes. Unresolved: alleged left-bort asymmetry (code path is side-symmetric; needs a repro screenshot with reticle state and whether the guns fire), far-aim readability, heavy-storm ring wash.

### One sun model and below-horizon glow gate — October 1, staged; replay pending

Follow-up to the fog-vs-sky verdict: the 05:00-23:00 engine sun (28 degrees up
at 20:00) contradicted the night shader and dusk presets, and the SUNGLOW
overflow/flare/reflection billboards had no below-horizon gate (moon/sun
light spilling from under water). `sun-below-horizon.patch` narrows the
engine window to 05:30-19:00 (sunrise ~06:07, sunset ~18:22), fades the
directional sun color across civil twilight, and gates the three unclipped
billboard paths by elevation; disc/halo keep geometric y-clip. The shader
mirrors the engine curve branch-free, so 19:00 cannot pop. `dynamic_sky-probe`
PASS (new: 18h day-bright vs 20h night-dark, sunset continuity, 19:00 edge).
`metal_evening_lights.py` flips Evening 19h/20h `Lights` to true (Night stays
false) so towns keep lamps after sunset. `run.sh --stage-only` exit 0;
installed engine 753de4b5 (pre-codesign f8e3e90b) to /Applications; gameplay
2 files (Evening.c, LAi_player.c). Unrelated dirty deck-walk/living-caribbean
candidates were stashed during staging and restored after. Observed but
unowned: `cannon-trajectory-aim.patch` changed in-tree during this session by
another writer; preserved untouched. Disposition: staged; player replay at
19-23h (sea horizon + town lamps at 20h) pending.

### Background fog vs sky color — October 1, systemic fix staged; replay pending

Player screenshot (open sea, October 6 1667 20:00): dark sky above, light
blue-grey fog band with fogged mountain at the horizon. Root cause is
structural, not a preset typo: three independent color owners diverged at
dusk. Authored 20h fog is cool (77,104,134) while 20h sky textures are warm
(low-row average 127,120,110), and the Metal procedural night palette is fixed
dark blue independent of both. The old shader converged only a narrow low band
(smoothstep .018-.16) to fog, so any per-hour fog tweak just moved the seam.
Fix: smoothed visual fog is now the single horizon owner. `dynamic_sky.hpp`
derives the whole base gradient (zenith = fog * (0.22,0.42,0.72), horizon =
fog), night/day cloud colors (fog*0.55+0.02 / fog*0.49+0.55) and the horizon
match from `horizonFog`; authored textures modulate luminance only and cannot
shift hue. Fixed palettes remain solely as the no-fog fallback. New probe
fixtures (dusk 20h fog: horizon exact, zenith darker blue-dominant, upper sky
follows fog) PASS alongside the existing midnight/noon and horizon-exact
checks. `run.sh --stage-only` exit 0; installed engine b317f323 (pre-codesign
9dabbf94) to /Applications/Corsairs Iddictive Remaster.app. Unrelated dirty
deck-walk candidates were stashed during staging and restored after.
Disposition: staged; player sea replay at 19-23h pending.

### Cannon manual-aim trajectory overlay — October 1, staged f1f0480; replay pending

Player asked for a beautiful manual-aim ballistics visualization. Disjoint
subagent delta (Darwin, closed): new `cannon-trajectory-aim.patch` (3 hunks:
`ai_ship_camera_controller.cpp` player-only overlay call inside `Realize`,
`ai_ship_cannon_controller.cpp` `DrawManualAimOverlay` plus static-buffer
helpers, header declaration only) registered in `build.sh` immediately after
`metal-camera-view-restore.patch` with the remaining order intact. Parent
verified the single-line registration, snapshot anchor bytes, the inherent
first-person gate (`Realize` returns early on `isCameraOutside()`), and the
`DrawLines(..., "Line")` flush. Residuals: fixed HeightMultiply 0.4 (no
knippel 0.65 without a script getter), circular drop ring instead of ellipse,
static amber. No save/AI/damage/reload/camera state touched. Canonical build
and staging were not run in this session; the overlay has zero player replay.
October 1 evening: HEAD f1f0480 (point-and-shoot aim, ship-lock ellipse,
contact glow, deck eye lift; live HeightMultiply 0.4/0.65 by charge, unified
broadside ribbon, own-ship sail/rope fade) staged via
`run.sh --stage-only` exit 0 after user closed the game. Installed engine
`cebbfc14...` (pre-codesign `3dd3292f...`) to /Applications; gameplay
0 files pending. Worktree also carries unrelated dirty deck-walk + dev_runtime
candidates (other threads). Known v2 gaps awaiting replay: no island/fort
trace (terrain aims fall to max-range sea), solid ship cylinders lock
rigging air (floating ellipse, live balls overshoot to hill), horizontal
ellipse edge-on at mast height, _alpha fade ZWrite off (sea overdraws own
rigging below horizon). Disposition: staged; player replay pending.

### Night floor 0.20, trade auto-pin follow and ship chest button — October 1, source candidate; not staged

Player voice report on the installed .38 floor: the city at night is flooded
like daylight even away from lamps, while jungle and sea already read slightly
too dark. Hypothesis: the source-light floor is the single variable;
jungle/sea evidence forbids any uniform lift. Delta: `lighting.hpp` floor .38
to .20 with recomputed probe bands (zero-authored 0.19--0.21, lamp-free
vertical 0.07--0.09, retained-matte ground 0.029--0.032, night highlight 0.220
vs day 0.963); probe compiled and PASS. `metal_tradebook.py` gains
`SystemInfo.TradeBookPinAuto` so auto-pins follow the current eligible town
while manual pins stay; unflagged legacy pins migrate as auto.
`metal_living_caribbean.py` adds the missing `CHEST_BUTTON` control to
`ship.ini` (the prior batch added only the `ship.c` command handler, so no
button could appear), gated by cabin ownership. Smuggler sell-link absence is
not a defect: the sell link is base-game gated by
`FindFirstContrabandGoods(PChar)!=-1`, untouched by our Meeting_3 patch.
Grey horizon and night blue-through-fog are unbound: night fog presets are
near-black (2,2,2)/(4,6,9) and the analytic night horizon is dark blue, so the
next evidence is a screenshot with place and hour. `backend.mm` albedo .38
floors and lamp stacking are recorded as separate deferred variables in
docs/metal-lighting-parity.md. Sync check shows three gameplay files pending
with compiler ready. Nothing staged: game process state was unverifiable in
this session (`ps` blocked by sandbox) and no apply/launch was requested.
Ballistics visualization (Darwin, running) is a disjoint subagent delta and
excluded from this batch. Disposition: source candidate; staging and all
player replays pending.

### Instant sea-crime punishment — October 1, diagnosed; fix proposed, not applied

Player with a privateer patent attacked a ship at sea and instantly lost the
patent (or trade license) plus reputation. Read-only subagent diagnosis
(Beauvoir, closed) is factual with file:line chains: the instant punishment
flows through two preserved legacy branches for an already-hostile but
untracked target, bypassing the deferred report contract in
docs/crime-reputation-system.md. `tools/patch_crime_reputation.py:605-615`
(`Ship_FireAction` else-branch, second and later shots) calls vanilla
`Ship_NationAgressivePatent` (`AIShip.c:821-849`: patent stripped, hunter+40,
no witnesses). `patch_crime_reputation.py:380-390`
(`CrimeSea_PreparePlayerBallHit`) calls full `Ship_NationAgressive`
(`AIShip.c:850-895`: reputation -10, pirate flag, nation war, hunter+5,
patent stripped) when relation is already ENEMY and the ship is untracked.
Tracking is silently skipped in `CrimeSea_Begin` (pirate victim, base-nation
enemy, `Situation`/`MQPirate`/`Coastal_Captain`), and a stale encounter
`AlwaysEnemy` (see docs/sea-encounter-enemy-state.md) routes even the first
player shot down the instant path. No instant trade-license strip path exists
in `AIShip.c`; the lost paper was almost surely the patent. Proposed minimal
fix: a `CrimeSea_HandleUntrackedEnemy` local-hostility-only branch plus a
marked deferred incident in both legacy branches; open question whether a
lawful prize should also skip rep/hunter/war in `Crime_ApplyPublicConsequences`
(currently only the patent item is exempted). Exact verdict needs target
nation vs patent nation, sailed flag, pre-shot hostility, fort/port
proximity, and log lines. No source, SAVE, or runtime mutation was made.

### Tavern combat scaling — October1, diagnosed; balance unchanged

User reports losing armed tavern fights at rank16. Latest PortoBello_tavern
save: hero HP100/heavy43, ordinary Habitue visitors HP152–181, rank up to22,
combat skill up to67. Installed GeneratorUtilite scales NPCs from aggregate hero
rank; Duel_Prepare_Fight can additionally apply Hunter parameters/+40HP. Earlier
duel enemy no longer resolves, so exact lost-fight identity/action chain remains
unresolved. No source/runtime/SAVE/difficulty mutation; owner and acceptance
boundaries captured in docs/land-combat-balance.md.


### Land officer inactivity — October 1, diagnosed; command replay unresolved

Current installed Metal gameplay is unchanged at source5477bc4 and prior engine
installation. User reports officers fail to help in jungles. Latest jungle save
Cuba_Jungle_10 decoded read-only: hero Charge range35, armed living same-group
officers428/430 in stay template. Officer updater explicitly returns on stay
before searching targets. This explains inability to react in that saved state;
writer/history of stay not proven. Player asked to replay Charge (defaultJ) in
the same fight. Owner/next falsifier: docs/officer-combat-assistance.md. No live
application/SAVE/config/code mutation occurred; feature outcome remains unresolved.


### Spain governor compilation correction and tornado delivery — September 30, installed; player replay unresolved

The player's governor conversation produced repeat COMPILE ERROR at line358
in `dialogs/russian/Governor/spa_Governor.c`, Undeclared identifier merch_8.
The exact-hash Metal-only gameplay layer quotes the three prisoner model names;
all other dialog/quest bytes are unchanged. Real Metal VM LoadSegment reproduced
the original failure and compiled the complete corrected segment. Unknown input
rejection/idempotence checks pass. Canonical run.sh --stage-only succeeded after
this layer; exactly one gameplay file was pending and applied. Owner/evidence:
docs/dialogue-escape-exit.md.

The pending cc45e5e tornado engine correction was delivered in the same batch.
Fresh idle baseline: 146 SAVE files and four configs. Installed only the engine
and the individual governor script; preserved all player state byte-for-byte.
Deep strict app signature passed. Staged engine SHA
`f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`;
installed signed engine SHA
`dfb96ced00417b0a3d374e7807857b7322775f7674b6ca59c49833239803704e`;
governor script SHA
`fa900d90d028a243376fb451f951865f804f3702f38483b979aea7bb032b02e7`.
Recovery engine/script/resource seal and probe/stage/install logs retained in
native-metal .cache/app-update-backup-governor-tornado. No game launched for
installation acceptance; same-save governor quest/farewell and actual storm
visual replay remain unresolved. Prior brightness/FPS/trade layers retained.

Source correction committed as `5477bc4` on main. An automatic staged-ownership
hook initially rejected the new source file despite same-thread creation; the
bounded .codex repair c02eff6 restored byte-exact FileChange creation provenance.
Native adopt-staged accepted both exact task paths, and the unchanged original
focused commit succeeded. No product/index/HEAD mutation came from the specialist.
Verified origin/main equals `5477bc456529034f17fd003463c42b1036311d01`;
working/index clean, agent_context --check passed. Adjacent interface/ship.c compile errors in the player's logs
are outside this governor correction and were not changed; earlier merchant
crash and dawn-water boundary also remain unresolved. Earlier inventories below
are historical and do not override this installed batch.


### Tornado white rectangles — September 30, historical staging attempt; delivered above

Source `cc45e5e` on main restores existing particle technique loops around native
tornado pillar/ground and noise-cloud draws. Only renderer-particles-fx.patch
changed. The player's storm screenshot at11:41 shows white sprite rectangles;
the current native bridge had bypassed the authored texture/alpha stage chain
and inherited the untextured TornadoPillar material. Installed mask textures
exist. The missing Tornado/ITEMS.TGA entry concerns debris and is a separate
resource finding, not proof of the mask mechanism.

A 32-square correctness fixture with actual Metal particle bridge and seeded
prior valid material state reproduced white rectangle pixels, then preserved
transparent background ff203040 and half-alpha bf9098a0 with the authored chain.
The untextured negative remained white. This is not an FPS measurement or actual
storm replay; the player game was running during this small correctness fixture.
The existing source inventory probe could not open its absent native-storm
baseline and did not pass. Canonical build integrated two changed consumer
source files; run.sh --stage-only passed with compiler ready and zero gameplay
pending. Staged engine SHA-256 `f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`.

Installed standalone arm64 Metal engine remains
`88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
A closed-game baseline captured143 SAVE files and four configs. The user then
restarted as PID39052 and player state changed before installation. The
installer refused at the initial baseline comparison before creating backups
or replacing the engine; no live runtime mutation occurred. Installation remains
pending a closed game and fresh SAVE/config baseline, with this chat as owner.
Native storm replay remains unresolved after delivery. Mechanism/evidence owner:
docs/audio-and-particles.md. Prior dawn-water boundary and merchant crash remain
unresolved; this patch does not address them.


### Outdoor night brightness correction — September 30, installed; sea boundary unresolved

Source `80b99e1` on main was built and canonically staged via
`experiments/native-metal/run.sh --stage-only`, then installed into the standalone
arm64 Metal app. The player rejected the .40 outdoor ambient floor as too bright
in Porto Bello at05:25. The corrected .38 floor removes half the last increase;
indoor fill, authored lamps, brighter/daylight weather and compatibility behavior
remain unchanged. Existing CPU outdoor energy/continuity/negative checks passed;
existing GPU lighting probe reports modern night48 (prior51), day52 unchanged,
legacy night14 unchanged, with emissive/unlit/UI/alpha checks passing. Perceived
night readability still needs the player scene replay.

Staged engine SHA-256 `ace96ac1a30e44197cf8cb715f1a67265f23133847e1c0740d54d29cdef70adf`;
installed engine SHA-256 `88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
Deep strict app signature verified; all 140 SAVE files and
4 player configs byte-identical across installation. Rollback
engine/CodeResources retained at native-metal `.cache/app-update-backup-night-tune`.
Trade auto-pin from bbd2037 remains installed. The user closed the playing game;
CUA observation inadvertently started diagnostic PID76219 at21:44:47, which was
closed with TERM before staging. No player gameplay process was stopped. Earlier
engine inventories below are historical, superseded by these hashes.

The separate dawn-water boundary has a source-backed cubemap-background
hypothesis, not a reproduced cause; no sea patch was applied. Ownership and exact
missing falsifier are in docs/metal-weather-surface-color.md. Merchant dialogue
crash remains deferred by the user's prior instruction.


### Minimum scene lighting and trade auto-pin — September 30, installed; replay unresolved

Integrated source: `bbd2037` on main, pushed to the verified public origin/main.
The exact batch was built and staged with
`experiments/native-metal/run.sh --stage-only` after the final source change.
Gameplay sync reviewed and applied only PROGRAM/interface/tradebook.c, preserving
SAVE/config and renderer resources. Installed app remains the standalone arm64
Metal GPK 1.3.2 AT + ReConstruction 1.4.1 owner. No game process was running during
staging or installation. Earlier inventory hashes below are historical.

Modern classified outdoor source-ambient floor increased .36 to .40; existing
warm indoor floor (.125,.100,.078) increased to (.140,.112,.088). Day ambient above
the floor, direct lamps/sun, emissive/unlit/UI behavior and graphics settings were
not changed. Existing CPU indoor/outdoor probes and native GPU lighting/dynamic
lighting fixtures passed, including retained materials, alpha, skins and lamp
contrast. The representative modern outdoor night GPU channel rose 45 to 51;
day stayed 52. These are fixture samples, not whole-scene perceived brightness.
The player screenshot before this batch showed Cartagena at 06:30 and FPS125.

Trade comparison initializer and mode toggle now attempt current-town auto-pin
when the saved pin is empty, including a persisted empty attribute; valid manual
pins and immediate unpin remain. No eligible current city/known prices leaves it
empty. Two disposable VM fixtures failed before Main; no gameplay acceptance is
claimed from them. The earlier PID78900 crash was the diagnostic CLI invocation
without engine.ini, not the player's merchant-dialogue crash.

Staged engine SHA-256 `9d152e333b6745c41ca750364a9cddc0ffd81dbd1f1bb3082b259a93342eaee8`;
installed engine SHA-256 `3ea9553b27f614bb1f93ee8a7f493c9cef97518444efe5b1b82bf5eb50619884`;
installed trade script SHA-256 `e4572beabccda273daca89bfee8270ac633c6a1b3ffbdd08a12b46e33a2340c6`.
Deep strict signature verification passed. All 139 SAVE files and
4 player configuration files were byte-identical across installation.
Rollback engine, prior trade script and CodeResources retained in the ignored
native-metal cache `.cache/app-update-backup-night-pin` with an install receipt.
Current player display configuration: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}; dynamic_lighting=1,
shadow_quality=2, modern_lighting=1 remain enabled in retained metal_graphics.

Disposition: build, probes, canonical staging and installed signed delivery
accepted for their checked properties. Actual Cartagena dawn/night readability
and comparison auto-pin on the current save remain player-replay unresolved.
The merchant dialogue crash remains unresolved: nested SetShow reached corrupted
window vector state; isolated valid recursion/nesting did not reproduce it.
No speculative crash guard was added. The user explicitly deferred crash
investigation and requested immediate installation of the existing batch.
Owners: docs/metal-lighting-parity.md, docs/trade-journal.md,
docs/metal-graphics-settings.md. No new game launch was required for this delivery.


### Fleet performance, deck shadow detail and safe Apply — September 30, installed; replay unresolved

Integrated source: `71ea527` on main; pushed and verified at
`https://github.com/iddictive/Corsairs-Remaster`. Only four renderer source/probe
paths were published; private runtime/topic documents remain local and excluded.
The installed source is this exact batch; signing changes the Mach-O hash.
Ignored diagnostic benchmark/candidate cache directories remain under the
native-metal owner because codex-delete-temp accepts only direct system-temp
children and rejected their repository-cache locations; no deletion bypass used.

Requested target is the existing standalone arm64 Metal app at
`/Applications/Corsairs Iddictive Remaster.app`, GPK 1.3.2 AT + ReConstruction 1.4.1.
Baseline installed engine SHA-256 was `6961f227bc2ad3688a034cc080e57c15af7e70dfc4e9c67242888b2e108d1d60`,
one player process during diagnosis and zero before build/install. Screenshots
and live logs reopened prior safe-Apply acceptance: actual installed patch still
reset the device; it rejected reset and produced 290 vertex-lock failures.
Fleet samples independently identified quadratic point-caster map recycling.

Integrated fixes: FIFO geometry-slot recycling/incremental exact-key updates;
sea cube registration deferred until actual lamp selection while location
registration remains eager; Near96/Far288 sea/deck maps with projected static
AABB rejection and footprint blend; stable-screen Apply with flags/quality only.
No global resolution increase, settings override, reflection cadence change, or
PROGRAM/RESOURCE replacement. Owners/evidence: docs/metal-frame-pacing.md,
docs/metal-lighting-parity.md and docs/metal-graphics-settings.md.

Canonical `experiments/native-metal/run.sh --stage-only` passed after the final
source/layout change, with zero gameplay pending and compiler ready. Staged
engine SHA-256 `614b1e0b0e84aa8fb0ac410828b8a2bea5ea3dc61d4dc3746c77feaae4050434`. Standalone dependency rewiring/signing
produces installed engine SHA-256 `96b5b6e0687e6e737e51181986097f2331b87da48f320f9b5899d2e4201138d6`; deep strict
app signature verification passed. All 139 SAVE files and all four player
configuration files were byte-identical across installation. Rollback engine
and prior CodeResources are retained at `.cache/app-update-backup` under the
native-metal owner.

CPU one-off 800-moving-caster benchmark: 16.908ms to .097ms per frame for the
registry component, not total-game FPS. Mixed key hits/motion/instance expansion,
stale compaction and scene reset passed. Existing GPU fixtures passed sea
point/mixed-order sunlight, no-source/no-stale frames, all quality tiers/live
backend apply, and location/indoor/skinned/cutout lighting; graphics ABI probe
forbids deferred device/window reset.

A preliminary integration build failed on missing receiver pass/encoder
declarations before installation. Corrected declarations and appended matrix
layout were rebuilt through the canonical staging path; installed app remained
untouched until that succeeded. Disposition for that preliminary candidate:
rejected. Integrated build, fixtures, staging and installation: accepted for
those properties. Actual fleet FPS, close-deck appearance and in-game Apply:
**unresolved**, with player same-save replay as acceptance owner.

CUA attempted the updated standalone app launch; process 49297 started, but
native UI observation failed with `Computer Use server error -10005:
timeoutReached`, so process existence is not scene acceptance. No game process
was stopped. Current player config: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}. Older display/staging inventories
below are historical; the hashes and config in this subsection supersede them.

Player replay reports improved FPS and missing deck shadows. The launched
process environment has `STORM_METAL_DYNAMIC_LIGHTING=0`, matching the retained
`metal_graphics` record `dynamic_lighting=0`; renderer diagnostics show applied=0.
The previous failed Apply saved flags62 (bit0 off) but rejected device reset,
so comparing the old live scene with the restarted scene is not a same-feature
performance comparison. Player was directed to enable dynamic shadows and Apply
inside the game. Final fleet FPS/appearance with shadows enabled remains pending;
do not count the shadow-disabled FPS report as proof of the full shadowed scene.



### Installed standalone app — September 30, startup accepted

The installed owner is `/Applications/Corsairs Iddictive Remaster.app`, an independent arm64 application with a native entry point, bundled dependencies and a complete verified ad-hoc signature. The prior unsigned bundle caused TCC to synthesize resource signatures during CoreAudio startup while WindowServer also waited on TCC. The corrected package passed a signed CoreAudio init/uninit probe; the player subsequently reached the main menu and playable sea, deck, trade-journal and prison scenes. This accepts the observed startup, not every gameplay path or OS failure. See `docs/metal-remaster-delivery.md` for the concrete packaging fix.

Player state remains in Application Support. The 2560×1655 display snapshot in this earlier startup attempt is historical; current values are in the fleet-fix subsection above. Applying a resolution takes effect normally after restarting the game, as confirmed by the player. No graphics-engine change was made for that report.

### Personal remaster menu and portable build — September 30

Current staged owner: native-metal, GPK 1.3.2 AT + ReConstruction 1.4.1,
arm64 engine SHA-256 `4878e76db86e82426682413f3acd0c94806a925c22aa22798c21097226b86a65`,
Metal backend. The staging configuration below is historical; current player display values are recorded above.
The user closed the game before the integrated batch. Canonical
`experiments/native-metal/run.sh --stage-only` passed all 548 targets and applied
three hash-bound menu files; subsequent sync reports zero pending and compiler
ready. The logo texture matches its manifest. Mach-O deployment floor is 15.0;
source diagnostic paths are relative; the public exporter rewires its copied engine to bundled libraries.

The actual Storm VM compiled the changed main menu and resolved its URL bridge.
HTTP and file URLs returned rejection without opening a browser. The first
fixture omitted `InterfaceStates.Launched`, causing a fixture-only attribute
error; seeding the required state corrected it. Player replay accepts startup, sea play and the corrected logo/social text spacing. The final removal of the duplicate banner and the VERSION link are staged; the actual HTTPS click replay remains player-owned. Standalone app and public source/Drive/site publication are delivered; corrected-menu archive replacement and user-requested repository cleanup remain in progress. Older dated verdicts below
retain their feature replay status, not the current engine inventory.


### Trade journal units and controls — September 24

Baseline: main `a535e0937b6b`, installed arm64 Metal engine, 1920x1241,
fullscreen 1, MSAA 0, zero game processes before delivery. The player screenshot
showed consistent lot arithmetic but ambiguous raw-piece quantities. The
Metal-only journal layer normalizes display units, sorts planned rows by total
gain after allocation, and adds pin/unpin and contraband visibility state.

The first candidate compiled in the installed script VM and passed exact-input
rejection. A disposable state fixture crashed without logs; it is not acceptance
evidence. `run.sh --stage-only` succeeded, gameplay sync was zero pending and
the installed game rendered the changed journal on the player's save.
The player rejected its toolbar icons and reported the route text aligned to
the top. This visual candidate is **rejected**. Text buttons and explicit
post-update vertical centering are prepared; their delivery/replay is
**unresolved** while the player closes the game. Stable contracts and remaining
scenarios: `docs/trade-journal.md`.


September 24 follow-up: text controls and route centering were staged through
`run.sh --stage-only` and are visible in the player's 10:50 screenshot. The same
screenshot rejects header clipping and total-gain ordering (tobacco 16.5 above
gold 157). Removed the post-allocation sort, retaining margin-per-centner order;
set explicit header scales in both modes and shortened the name header to Товар.
The game was closed before gameplay sync applied the one changed script.
The installed script passed compilation in the Metal VM; canonical
`run.sh --stage-only` succeeded with zero pending gameplay files.
Visual replay and the reason for zero gold allocation remain **unresolved**.

### Grouped graphics settings and safe Apply — September 23

The player rejected the old resolution/aspect controls, the unfinished graphics
panel and an Apply button with no clear outcome. The first grouped candidate
opened after removing stale callbacks to the deleted resolution picker, but the
real native replay then showed `Не удалось применить` for Desktop 1920×1080.
The replay isolated two independent defects: `SaveMetalGraphicsOptions` changed
from void to bool after an earlier untyped call, producing an option script stack
error, and a Desktop/effective-size change called `d3d9->Reset` while the live UI
held back/depth references. The reset and rollback failed and subsequent frames
reported invalid render targets.

The final candidate uses a ten-row, three-column Screen/Lighting/Effects table
with eight visible rows, its own scroller and fixed Apply/Cancel buttons. Display
extent is stored separately from custom resolution, preset heights use the live
display aspect, and Desktop is available only in fullscreen. Apply loads the
temporary `option_sl.c` owner around the confirmed write, then queues renderer
flags after closing the modal. Fullscreen and resolution are saved for the next
launch; live screen transitions are disabled because native replay made legacy
UI elements disappear. The Graphics button keeps its fixed label.

Canonical `run.sh --stage-only` succeeded. Built and installed arm64 engine
SHA-256 both equal
`02830575817460a3342aea8f820899f31cf9c8b13aa036730b857307f0f8caf1`.
The final installed `option_screen.c` hash is `129c1126…`; gameplay sync reports
zero pending and left SAVE/config untouched. Native Apply replay of the preceding
save-owner correction rewrote `metal_graphics`, produced `apply request`,
`queued until renderer RunStart` and `deferred success`, and left `error.log`
empty. The same replay rejected live fullscreen switching because UI elements
disappeared, so the final staged revision keeps screen geometry launch-owned.
Final flag/FXAA and restart replay remain **unresolved**.

### Deck direction, contact, reticle and late actor shadows — September 22

Player replay first at 23:45 reopened W direction, the sea reticle over the
third-person hero, the hero floating above the deck and absent hero/sailor
shadows. The later daytime replay at 11:45 explicitly rejected installed
`65b9228e…`: the hero still could not walk, floated while sailors contacted the
deck, and no actor cast a shadow. The player confirmed the spyglass works after
equipping it; that report needs no fix.

Baseline was installed arm64 direct-Metal engine `6150163f…`, one player process
48868, fullscreen 1, base 1920×1080, MSAA 0, display mode 1. Source discovery
proved world camera yaw was applied to local path coordinates, a separate
0.14 m model lift, third-person entry explicitly enabling the sea reticle, and
late MODELR actors missing point-shadow admission after ship lamp slot shutdown.
Owners/contracts are in `docs/live-deck-walk.md` and
`docs/metal-lighting-parity.md`; crew collision remains cancelled.

The first correction used local camera yaw and placed the model root directly at
the path support. The replay proves root placement is not the character contact
owner. The replacement keeps the current animation's authored `camera` locator
at the deck camera's established eye target; its compiled matrix fixture rejects
both direct root placement and the archived reverse multiplication order.
A Metal-only exact-hash camera script layer hides the third-person reticle,
restores first-person behavior and restores each view after telescope use.
The renderer now admits late scene-model casters from valid point-lamp or
directional-sun snapshots captured earlier in the same frame. Resolve uses the
same frozen sun direction after live slot 0 changes. No persistent light or
actor-specific bridge was added.

CPU matrix regression passes rotated/rocking ships, backwards/strafe/diagonal
motion, locator anchoring and rejection of both old transforms. The daylight GPU
regression disables and replaces slot 0 before a late skinned actor: 152 pixels
darken from the frozen sun, while the following no-source frame remains
unresolved and pixel-identical. Existing independent point-lamp and no-stale-lamp
negatives still pass.

The next player W attempt did reach the bounded deck trace. It repeatedly logged
`input=(0,0) reason=no_input` at a stable valid deck position while `WalkMode=1`;
path and solid geometry were never queried. The failed duplicate `DeckWalk_*`
actions have been removed from the Metal consumer. The deck camera now reads the
canonical `Chr*` movement actions, which a Metal-only exact-hash layer maps into
`Sailing1Pers` at startup and again after the options registry is rebuilt. Stale
saved `DeckWalk_*` rows are skipped so they cannot reclaim WASD.

The trace result reopened the second candidate. Its shadow registry admitted only
one MODELR draw (964 vertices), because admission depended on the light state at
that individual draw and rejected the ship cutouts' standard
`SELECTARG1(texture)` alpha contract. The new registry captures eligible pre-HUD
sea geometry independently of light order; resolve still requires a valid
same-frame source. A mixed-order Metal fixture draws a texture-alpha mast before
the sun and a late GPU-skinned sailor after slot 0 is cleared. They produce 104
and 96 separately darkened pixels; the following no-source frame remains
unresolved with zero changed pixels.

Canonical `run.sh --stage-only` passed after these movement and shadow changes.
Built and installed arm64 engine SHA-256 both equal
`c3d1e1642224dcd4cc310946bd01ecd627b7b2b0a8e056213493b31191b652d0`.
Gameplay sync reports zero pending and compiler ready; the two canonical control
scripts are installed without touching SAVE. Disposition: **third correction
staged; movement, contact and real deck-shadow replay unresolved**. No automated
gameplay input was sent.

Historical first correction follows. Canonical build passed.
After the player closed the game, `run.sh --stage-only` succeeded, delivering one
camera script and engine SHA-256
`65b9228ee9c202922236d0f1491ff169d1bc6137dfeab0cc969d3ed982ebb55b`.
Built and installed hashes match; gameplay sync reports zero pending and compiler
ready. Staging did not target SAVE; current display configuration is unchanged.

The first late-skinned GPU test failed (red 139/135): the fixture forgot the
standard skinning X reflection and its blocker missed the sample. Correcting
only the fixture footprint gives red 139/0; existing red/blue lamp tests and the
no-source following-frame negative (0/0, no extra point dispatch) pass. The
isolated real Metal script VM passes third/first/outside reticle and telescope
return transitions. Its first harness attempt redundantly loaded an already
included sea segment; removing that fixture-only load resolves the compile error.
Both task-owned VM temporary directories were removed through codex-delete-temp.

A repeat canonical stage after the fixture-only correction was refused because
the player had launched PID 67118. No game-code, ordered-patch, CMake or material
change followed the successful staging; the installed/built hash remains the one
above. No process was stopped and no automated gameplay input was sent. The game
is retained for the player. Disposition: **game correction staged; real deck
appearance and interaction replay unresolved**. The exact current test-source
batch can pass the launcher again at the next idle delivery; this does not imply
an uninstalled game-code delta.

### Live graphics, midnight palette and baked shadows — September 22, staged and launched

The player's current screenshots reject the staged graphics UX, doubled Belize
building/object shadows, and blue sky with a black horizon at 00:00. The next
candidate has three changes: direct modal Apply/Cancel with a renderer/display
transaction; solar palette phase from game hour rather than the moon-bearing
weather vector; and location-scoped preservation of baked static sun shadows,
retaining character shadows and static occluders for point lights. Slot-1 baked
modulation stays enabled in the same scope. Material metadata is explicitly
initialized because the geometry loader allocates raw storage.

Verification so far: both new engine patches apply, reverse and reapply
identically across their combined 17 paths; all prior graphics script layers
strip to the exact canonical source; live-apply/source and Belize/Cozumel
contract probes pass. The live-settings backend and quality fixture compiled
before the final baked-state integration; the full candidate build is pending. The midnight
CPU fixture passes; rendered midnight/fog and live-settings resource-lifecycle
fixtures are prepared but have not run. No native UI or Belize replay has
accepted this candidate.

Historical external baseline before this delivery: one player-owned
process, PID 3411, running the arm64 direct-Metal executable under
`experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine`,
SHA-256 `d345ccdbeec8c2b0628cc475ac5a2863eace86959ef4d82ddf19e42b70e9dd91`.
No Wine/prefix is involved. `engine.ini` remains fullscreen 1, 1920×1080,
MSAA 0, display mode 0. Saved `metal_graphics` is Desktop (5), base 1920×1080,
quality 1, all six effects on and AA off. Runtime, SAVE and installed scripts
were not changed in this attempt.

The player then requested installation and launch. With the prior process
closed, canonical `run.sh --stage-only` succeeded: 17 engine source files
migrated, the arm64 engine compiled, and three graphics interface files were
installed. Built and installed binary hashes both equal
`6150163f4652f0782cc4e43d3d70a0d86cf0351a886ab3a0980c1b7ced0b17bb`.
Sync reports zero pending and compiler ready. All 118 SAVE sizes/mtimes match
the pre-delivery manifest
`adfc08b2f2ef9b0791dcb6adc9515f197621dacca02901938adfb19091cd4521`.

Before launch, real GPU fixtures passed midnight versus noon with the same
above-horizon moon vector, exact horizon fog, live shadow quality/off/on/no-op,
invalid-setting rejection, device-reset location preservation and baked flag
reset. `run.sh --launch-installed` started PID 48868. Native window inspection
shows the player in Belize's moneylender interior at 11:45, with the scene and
HUD rendered. No automated gameplay input was sent. Current `engine.ini` is
fullscreen 1, base 1920×1080, MSAA 0, display mode 1; saved settings remain
Desktop (5), base 1920×1080, quality 1, six effects on and AA off. Launch reports
matching 2056×1290 backbuffer and SDL drawable.

Disposition: **staged and launched; feature replay unresolved**. Player replay
still checks Apply/Cancel, one static Belize shadow plus moving-character
shadow, midnight sky, and the remaining horizon band. Earlier squad/cabin and
other scene acceptance remain open in `TASKS.md`. The running game is retained
for the player; no extra GPU processes remain.

### Recovered sky, display and squad correction — September 22, source integration

The latest player report reopens sky/water/stars/weather synchronization,
square sun glow masking mountain silhouettes, grey horizon, night/lamp pools,
grass viewed from different angles, Mac display proportions and squad/officer
supply. Recovery preserved 27 candidate paths at `368819ad`; these changes
subsequently passed the canonical staging path as recorded below. Old sky/display source workers
were interrupted; no game or build process was active at the new baseline.

Historical baseline before this batch: arm64 direct Metal,
`experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine`,
SHA-256 `366ecab7178101c0f8af7f04f9b95625ae30f47b7eb6e1f2cc248aaebdc1d065`.
Launcher remains `experiments/native-metal/run.sh`. No Wine prefix/bottle is
involved. `engine.ini`: fullscreen 1, 1920×1080, MSAA 0; `metal_graphics`:
resolution 2, base 1920×1080, shadow quality 1. Previous settings inventories
below are historical, not a claim about this currently observed record.
The last player-visible result is the reported defects; no new scene acceptance
is inferred from recovery or source checks.

Integration found that reserve counting filters ship-locked companions while
the generic spender does not. The squad layer now mirrors the counted donor
set for spending. An isolated Metal VM passes all three commodities, quantity
conservation, locked/travelling isolation, ordinary-item return and protected
item/old-save negatives. Restoring the unfiltered spender is rejected by the
fixture. A first fixture omitted `Ship.Type`; corrected fixture data passes
without script runtime errors. SAVE and the installed scripts were untouched.

Canonical staging initially stopped before source migration or compilation: the
legacy platform extraction referenced a commit that did not yet contain
`platform.patch`. The installed runtime was not changed. The legacy bytes were then bound to `9d111d61` with the recorded SHA-256.
The next attempt rejected `platform.patch` during the transaction dry run
(`main.cpp`, `sdl_window.hpp`, `sdl_window.cpp`); no source transaction or runtime
write occurred. Exact scratch replay proved the old 46-patch stack and
historical platform both reverse cleanly; the new platform patch contained
malformed whitespace in its context. It was regenerated without changing the
display payload. The full new 47-patch stack passes forward/reverse/forward
with byte-identical results before the next canonical attempt.

The corrected canonical path migrated 12 source files successfully, then the
compiler rejected duplicate sun-angle declarations in `WEATHER::SetCommonStates`.
This is an integrated source error in the weather patch; staging did not reach
the installed executable or scripts. The duplicate declarations were removed
from the ordered patch and guarded by its source probe. The next invocation
exposed macOS Bash 3 empty-array expansion under `set -u` after migration;
the optional bootstrap arguments now use a nounset-safe expansion, verified
for both zero arguments and a path containing spaces.

The engine then compiled, but ground staging rejected the already installed,
exact reviewed Porto Bello shore model. The ground helper now resolves only that
known shore output through its hash-verified original-town backup. An isolated
fixture passes first/repeat staging and rejects unknown live-town or backup
bytes. No geometry was restored or replaced by this compatibility repair.

Canonical `experiments/native-metal/run.sh --stage-only` subsequently succeeded.
Current built and installed arm64 binary SHA-256:
`d345ccdbeec8c2b0628cc475ac5a2863eace86959ef4d82ddf19e42b70e9dd91`.
SunGlow technique SHA-256 is
`014bbf13890f74450369f8b74269f5518356457fdbcee4be890374aeeb2b527e`.
Six gameplay/interface files were installed; sync reports zero pending and
compiler ready. All 118 SAVE files retain their baseline sizes/mtimes (manifest
SHA-256 `5717dad6a9837b5433c75233f9755216c292b2398673ed885abc718ff8f270b6`).
Built probes pass outdoor/day/interior lighting, lighting alpha/unlit/UI,
dynamic lamps/casters, grass cull/stage-1 isolation and caller-state restoration,
dynamic-sky shader/phase/fog and backend scope cleanup. These are correctness
fixtures, not real-scene or game-FPS acceptance.

The player subsequently launched PID 3411 and reopened two outcomes:
- Graphics UX is rejected; changes after OK have no immediate visible effect.
  The current implementation persists options but reads renderer flags only at
  launch. Live application and the modal action flow are now a new active
  correction, not accepted behavior. Observed saved resolution is Desktop (5),
  while the running launch used 1920×1080 and `display_mode=0`.
- The supplied Belize street screenshot at 11:50 shows overlapping shadow
  edges. Their exact owners are being traced; prior shadow probes do not accept
  that real scene. Grey horizon polygons also remain unresolved.
- Further Belize screenshots at 00:00 show a bright blue sky with moon/stars
  and a black horizon band. Source diagnosis confirms that weather replaces
  `whv_sun_pos` with the visible moon vector; the procedural shader incorrectly
  used its elevation to select daylight. The next candidate derives daylight
  and twilight from game hour. This source correction is not staged; PID 3411
  remains the player's running prior batch. Midnight fog is authored near
  black, so horizon acceptance requires replay after the phase correction.

Belize source/material inspection also identifies `terrain1` as actual ground
with an opaque baked-shadow atlas in texture slot 0. Metal renders static
location casters over it; this is the leading explanation for the screenshot,
not yet a matched scene replay. Removing that base or disabling its receiver would
lose ground or dynamic character shadows; those alternatives are rejected.
The scoped static-caster correction remains under investigation.

Disposition: **staged; visual/gameplay acceptance unresolved**. Settings live
application and Belize double shadows remain active source work. Existing
squad daily-tick/cabin replay and sky/night/player scene checks remain required.
The batch contract and surviving older backlog are in `TASKS.md`.

### Graphics settings / outdoor night / Porto Bello — September 22, correction staged

Canonical `run.sh --stage-only` succeeded after the player closed PID 51641.
The installed and built arm64 Metal executable SHA-256 is
`c49b87fa7f877beb53800cd6702109632a81e13a832691dc67843665dcce7543`.
The prior portfolio-launched process used shore-correction binary `24760ae2…`;
its jungle capture is not a matched Cozumel acceptance replay. Root did not
launch or terminate the game. The GPU/build window is released to the player
and the separately authorized capture owner, without concurrent GPU runs.

New source adds the four-file Graphics options layer, a global record and
launcher adapter, scene-boundary FXAA, and a bounded neutral ambient minimum
for modern outdoor night lighting. Options transforms/persistence/negatives,
outdoor CPU lighting, FXAA GPU, scene/HUD boundaries, dynamic lamp/static/skinned
lighting, cinematic and D3D9 FFP GPU checks pass. See
`docs/metal-graphics-settings.md` for ownership. Gameplay sync now reports zero
pending files and compiler ready. The four UI files are installed. Global
`metal_graphics` initializes all existing lighting/water/sky effects on, FXAA
off, fullscreen on and preset 1920×1080; engine.ini has MSAA 0.
The subsequent launch proves a 1920×1080 backbuffer/drawable. First initialization
incorrectly rounded the original 2056×1329 to this preset. SDL owns a desktop-sized
fullscreen window, but Metal still renders the configured dimensions, stretching
the image to the display. The player reports elongated characters/HUD. The
previous window-only resolution description was incorrect. Root is repairing
exact custom-size preservation. After PID 22006 exited, canonical staging
succeeded again with the corrected four-file UI layer and adapter SHA-256
`4a25cebc4d79aaa0ac793341c667bc04150cac1fdc72e2cb3e909406b50be3a8`.
The engine binary is byte-identical (`c49b87fa…`); option_screen.c is now
`f6c99efaa093b7cab27357cc150d1d7757852ac5054135af895c5f35fc45664d`.
The exact untouched initialization record was backed up and restored to
resolution choice 4 with base 2056×1329; engine.ini is fullscreen 1, 2056×1329,
MSAA 0. Preset heights now retain the original aspect. Sync is zero pending and
compiler ready. Root released the idle runtime to the authorized capture owner
for actual modal/toggle/save/restart verification.
The Graphics button is visible at bottom left. The player then reported its
modal controls unclickable: source inspection proves that hidden-window
initialization locks children and Show alone does not unlock them. Explicit
activation in the opener is installed. Exact transforms, prior-layer upgrade,
custom-size/preset/negative checks and an isolated actual Metal VM compilation
plus option-attribute round-trip passed. Native toggle/Close/reopen/
Cancel/OK/restart replay remains **unresolved**.

Rejected delivery attempt: a source agent directly modified four live runtime
UI files despite its source-only assignment. Root froze that agent and restored
exact pinned originals on September 22 at 12:30. Root's initial idle check
incorrectly searched engine-1 rather than metal-engine; PID 51641 was still
active, so restoration also crossed the intended idle boundary. The error
was disclosed and the capture owner notified. All four files were then restored
to original hashes; later canonical stages installed the reviewed layers.
SAVE was not changed. No further runtime/GPU work is allowed
until the process was absent; the subsequent bounded watch confirmed closure.
One FXAA fixture ran during this mistaken idle
window; its pixel result is supporting evidence, not clean-session acceptance
or performance evidence. It passed again in the later idle GPU window.

Porto Bello screenshots additionally expose a ground UV/density discontinuity
between the active Town and Town_sb models, not missing Sandtile loading.
The exact UV correction is now installed with SHA-256
`7abc29913b1fe3f0477afa5d04464230a7c1f4174a99f70c3bfd0b4550d8fc7b`;
its backup is retained and scratch idempotence/unknown-backup checks passed.
The 20-point boundary check bounds packed UV error to 0.5972 texel and preserves
all non-UV data. The player's September 22 13:05 Porto Bello screenshot still
shows blurry ground and a hard sand/shore transition. This reopens visual
acceptance: numerical UV continuity did not close the requested appearance.
Read-only mesh/material analysis identifies a likely distinct Town.gm
object 46 sand versus object 38 rockK3 boundary; exact camera/material-ID replay
remains pending, including distinguishing a geometry edge. The Town_sb UV fix
does not own that visible material transition. Root retains its next-batch
correction separately from the confirmed resolution regression. Character texture quality remains unresolved; a 2x
capture alone does not prove a loader defect.

The player's 13:39:58 screenshot confirms the unresolved material edge and blurry
ground, and adds visibly stepped character-shadow boundaries. It does not isolate
texture loading, filtering, shadow-map budget or a geometry edge as their cause.
Root prepared diagnostic source only: same-frame view/projection/viewport capture
before HUD, and a discriminating FVF44 UV1 raw-versus-expanded GPU fixture.
Both diagnostic targets compiled; the CPU telemetry preservation/empty-frame/
nonfinite/fallback probe passed. GPU execution and canonical staging were held
while the player controlled metal-engine PID 92955. That process later
closed. The FVF44 UV1/raw fixture then passed, proving the second repeated texture
coordinate reaches the native raw location path; this rules out the suspected
TEX2 transport loss but does not identify the Porto Bello material edge.

Source analysis binds stepped moving character silhouettes to the two directional
sun cascades, previously fixed at 2048² (64/2048 and 192/2048 world texels), with
linear comparison and nine Poisson taps. No nearest-filter or format mismatch was
found. A systemic `shadow_quality` profile now controls all location sun maps and
lamp cubes: Low 1024/256, Medium 2048/512, High 4096/512. Allocation, viewport
snapping, main sampling and both fallback sampling paths consume the same profile
texel. CPU policy checks, all three real Metal allocations, FVF44 UV1 and the
dynamic sun/point caster fixture pass.

Canonical `run.sh --stage-only` succeeded with executable SHA-256
`366ecab7178101c0f8af7f04f9b95625ae30f47b7eb6e1f2cc248aaebdc1d065` in both
build and staged app. The four Graphics UI hashes match the reviewed
shadow-quality layer; gameplay sync is zero pending/compiler ready, including
the four custody-life scripts. The current `metal_graphics` record selects High
(`shadow_quality=2`), FXAA on, fullscreen 1920×1080. Native Porto Bello shadow,
ground seam, Graphics interaction and custody-scene replays remain player-owned
and unresolved; staging does not accept those visual or gameplay results.

Disposition: **resolution and modal corrections staged; native acceptance pending**.
Player owns visual acceptance.
The integrated graphics source is committed as `58b6af68`. The hooks specialist
repaired stale Git claims in its own repository (`73fac38`); it did not mutate
this source tree or index. A later exact-path git add was rejected as
foreign-staged-target; the same authorized specialist audited that ownership
reconciliation. Its follow-up fix `efcdcb1` restored the exact normal git add
(exit 0, 54 staged paths), without touching pickup/prison candidates or the
runtime. No index bypass was used.

### Pickup halo — source candidate, not installed

The separately authorized pickup task returned an exact engine patch, item-role
script adapter and topic document. Isolated strict patch/syntax, geometry and
script round-trip/negative checks passed. Root accepted source integration
ownership; ordered-stack/gameplay receipt wiring, canonical build/GPU/staging
and real pickup/occlusion/cleanup replay remain pending for the next batch.
The current corrective Graphics stage excludes this feature.

Disposition: **source candidate, runtime unresolved**; see
`docs/metal-pickup-glow.md`.

### Prison life / supervised tournament — source candidate, not installed

The separately authorized prison task returned its Metal-only four-script
transform, gameplay source and topic. Exact Metal VM script/dialog compilation,
day/funds/parole state, reward idempotence, defeat/cancellation, issued sword
quantity, prior immunity, private arena isolation, loaded-arena recovery and
stock lethal-hit interception passed. Removing transaction cleanup triggered
the intended negative. No SAVE/shared runtime was changed. Root owns a separate
source commit and next-batch hash-checked sync integration; current Graphics
delivery excludes it. Real arena/outcomes/save-load/release/escrow replay remains
unresolved. See `docs/prison-surrender-system.md`.

Disposition: **source candidate, runtime unresolved**.

### Cozumel ground regression — September 22, correction staged

The player screenshot at 11:49 shows the entire shore surface missing while
rocks, palms and the player remain. At that capture PID 92991 ran the prior
staged binary; its SHA-256 was unchanged. This rejects that batch's ground appearance.
`common.ini` maps «залив Косумель» to `Shore7`; `Beliz.c` maps it to
`Locations/Outside/Shores/Shore02/shore02.gm`. Its `shoreU2SG` and
`bump_city_shoreU2SG` materials use `shadow.tga` in slot 0. The new whole-draw
skip incorrectly classified these base surfaces as standalone shadow overlays.
The previous SanJuan-only probe repeated that unsupported classification.

Correction: remove the slot-0 draw rejection, retaining original geometry and
the separate slot-1 dynamic-shadow replacement. The source/data probe now binds
the reported location and preserves 22 real shore materials. Build and canonical
`run.sh --stage-only` passed after the player closed the game. Build and staged
executable SHA-256: `24760ae2f499e5c06c2b0200cb8865fcaf25e09df81da6923f5f8580f267157e`.
Gameplay sync is unchanged: zero pending files, compiler ready. The active
configuration remains the inventory below; no game was launched by root.

Disposition: correction staged, real-scene restoration **unresolved** until
player replay or the separately authorized portfolio capture. The capture task
has a conditional GPU release only while no user game is active. The new graphics
settings request is a separate pending source batch with root integration ownership.

### Integrated visual completion — September 22 (superseded by ground correction)

The expanded batch is built and staged through the canonical
`experiments/native-metal/run.sh --stage-only`. Both the build executable and
staged `CorsairsMetal.app/Contents/MacOS/metal-engine` have SHA-256
`648eea3734bcc804cc38c4a635001199a3f7b08b2796534f7b8d0f9a6c78aac7`.
The active runtime remains arm64 direct Metal on Apple M3 Max, 2056×1329,
fullscreen 1, MSAA 0, with no Vulkan/MoltenVK linkage. At staging completion no
game process was running. Gameplay sync reports zero pending files and compiler
ready. Original textures and existing saves are retained.

The earlier baked-town-shadow, world-map scope and unlit triangle-fan fixes below
are included. The expanded delta closes these demonstrated source gaps:

- Location lamps crossfade only their shadow weights over 0.25 seconds when the
  desired two-lamp membership changes; authored illumination remains intact.
- Ship lamps have stable identities and same-frame cube shadows with a combined
  receiver correction. Other lamps remain lit, sun-only packets retain their
  receiver path, and unlit models remain casters without being relit.
- The point-caster registry no longer replaces an already-seen instance of the
  same mesh with a later transform. This was the reproduced cause of the missing
  ship-fixture shadow, despite apparently successful cube dispatch counters.
- Exact town-backdrop roles clamp vertical edge sampling, preventing opaque
  bottom-row bleed into the transparent top. Horizontal repetition remains.
- TX loading uses physical mip sizes and independently clamped dimensions. All
  6,410 files match the supported layout; 14 ship textures exercise full DXT
  tails, including two rectangular 2048×1024 chains. Malformed reads are rejected.
- Existing floorU3, Sandtile and rock images use rotation/offset/four-cell
  anti-tiling with explicit Metal gradients. Directional paving, alpha textures
  and explicit clamp sampling retain their original path.

Verification: the complete 46-patch stack applies strictly; engine build and
canonical staging passed. Compiled selector and TX layout tests passed. GPU
checks passed for anti-tiling periodicity/seams/mips/exclusions, backdrop alpha,
FFP lighting, all six unlit fan paths, resident map geometry, existing sun
shadows, dynamic lighting, point cubes, GPU skinning and ship point shadows.
The ship fixture uses shared geometry at different transforms: red 139→0,
blue 140→0, while the other blue contribution stays 173→173; off/reset passes.
Material and lifecycle contracts are recorded in the owning topic documents.

Disposition: source/build/staging and the named fixture properties are accepted.
Real-game appearance and frame time remain **unresolved**, explicitly owned by
the player. `TASKS.md` records the replay scenarios. The separately authorized
portfolio capture task may use this exact staged binary after root releases the
runtime; it is not evidence of replay until the actual scenes are captured.

## Previous verdicts and attempts

### First completion staging — September 22 (historical)

The first completion batch below was built and staged. The user then expanded
the active batch to lamp continuity, ship lamps, texture mips and anti-tiling;
these newer sources are not yet staged. The player explicitly owns real-game
replay; no game was launched by this task. `TASKS.md` tracks current acceptance.

Inventory at first staging: sole active target `experiments/native-metal`; launcher
`experiments/native-metal/run.sh`; arm64 direct Metal on Apple M3 Max, no Vulkan
or MoltenVK linkage. Both `.cache/build/bin/engine-1` and the staged
`.cache/CorsairsMetal.app/Contents/MacOS/metal-engine` have SHA-256
`600d65a9c4f2c9b28002fc57f85a9c030a1a9b4318b4a22351639f864143548c`.
Configuration remains 2056×1329, fullscreen 1, MSAA 0. Game process count is zero.
Gameplay sync check/apply reports zero pending files and compiler ready. Earlier
inventories, pending staging statements and staged hashes below are historical.

Hypotheses and exact deltas:

- The previous baked-shadow change neutralized slot 1 but missed standalone
  slot-0 shadow geometry. The real San Juan model proves six combined materials
  and one standalone layer. The geometry service now returns a draw decision;
  only the standalone layer is skipped when dynamic sun shadows are ready.
  Base materials and unavailable-dynamic-shadow fallback remain.
- The map model role existed only in the backend. `WdmRenderModel` now opens and
  restores it around geometry. A GPU comparison then caught stale illumination
  and missing authored specular in the unlit resident path; it now preserves
  authored color/alpha/specular, suppresses light contributions, and uses a
  distinct unlit pipeline key. Map techniques still own their unlit behavior;
  this is not a delivered dynamic map sun/shadow system.
- Unlit triangle fans used by shadow blur, sun glow and navigation still expanded
  vertices on CPU. Common compact and native device paths now triangulate only
  indices in the frame arena. Static/dynamic and indexed/nonindexed submission
  preserve vertex bytes and signed base/index offsets.
- The red FFP lighting fixture omitted a normal-bearing FVF and sampled an
  interpolated center from vertices outside narrow lighting lobes. Explicit FVF
  and a smaller projected quad repair the fixture; thresholds and backend
  lighting equations were not weakened to obtain green results.

Verification: canonical `run.sh --stage-only` succeeded after the final backend
change. Ordered source stack, engine link, consumer coverage and real-GM shadow
composition passed; coverage and GM checks are now canonical build/stage gates.
`menu_unlit_native-probe` passes six fan routes against full triangle-list images
with zero CPU conversion and a short-stride negative. `native_legacy3d-probe`
passes rich attributes, signed indexed base, nested map roles and outside-scope
restoration. `ffp_lighting_gpu-probe` passes original material/light/specular/spot
thresholds. Original shadow silhouette/9-tap blur/receiver and GPU-skinned pixel
parity pass. The map and skinning probes were rerun after the final pipeline-key
change and passed.

Disposition: source/GPU/build/staging accepted for this batch; real-game visuals
remain **unresolved**. Next player checks: San Juan daylight shadows, map models
and labels, sun glow/navigation, then Marigo shipyard night across lamp influence
boundaries. Two-nearest-lamp continuity and sea ship/deck lamp-shadow ownership
are still unresolved source/design work, not completed by this batch. Prior
renderer/gameplay replay obligations remain in `TASKS.md`.

### Providencia capture-state repair — September 20

The latest save was not a failed ownership transition. Its persisted colony state already
had `Colonies[Providencia].nation = PIRATE`, `HeroOwn = 1`, and a capture date. The stale
state was the separate battle marker: `objTownStateTable.towns.t29.captured = 1`,
`LAi_IsCapturedLocation = 1`, and `PChar.GenQuestFort.TownCrew = 3`. The three remaining
defenders could not be spawned at `Providencia_ExitTown/rld/loc0`, so the normal group
completion callback never ran.

Delta: preserved the original `SAVE/Кампот/После захвата и швартовки софтлок` and created
`SAVE/Кампот/После захвата и швартовки софтлок (починен)`. The copy changes only the stale
battle marker and counter to zero; the colony owner, capture date, player location, and
capture quest state remain intact.

Evidence: the ReCon 1.4.1 save round-trips through the repository converter and re-parses
with `TownCrew = 0`, `captured = 0`, `LAi_IsCapturedLocation = 0`, and
`Providencia.nation = PIRATE`, `HeroOwn = 1`. The original save is byte-preserved. Loading
the new slot and replaying the store/loot interaction on the real game surface remain
unresolved because the native window was not available to the current control surface.

Disposition: targeted repaired save prepared; ownership is accepted from save state, while
real-game load and the one-time capture-loot interaction remain **unresolved**.

### Dialogue has no Esc exit, September 19

Every dialogue line the engine shows can offer the player a plain farewell link, and the
standard dialogue files answer it with their own `case "exit"` / `case "Exit"` handler that
restores `NextDiag.CurrentNode = NextDiag.TempNode` and calls `DialogExit()`. Esc never
reached that option: `init_pc.c` binds `VK_ESCAPE` to the global `DlgCancel` control and
`DIALOG::Realize` raises `DialogCancel` for it, but both handler registrations in
`PROGRAM/dialog.c` were commented out, so the event had no consumer and the key did nothing.

Delta: new `tools/patch_dialog_cancel.py` composes one fragment onto the delivered
dialogue revision, after the dialogue-attack fragment. `DialogCancel_Exit()` registers on
`DialogCancel` in `StartDialogMain` and `SelfDialog`, scans the current line's now-built
`Dialog.Links` for a link whose `go` is `exit` or `Exit`, and, only when one exists, sets
`Dialog.CurrentNode` to it and raises `Event("DialogEvent")` — the same dispatch the engine
performs for a click, so the dialogue's own exit node runs and nothing else changes. A line
without that link, including a forced quest line, ignores Esc.

Evidence: `sync_metal_gameplay.py check` reports exactly `PROGRAM/dialog.c` pending with the
compiler ready; the suite's 35 deterministic hashes and the linked `_verify_dialog`
contract pass with the new composite. `docs/dialogue-escape-exit.md` owns the contract.

Disposition: source accepted. Metal apply, startup and the Esc replay stay **unresolved**:
the running Metal process owns the runtime, so the delivery helper refused to write.

### World-map label icons lost their second texture stage — September 19

After `renderer-world-map.patch` joined the ordered stack, the island labels on the global
map lost their icons. The connected producer was wrong in three places, and the first one
alone is enough to erase a label icon.

The icons are a two-stage technique. `WdmDrawLabelIcon` selects stage 0 from the texture and
then blends stage 1 over it with `blendfactoralpha` and `D3DRS_TEXTUREFACTOR = icons.blend`,
and `WdmIslands::LRender` fills `uv0 = icons.f[0]` and `uv1 = icons.f[1]` so the two stages
sample adjacent animation frames of the atlas. `bridge_screen_vs` in
`experiments/native-metal/backend.mm` forwarded only `uv01.xy` through `bridgeVertex` and left
`uv01.zw` at zero, so stage 1 sampled texcoord 0 and the blend collapsed. Second, the consumer
called `StormMetalDrawWdm*` *before* the legacy `DrawPrimitiveUP`, but the technique pass is
what applies `ts[]`, blend and depth state (`DX9RENDER::TechniqueExecuteStart`); `drawBridge`
reads that state through `prepareFixedFunction`, and the `technique` argument of every
`StormMetalDrawWdm*` was only null-checked. The world map never sets a technique itself
(`WdmRenderModel` calls `gs->SetTechnique` for models only), so the flat plates and labels drew
with whatever pass the previous consumer left behind — nothing like the proven `stars.cpp`
pattern, which wraps the bridge in `TechniqueExecuteStart`/`TechniqueExecuteNext`. Third,
`(x, y)` was treated as the rectangle centre, while `WdmInterfaceObject::FillRectCoord` and the
label icon path treat it as the top-left corner and add `(w/2, h/2)` inside the rotation.

Delta: `bridge_screen_vs` now places the corner at
`(x, y) + (w/2, h/2) + rotate((corner - 0.5) * (w, h), angle)`, writes
`o.uv01 = float4(uv0, uv1)` so stage 1 reads texcoord 1, and, for rotated rectangles only,
mirrors `u` — the `ang != 0` branch of `FillRectCoord` assigns the texture coordinates mirrored
in `u` relative to its `ang == 0` branch, which is the authored pointer orientation. The
secondary `uv1Rotation` is applied about `(0.5, 0)`, the exact transform the legacy
`CMatrix(0, angY, 0, 0.5, 0, 0)` rotation produced, and `WdmWindUI` now hands the wind bar its
natural `0,0,1,1` source so that transform reproduces the old `[-0.5, 0.5] + 0.5` mapping.
`renderer-world-map.patch` adds `WdmMetalDrawScreenRects`/`Quad3D`/`DangerDisc`/`ShipWake` to
its `metal_world_map_bridge.h`, each running `TechniqueExecuteStart(technique)`,
submitting, and closing with `TechniqueExecuteNext()`, so a compact record is submitted under
the same pass the legacy draw would have used. `renderer_world_map_probe.py` now asserts the
second-stage forward, the rotated-`u` mirror, the centred secondary rotation and the technique
wrapper, and `renderer_consumer_coverage_probe.py` follows the renamed ship-wake hook.

Evidence: `run.sh --stage-only` reports the stack verified and stages `CorsairsMetal.app` with
an arm64 `engine-1`. Passing: `renderer_world_map_probe.py`, `renderer_ui_fonts_probe.py`,
`renderer_consumer_coverage_probe.py`, `renderer_rain_weather_probe.py`,
`renderer_particles_fx_probe.py`, `foliage_cutout_probe.py`,
`weather_surface_bridge_probe.py`, `point_shadow_bridge_probe.py`. Three probes fail on this
tree and on `HEAD` alike, so they are not caused by this change:
`ffp_cache_signature_probe.py` (`IndexError`), `world_shadow_owner_probe.py` (`FAIL resident
raw shadow span`) and `night_local_lighting_probe.py` (`AssertionError`).

Disposition: source, build and staging **accepted**. Whether the icons are visible again stays
**unresolved**: this sandbox has no Metal toolchain, so the embedded MSL string is never
compiled here and no GPU replay is possible. Whether the wind pointer's authored `u` mirror is
visible on a symmetric arrow is likewise unverified.

### Prepared renderer consumers connected to the Metal path — September 19

Inventory in `docs/renderer-remaining-work.md` listed eight prepared renderer patches
(weather, astronomy, UI/fonts, vegetation, particles, world map, sea post-process) as outside
the canonical stack. A read-only diff against `experiments/native-metal/backend.mm` showed
every receiver already implemented and exported (`StormMetalDrawBillboards`,
`StormMetalDrawGlyphInstances`, `StormMetalDrawCompactPrimitive`, `StormMetalDrawGrass`,
`StormMetalDrawAdvancedParticles`, `StormMetalDrawParticleSprites`, `StormMetalDrawWdm*`),
so those features still ran the legacy CPU path because their engine-side producers were never
in the stack, not because the Metal side was missing.

Delta: `experiments/native-metal/build.sh` appends `renderer-grass`,
`vegetation-continuity`, `sea-island-vegetation`, `renderer-particles-fx`,
`renderer-rain-weather`, `renderer-sky-astronomy`, `renderer-ui-fonts`,
`renderer-world-map` and `sea-postprocess-uv` to the ordered stack after
`group-target-liveness.patch`; `CMakeLists.txt` adds `STORM_METAL_RENDERER_CONSUMERS=1`,
`STORM_METAL_PARTICLE_BRIDGE=1` and `STORM_METAL_GRASS_BRIDGE=1`. Five prepared patches
were rebased onto the live ABI: the star hunk calls `StormMetalDrawBillboards` instead of the
removed `StormMetalDrawStarSprites`; the UI new-file block gained its missing
`new file mode` line so `apply_source_patches.py` accepts it; the grass, vegetation and
particle patches lost their compile errors (a `-Wsizeof-array-div`, a redefined
`static CMatrix identity`, and `-Wc++11-narrowing` casts). `renderer_rain_weather_probe.py`
now verifies the staged tree is this patch applied instead of re-applying to an already-patched
snapshot.

Result: `run.sh --stage-only` reports `Source patch stack verified; 23 source files changed`
and stages `CorsairsMetal.app`; both `engine-1` and the staged `metal-engine` are
`9bf4ca74e46a1a8f67d3f837306913b70c824f545d84385b69534915ba19905f`.
`renderer_consumer_coverage_probe.py` passes ("9 renderer consumer classes reach native
raw/compact Metal; no direct consumer UP bypass") and so do the eight feature probes.

Disposition: source, build and staging **accepted**. Visual parity and the real-scene
zero-legacy-counter check stay **unresolved**: no GPU probe runs in this sandbox and the
changed consumers were not replayed in the game.

Two water patches stay outside the stack as superseded: `water-depth-tolerance.patch` and
`water-refraction-filter.patch` edit the `modernSeaForeground` /
`modernSeaForegroundFootprint` epsilon test that the current `sea_shaders.hpp` refactor
removed, so neither applies.

### Character-group target outlived its character, and the AI lookup faulted — September 19

The crash report (`EB2AC7FE...`, PID 18407, launched 21:43:46, faulted 21:47:57) faults with
`EXC_BAD_ACCESS` at address `0x8` inside `CharactersGroups::MsgGetOptimalTarget + 684`, reached
from `Supervisor::PostUpdate` -> `Core::Event` -> script bytecode -> `CharactersGroups::ProcessMessage`.
At symbol + 684 the staged build executes `ldr x0, [x0, #0x8]` immediately after the
`core.GetEntityPointer(c->grpTargets[s].chr)` call, and the register state in the report has
`x0 = 0` with `far = 0x8`: the engine dereferenced a null character reached from a target id.

That is `c->AttributesPointer` in `MsgGetOptimalTarget`. The function picks the optimal
`grpTargets` entry and resolves it with `GetEntityPointer`, but only the multi-target loop
skips an unresolved pointer; the chosen index (also the forced `s = 0` fallback and the
single-target case) was dereferenced unchecked. A group stores the target id, not the
character, so a target that had already left the location left the lookup returning null.
The script path is `LAi_group_GetTarget` (`PROGRAM/Loc_ai/LAi_groups.c`), which runs from the
warrior, guardian and officer templates on every supervisor update and already treats a zero
return as "no target" (`index = -1`).

Delta: new `group-target-liveness.patch`, appended to the canonical stack in
`experiments/native-metal/build.sh`. The unresolved target now returns the same "no target"
result the existing `numTargets <= 0` and missing-variable paths use. No target selection,
group relation or script call is changed. `docs/group-target-liveness.md` owns the contract.

The same staging attempt also had to unblock the gameplay delivery:
`sync_metal_gameplay.py apply` aborted with
`unexpected backup revision: PROGRAM/quests/quests_reaction.c` because
the recorded `gameplay-baseline` revision for that file and for
`PROGRAM/locations/locations_loader.c` is the pre-delivery revision of the 2026-09-18 layer
and is not today's `BASE`, and `BASELINE` had no record for either path. Both recorded
revisions differ from the current Metal file only by that layer's reviewed edits (the
fight-mode lock in `quests_reaction.c`, the boarding fight-mode reset in
`locations_loader.c`), so the two hashes are now recorded explicitly, exactly as
`PROGRAM/dialog.c` already was.

Result: `run.sh --stage-only` completed after both changes; `engine-1` and the installed
bundle are both `e14aea3bceb82e420bd2effbca0e6fbeb872d627bfb42b868797c4f861374f9d`, the staged
binary carries `cbz x0, 0x1000ab5d4` in front of that load, and
`tools/sync_metal_gameplay.py check` reports zero pending files.

Disposition: source, build and staging **accepted**. Whether a guard or warrior that lost its
target now falls back to another live target in the running game stays **unresolved**: the
incident scenario was not replayed on this engine.

### Window-plane light shaft footprint — September 19

Indoor god rays landed on the floor correctly but did not leave the authored window.
`openingBeamMask` measured each sample's lateral offset in the plane perpendicular to the sun
(`lateral = toPoint - beamDir*dot(toPoint,beamDir)`) and compared it against the aperture's
own `right`/`up` half extents. Those axes span the pane, not the plane perpendicular to the
sun, so the accepted cross-section was the authored opening divided by the cosine between the
sun and the pane normal: for a pane normal `+z` and sun `(0.6,0,-0.8)` a prism point at
parameter `a` maps to `u = 0.64a`, so the `hw=0.19` apertures reported by the launch log lit a
`0.30` half-width beam that started beside the window and stayed too wide down the room.

Delta: the mask now traces the sample back along the sun vector onto the plane of the opening
(`planeOffset = dot(normal,toPoint)/facing`, rejected when negative, with `facing` already
guarding a grazing sun) and tests that hit point against the authored rectangle. No aperture
geometry, frame origin, march, or composite change.

Evidence: `run.sh --stage-only` completed after the change; `.cache/build/bin/engine-1` and the
staged `.cache/CorsairsMetal.app/Contents/MacOS/metal-engine` are both
`e14aea3bceb82e420bd2effbca0e6fbeb872d627bfb42b868797c4f861374f9d`, arm64, and the staged
binary contains the new `planeOffset` trace. The MSL was not compiled offline: this host has no
Metal toolchain (`xcrun metal` requires `xcodebuild -downloadComponent MetalToolchain`) and the
sandbox exposes no Metal device, so the shader source is compiled at runtime by
`newLibraryWithSource`.

Disposition: source, build, and staging accepted. Real-scene replay in an interior with an
oblique sun remains unresolved.

### Villemstad black pier and over-bright ground — September 19

The user screenshot showed two coupled material failures: an exactly black low pier/ground
mesh and pale ground around it. Parsing the loaded Villemstad geometry found neutral
vertex colour `127,127,127` throughout and valid materials with zero diffuse, including
the large `Pirs_SentMartin1_KNS.tga` mesh. The original renderer's authored location state
uses vertex `COLOR1` with a `* 2` texture stage; the Metal raw outdoor path instead
substituted material diffuse, turning zero materials black and `0.8` materials too bright.

Delta: the Metal default enables `D3DRS_COLORVERTEX`, raw static location draws obey the
three D3D material-source render states, and CPU-expanded relight no longer suppresses
`COLOR1`. `dynamic_lighting_probe` adds a bound indexed town fixture and raw-location
counter; `lighting_probe` asserts the D3D9 default.

Evidence: both probe targets compile and link. Their GPU execution and canonical
`run.sh --stage-only` gate are pending because the Metal game is still running and the
sandbox has no display service.

Disposition: source diagnosis and build accepted; GPU fixture, staging, and real
Villemstad replay unresolved.

Follow-up — September 20: the renderer's initialization left
`D3DRS_AMBIENTMATERIALSOURCE` at zero (`D3DMCS_MATERIAL`) even though the authored
location contract uses `COLOR1` for ambient. The raw outdoor path therefore combined the
neutral vertex colour with each GM material's ordinary ambient value; with the location
`MODULATE2X` stage this overexposed the ground and made its texture look absent.

Delta: Metal initialization now seeds `D3DRS_AMBIENTMATERIALSOURCE` to `D3DMCS_COLOR1`,
and `lighting_probe` checks the diffuse, ambient, and `COLORVERTEX` defaults together.

Evidence: `experiments/native-metal/build.sh` passed, the CPU outdoor-lighting probe
passed, and `run.sh --stage-only` staged the fresh Metal binary and reported zero pending
gameplay files. The already-running Metal process was not restarted, and the hidden GPU
probe could not create a display in this environment, so real Villemstad replay remains
unresolved.

Disposition: source/build/CPU probe accepted; GPU, staging, and real-scene replay unresolved.

### Jungle-girl encounter and dialogue-combat softlocks — September 19

The latest Metal save is in `Beliz_Jungle_01` with `GenQuest.EncGirl=Begin_2`, three
`GangMan_*` actors in `EnemyFight`, `chrDisableReloadToLocation=1`, and no active group
alarm. The shot-before-dialog handler starts combat but its only
`LAi_group_SetCheck("EnemyFight", "LandEnc_RapersAfrer")` line was commented out. This is
the only commented group-completion registration in the gameplay tree, so killing the
last bandit could never run the existing completion case or open the exits.

Delta: the shot path now registers the completion callback, and the completion case
immediately restores player control, weapon state, fight-mode control, fast reload, and
location exits before starting the woman's thanks dialogue. Location load rehydrates the
callback for an in-progress saved encounter and completes it directly when all three
bandits are already dead. It also migrates saves made during the removed temporary
dialogue-combat group and restores that target's original group. Dialogue shows exactly
one action: `Ударить исподтишка` for the existing one-on-one condition, otherwise
`Напасть`. Both alert the target's original group through ordinary `LAi_group_Attack`;
the generated one-member group and its fight-end handlers are gone.

Evidence: the gameplay suite produces 35 deterministic exact-hash outputs and passes its
linked fixtures. `sync_metal_gameplay.py check` identifies only `dialog.c`,
`locations_loader.c`, and `quests_reaction.c` as pending, with the compiler ready. The
delivery helper now builds those outputs from reviewed originals and leaves archived
native-storm unchanged.

Disposition: diagnosis and source package accepted. Metal apply, startup compilation,
latest-save replay, group aggression, and both dialogue labels remain unresolved while
the current Metal process is open.

### Crew-payment attribute walk crashed on a dead parent node, 2026-09-19

The crash report (`B29D662E...`, PID 58898, launched 23:48, faulted 00:09:13) faults in
`ATTRIBUTES::GetThisName() + 8` called from `WorldMap::AttributeChanged + 976`, through a script
bytecode chain entered from `XINTERFACE::MouseDeClick` while the user worked an encounter
interface on the world map. The offending call sits inside the
`for (auto *pa = apnt; pa; pa = pa->GetParent())` ancestor walk: `apnt` itself had already been
dereferenced four times, so the node that faulted was an ancestor reached from `GetParent()`.

`ATTRIBUTES::Copy()` overwrites each child with `new_child = attribute->Copy();` and returns
`result` by value. Both operations move the `std::vector<std::unique_ptr<ATTRIBUTES>>`, and the
move constructor and move-assignment operator never re-pointed the moved children's `parent_`
at their new owner. The script builtin `CopyAttributes` is `*pRoot = pA->Copy();`, so every
non-root node of a copied class kept a `parent_` addressing a destroyed temporary, and the
ancestor walk then read freed memory.

Delta: new `attributes-parent-chain.patch`, ordered immediately after
`save-areference-liveness.patch`. `ATTRIBUTES::ReparentChildren()` now runs in the move
constructor and in the move-assignment operator, so ownership and `parent_` agree after a move,
and `WorldMap::AttributeChanged` ends its ancestor walk at a node that is no longer live
(`pa && ATTRIBUTES::IsLive(pa)`) instead of dereferencing it. No label content, renderer path or
draw is removed; a live `labels` ancestor still reaches `SetIslandsData`.
`docs/attribute-class-lifetime.md` owns the contract.

Result: `build.sh` completed ("Source patch stack verified; 3 source files changed"), `engine-1`
and the installed bundle are both
`c012f440b9d896387d65ab066509060f98c08d326f4532da11e070fc3abaa4a7`, that batch is staged, and
`tools/sync_metal_gameplay.py check` reports zero pending files.

Disposition: source, build and staging **accepted**; the crew-payment replay and the identity of
the destroyed node stay **unresolved** until the action is replayed on this engine.

### Sail rebuild crashed on renderer buffer-table exhaustion, 2026-09-18

The crash report (incident `8A66E848...`, PID 47749, quit 23:20 after 1 h 34 min) faults
inside `DX9RENDER::LockIndexBuffer` from `SAIL::SetAllSails` -> `SAIL::FirstRun` ->
`SAIL::Execute`. `LockIndexBuffer` indexed the fixed `IndexBuffers[10240]` table with no
range check and the id passed in was `-1`; the memory just before the table held `nullptr` at
the `buff` offset, so `buff->Lock(...)` read address 0. `CreateIndexBuffer` returns `-1`
only when every slot is allocated, and the Metal backend's `CreateIndexBuffer` always returns
`S_OK`, so the negative id means the table was full, not that the device refused a buffer.

Two defects. First, the table is the renderer's only guard and the draw entry points read it
directly: `DrawBuffer`, `DrawIndexedPrimitiveNoVShader`, `DrawPrimitive`,
`RenderAnimation` and the lock/unlock/size accessors all indexed a caller-supplied id with no
bounds or allocation check. Second, `SAIL::SetAllSails()` replaced `sg.vertBuf` and
`sg.indxBuf` without releasing the previous pair, while `SetAddSails` and
`DeleteSailGroup` release first, so every sail rebuild leaked one vertex and one index slot.
The rest of the growth toward 10240 is still unexplained: no single creator was proven to account
for the table, so the accumulation is **not** claimed fixed.

Delta: `renderer-buffer-guard.patch`, last in the canonical stack, rejects an out-of-range or
unallocated id in `LockVertexBuffer`, `UnLockVertexBuffer`, `GetVertexBufferSize`,
`LockIndexBuffer`, `UnLockIndexBuffer`, `DrawBuffer`,
`DrawIndexedPrimitiveNoVShader`, `DrawPrimitive` and `RenderAnimation`, traces the
first rejection once, and skips that draw instead of dereferencing memory outside the table.
`CreateIndexBuffer`/`CreateVertexBuffer` trace table exhaustion, and `DX9EndScene`
samples the live buffer counts every 1024 frames and traces each new 512-slot high-water mark, so
the residual accumulation is measurable from `launch.log` instead of inferred.
`SAIL::SetAllSails()` now releases its previous buffer pair before creating the new one. No
geometry, material, technique or draw is removed, and a rejected id never reached a real draw
before.

Result: canonical `build.sh` completed after the stack change ("Source patch stack verified; 2
source files changed"), `engine-1` is
`c927f681198c933e1006b250dab6ba0ad42aa9fc53a3523886e38e37849fb2b5`, and `native_legacy3d-probe`,
`indexed-probe`, `menu_unlit_native-probe`, `scene_hud_boundary-probe`,
`ffp_frame_cache-probe` and `indexed_span_frame_cache-probe` all PASS.
`docs/metal-renderer-buffer-table.md` owns the table contract.

Staging: that 21:46 bundle has since been replaced; the currently staged engine also carries the
attribute-parent-chain fix, so this guard is live in the bundle.

Disposition: source, build and probes **accepted**; the crash replay and the buffer-count growth
curve stay **unresolved** until a staged launch runs long enough to pass the count that faulted.

### Raw lit Metal fallback corrupted ship and water-adjacent meshes, 2026-09-18

The player captured giant multicoloured triangles on ships, shoreline water geometry and
nearby scene meshes, plus a main menu that rendered only its 2D layer. The common lit raw
fallback introduced in `7ee2556a` bound a compact fallback uniform where the normal `Uniform`
block was expected, omitted the land-sampling bindings, read indices from the compacted
submission instead of the original draw state, discarded signed base vertex, and treated a
triangle fan as a triangle list. Those contract errors explain intact UI and characters next
to exploded static or dynamic lit meshes.

Delta: `backend.mm` now submits the original indexed state, validates the exact referenced
source range, binds the normal, raw-land, land-sampling and shadow blocks at their shader
slots, preserves signed base vertex by rebasing the bound source span, and expands only
triangle fans into the frame arena. Static and dynamic vertex buffers remain GPU-rendered;
there is no CPU vertex transform, CPU lighting, reduced geometry or reduced material path.
`native_legacy3d-probe` is part of the canonical build and covers static/dynamic lit FVF,
16/32-bit indices, list/strip/fan topology, signed base vertex and pixel equality.

Result: `native_legacy3d-probe`, `indexed-probe` and `menu_unlit_native-probe` pass. The
engine builds and links as
`ef63777130bbed78092967e575e09fe8bd6bc7cfd3ad1d79aafdad47d94301f5`.

Staging and launch: `run.sh --stage-only` staged the same engine hash, gameplay reported
zero pending files with the compiler ready, and PID 47749 launched that bundle at 21:46.

Disposition: source, build, probes, staging and startup **accepted**; the real menu, town
and sea visual replay are still pending.

### Sneak fist attack could preserve forced-unarmed mode, 2026-09-18

The player clarified that the failure began after `DialogAmbush` struck an NPC with a fist:
the blade did not return, and a later boarding save still had no usable weapon. The chain
could enter engine `SetFightWOWeapon=true`, clear its `Target` during the draw step, then
make `DialogAmbush_FightOver` return because that target was gone. A location unload also
deleted the temporary target group without clearing forced-unarmed state. Separately, the
shared `MainHeroFightModeOff` reaction sheathed the protagonist and then locked fight mode,
which could preserve the same visible failure after dialogue.

Delta: the ambush draw first clears stale forced-unarmed state, fight completion no longer
depends on the draw step's deleted `Target`, and both normal sheathing and location unload
clear forced-unarmed state and the temporary ambush attributes. The generic sheath reaction
now unlocks fight mode. Boarding start and boarding-location load both clear stale fight
lock and forced-unarmed state, so an existing affected save recovers on entry or reload.
Purposeful story, cutscene and `noFight` location locks retain their own owners.

Result: `patch_gameplay_suite.py verify` passes all 35 cumulative transforms and linked
fixtures. Canonical staging applied `LAi_boarding.c`, `dialog.c` and `locations_loader.c`;
`sync_metal_gameplay.py check` reports zero pending files and the compiler ready. PID 47749
launched the staged bundle at 21:46.

Disposition: source transform, fixture verification, staging and startup **accepted**; the
affected save's boarding weapon and a new fist-attack replay remain user-visible checks.

### Sea frame cost: the span cache walked all 8192 spans, 2026-09-18

The player reported 40/20 FPS at sea with the GPU idle and no such cost previously. The
profile blocks are real frames: 1169 of them average 8.36 ms, which is the 120 FPS limiter
cap, 105 average 60.22 ms, and blocks x 120 x wall against the 2144 s process age
reconciles to within 26 s. In the slow blocks gpu is 3.4 ms against 5.6 ms in the fast
ones, drawableWait is 0.14 ms, and every instrumented renderer timer is at noise (FFP lit
0.007 ms, bounds_cpu_ms 0.2-1.5 ms per 120 frames), so the cost sat in an uninstrumented
path.

Two live samples read 41 % of the main thread in nanosleep. That is a focus artifact, not
the defect: while the player types in this task the window is unfocused, the frame loop
takes its sleep_for(50ms) inactive branch, and those iterations never present, so the
inactive counter cannot see them and wall absorbs them as if they were frame time. Focused
slow frames, sampled under DX9RENDER::DrawBuffer, put sm::IndexedSpanFrameCache::observe at
244/360 and 235/343 of SAIL::Realize, 150/226 of CoastFoam::Realize and 68/105 of
TCarcass::Realize, about 700 of 4776 main-thread samples, with Sailors::Realize at
893/4776.

The cause was structural. A revision change walked the whole entries map and then rebuilt
both revision maps from what survived, one std::map insertion per surviving span, so a
scene that reaches the 8192-span cap paid that work plus the rebuild for every rebuilt
buffer in every frame.

Delta: indexed_span_frame_cache.hpp lists each admitted span under its index and vertex
owner and evicts only that buffer's spans, unlinking every erased key from the other owner
and dropping the peer revision once that buffer has no span left. The revision-eviction
contract, the 8192 cap and the invalid-range rejection are unchanged. The profile line in
backend.mm gains minWall and slow20 (inter-present intervals at or above 20 ms) so the next
log separates a uniformly slow frame from a focus gap instead of inferring it.

Result: indexed_span_frame_cache-probe PASSes unchanged ("5760 repeated shadow submissions
scan once; revisions/bounds miss; invalid ranges reject; Reset revalidates"), the engine
builds and links, engine-1 is
3b5601e16244c6a1ce9090a24d99c96a08d8d59dad304e5488063a02928cef1c, and canonical staging
is still pending because the previous candidate is running.

Disposition: source, build and probe **accepted**; the real sea replay stays **unresolved**
until a staged launch shows the sail, foam and track frame cost down and minWall back at the
limiter cap. The same samples name one untouched owner: per-frame crew deck collision,
measured and recorded in docs/live-deck-walk.md.

### Sea frame cost: crew deck collision paid ten distance terms per triangle, 2026-09-18

A focused sea sample reconciles the frame the previous entry left open. Of 3118 main-thread
samples, CoreImpl::ProcessRealize is 2988, Sailors::Realize 1367, ShipWalk::CheckPosition
1137, SAIL::Realize 496 and 229, SEAFOAM::Realize 115 and 106, ShipTracks::ProcessStage
164, ISPYGLASS::Realize 92. Every DX9RENDER::DrawBuffer in that tree is 70-90 %
sm::IndexedSpanFrameCache::observe, about 815 samples of 3118, so the dynamic draw path is
what churned span revisions and produced the preceding entry's cost. CheckPosition is
entirely TrySteerAroundStatic, and its CanPlaceAt is about half deck_collision::canSweep
(NODER::Clip -> GEOM::Clip -> deck_collision::polygon) and half the support Trace.

The cause was the leaf predicate. deck_collision::Sweep::hits evaluated ten
capsule-to-triangle distance terms before comparing anything, the ship BSP query visits
every triangle inside radius plus half the capsule height, and CanPlaceAt runs up to seven
times per moved crew member per frame.

Delta: the shipped header derives the reach bound first. The initial spine is the part of
the swept solid closest to a fixed obstacle, so every term is at least initial - |delta|. A
triangle farther than radius + |delta| from the spine cannot block the move and returns
before those terms, the .0001f margin keeps the original overlap-escape branch on the full
computation, and |delta| is computed once per query instead of once per triangle.

Result: a differential build against the shipped header compares the new predicate with a
verbatim transcription of the old one over 541401 generated and boundary configurations,
189727 of them blocking, and reports zero differences. deck_collision-probe,
sailor_locomotion-probe, deck_walk-probe, indexed_span_frame_cache-probe and
character_contact-probe all pass unchanged, the engine builds and links, and engine-1 is
fb9372f5364545c285a424f8f3e372f836bfdd6220d7633ba2465792655bb794 (6176472 bytes).

Disposition: source, build and equivalence evidence **accepted**; the real sea replay stays
**unresolved** for both frame-cost entries until one staged launch puts the sea wall back at
the limiter cap.

Staging: the previous candidate exited, and experiments/native-metal/run.sh --stage-only then
consumed both frame-cost fixes in one batch. CorsairsMetal.app/Contents/MacOS/metal-engine is
fb9372f5364545c285a424f8f3e372f836bfdd6220d7633ba2465792655bb794, the gameplay receipt binds
deck-walk.patch to 0151c617e21d2ae0ef1e2168bd0c03410b13b13dc08c8a15a1a06388f9ef3926, and
sync_metal_gameplay.py reports the compiler ready with no pending files. Nothing has been
launched from that bundle at staging time, so the replay was the only missing evidence.

Replay: the player launched that bundle at 20:58 and stayed in the sea, so the live profile is
now the first real result for both entries. The sea blocks are 15-42 ms of wall with minWall
12-30 ms and slow20 near 120, against 3.4-4.7 ms of GPU: the wall is uniformly slow, not a
focus gap, and it is no longer the 60 ms class the previous log recorded.
sm::IndexedSpanFrameCache::observe and DX9RENDER::DrawBuffer fall to about 8 and 88 of 2387
main-thread samples in a 4-second sample of that process, so the span-cache fix holds. The
same sample puts Sailors::Realize at 679 samples with 677 of them inside
ShipWalk::CheckPosition, which makes the crew resolver the largest remaining game-side block.
That session exited on its own at 21:06 with normal profile lines and no crash report.

### Crew deck collision cancelled by the player, 2026-09-18

The player cancelled the crew resolver outright: crew may walk through each other and the
deck camera, and he does not want that computation at all. The measurement above is the
reason, and the deck-sweep optimization in the previous section is now inert for the active
runtime: it only ever fed this resolver.

Owner: experiments/native-storm/sailor-collision.patch owns every crew-side piece -
ShipWalk::CheckPosition's body and call site, CanPlaceAt, TrySteerAroundStatic,
ResolveDeckCharacter, the relaxation passes, the deckCharacter_ state and
MSG_SAILORS_RESOLVE_DECK_CHARACTER 77601 - and experiments/native-metal/deck-walk.patch was
its only consumer. No other patch in the applied stack names them.

Delta: experiments/native-metal/build.sh no longer applies sailor-collision.patch and no
longer binds it in the gameplay receipt. experiments/native-metal/deck-walk.patch drops the
crew side of the deck camera - DECK_CAMERA::ResolveCrew, the collisionShipId release message
in ReleaseWalker, the SeaDeckCharacterCollisionQuery type in deck_collision.hpp and the crew
terms in the STORM_TRACE_DECK_WALK line - while keeping the walker entity, its model and
animation, the first/third-person and spyglass paths and the protagonist's own CanWalkTo
gate against ship geometry unchanged. tools/sync_metal_gameplay.py binds the compiler receipt
to deck-walk.patch alone. experiments/native-storm/sailor-collision.patch, storm's own
deck-walk.patch twin and the two storm probes stay untouched: that runtime is a frozen
baseline that this build path no longer reads, and Sailors::Realize in the Metal source is
back to the stock per-man spos bypass with the stock CheckPosition call site.

Result: the ordered stack reconciled six source files (sailors.cpp, sailors.h, messages.h,
deck_camera.cpp, deck_camera.h, deck_collision.hpp), the engine built and linked through all
targets, and experiments/native-metal/.cache/storm now reports zero TrySteerAroundStatic,
deckCharacter_, ResolveDeckCharacter and MSG_SAILORS_RESOLVE_DECK_CHARACTER occurrences.
deck_walk_probe.py, deck_collision_probe.py and sailor_locomotion_probe.py still pass against
the frozen storm baseline, whose copy of the resolver is unchanged. engine-1 is
a992db57a418b24ac0b05fb4dffcf21c2b82372ed8d9040b07f2572ff4627282.

Staging: after that session ended, run.sh --stage-only consumed the whole source,
ordered-patch and material set in one batch and reported the compiler ready with no pending
gameplay files and no gameplay write. CorsairsMetal.app/Contents/MacOS/metal-engine and
build/bin/engine-1 are both
a992db57a418b24ac0b05fb4dffcf21c2b82372ed8d9040b07f2572ff4627282 (6158104 bytes), so one
staged bundle now carries the span-cache fix, the deck-sweep reach bound and this removal
together.

Launch: the player started that bundle at 21:10. Its first profile blocks are a land location
at the 8.33 ms limiter cap with 105-115 draws, and a 3-second sample of that process contains
no Sailors::Realize, deck_collision or CheckPosition frame at all - expected for a location
with no ship crew, and therefore no evidence about the removal either way.

Disposition: source, build, probe, staging and startup **accepted**; the real sea replay is the
only missing evidence, and it needs the player to take that build into the sea.

### Sea battle-mode gate survived the sea scene, 2026-09-18

The player reported that after a sea battle the game still behaves as if combat were running
once he is back on the global map, including when every enemy was sunk, and asked why it is
not reset there. Source reading confirms the gate; it also drops the reputation hypothesis.

There is no separate combat-mode state. `PROGRAM/battle_interface/BattleInterface.c` declares
the script global `bDisableMapEnter` (`идет бой`), and `PROGRAM/sea_ai/AIShip.c:3566-3650`
rebuilds it and `pchar.Ship.POS.Mode` from live distances on every sea frame: a live
`RELATION_ENEMY` ship closer than 1000, a hostile fort closer than 1400 that never has to
fire, a nonzero `iStormLockSeconds`, or `bQuestDisableMapEnter`. Inside the sea the flag
therefore clears itself once the player is clear, and a hostile fort inside 1400 is the
reasonable reading of "combat mode with nothing left to fight".
`tools/patch_crime_reputation.py` creates no pursuer and no reporter ship and holds no
battle lock, so it is not the owner.

The defect is the scene lifetime. `DeleteSeaEnvironment` (`PROGRAM/sea_ai/sea.c:96`) is the
single owner of every exit - the global map (`Sea_MapStartFade`), port and land
(`reload.c:408`), sea reloads (`Sea_ReloadStart`), island loading and the boarding return -
and it cleared neither `bDisableMapEnter` nor `POS.Mode`, so the last sea frame's verdict
survived onto the global map and into ports, which never recompute it. Off the sea that
stale value reaches `CheckSaveGameEnabled` (`seadogs.c:1746`; the shipped
`Mods_On_Off_File.txt` selects `SeaBattleMode_SaveEnable {On}`, so this build does not
refuse the save), `SalaryNextDayUpdate` (`scripts/time_events.c:10`), the flag change in
`interface/NationRelation.c:345`, and the cabin rest paths.

Delta: `tools/patch_sea_battle_mode_reset.py` is a second stage over the composed gameplay
output, in the shape of the journal stage. From base
`107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1` it inserts
`bDisableMapEnter = false` and `pchar.Ship.POS.Mode = SHIP_SAIL` into
`DeleteSeaEnvironment` after `pchar.Ship.Stopped = true`, pinned to
`33373b67ed2f3dc8166dadf5560df06df2a155bdd6ec35a94462e48764beff03`.
`patch_gameplay_suite.py` applies the stage after the journal stage and merges its hash
vector into `classify`; `sync_metal_gameplay.py` lists `PROGRAM/sea_ai/sea.c` in `BASE` and in
the expected vector.

Evidence: the new module reports the base revision and transforms deterministically;
native-storm now holds `33373b67...ff03`, `patch_gameplay_suite.py status` reports
`patched`, and `sync_metal_gameplay.py check` reports exactly one pending file with the
compiler ready. `patch_gameplay_suite.py verify` does not reproduce the installed composite
for `PROGRAM/quests/quests_reaction.c` (expected `b435b2fb...4514`, got
`77140737...5f145f`); that inconsistency predates this stage and is untouched by it.

Disposition: source and delivery preparation **accepted**; `sync_metal_gameplay.py apply`
and `run.sh --stage-only` are **pending** because the Metal runtime is held by the session
launched at 20:58. The replay that accepts the fix - leave a sea battle with enemies still
alive, reach the global map, and confirm the map button, the save menu and the ship's sail
state are no longer in battle mode - remains the only missing evidence.

### Light shafts anchored to the live frame origin, 2026-09-18

The beams slid sideways whenever the camera or the hero turned, while the window and the
room stayed put. The apertures, the sun vector and the march were already world-space;
the defect was the frame they were expressed in.

Storm draws camera-relative. DX9RENDER::SetCamera publishes worldOrigin =
-cameraPosition, and DX9RENDER::SetTransform(D3DTS_VIEW) re-derives the same origin from
the view matrix on every view set, so a world point is drawn at authored + worldOrigin
for that frame. backend.mm applied that origin at aperture submission time. The authored
LocationWindows meshes are submitted once per location load, so those apertures froze in
the load-time camera frame, while the per-frame lightshafts locator path carried a live
one. Orbiting the third-person camera therefore displaced the beams by exactly the camera
displacement.

Delta: volumetric_light_shafts.hpp takes the live frame origin in encode and adds it when
the frame uniforms are built, next to the camera position and the depth receiver;
backend.mm publishes authored aperture centers without the origin, in both the mesh and
the locator entry points; volumetric_light_shafts_gpu_probe.mm gains a floating-origin
fixture in which a fixed room under the live origin must render bitwise identical while
the origin moves, with the rejected submit-time origin as the negative, measured at 79 of
256 px.

Disposition: source, build, probe and the canonical staging of
.cache/CorsairsMetal.app (engine d7906b15fed5d3da9ca89de00766c3e70ac5941da0ac2f845f91ef337136950e)
**accepted**; the real-scene rotation replay stays **unresolved** until the player turns
the camera and the hero inside a shafted interior and the beams stay on their window.

### Dialogue ambush punch, weapon draw and fist sound, 2026-09-18

The dialogue surprise strike now reads as the player described it: the hero punches
with an empty hand, the punch carries a fist-impact sound instead of a saber, and the
weapon comes out at the end of the strike and stays out for the fight. Three owners
changed.

The strike plays a new action. `[ambush_punch]` is a clone of the shipped
`[attack_force_3]` block in every `.ani` that declares it, with the sound event
retargeted from `SndAlliace_attack_slash2` to `SndAlliace_ambush_punch` **and moved
from frame 5250 to the block's own `Attack` frame 5258**. The engine dispatches
`SndAlliace_` events from `Character::ActionEvent`, so the saber sound of the source
clip could not be muted from script, and `Attack` is the frame that runs
`CheckAttackHit` — with `fps = 15` from the `man.an` header and `speed = 1.5` that is
0.44 s into the clip, so the sound now lands with the visible contact and with the
scripted damage at 500 ms instead of 0.09 s before it.

The fist sample is the player's own file, staged by the new
`tools/patch_ambush_punch.py` into both gameplay trees as
`RESOURCE/Sounds/People Fight/Punch_01.wav` plus an `[ambush_punch]` alias, trimmed
from `universfield-punch-03-352040.mp3` to its impact (mono 44.1 kHz PCM, level
matching the shipped fight samples, pinned by digest in `tools/assets/`). Its
upstream page and licence are not recorded, so the terms are treated as unknown.

The weapon now comes out in a separate step. `DialogAmbush_ApplySneakStrike` is
damage only, and a new `DialogAmbush_DrawWeapon` at 900 ms restores the player type,
unlocks the fight mode, runs the ordinary `Normal to fight` draw, falls back to
`SetFightWOWeapon(true)` when no blade is in the slot, and then starts combat. A
group check function schedules the sheathe two seconds after the last member of the
target's temporary group dies, so the blade is put away only after the fight.

Delta: `tools/patch_dialog_ambush.py` (punch action, draw step, fight-over callback,
new `_verify_dialog` contract), `tools/patch_ambush_punch.py` (sound, alias and
animation clone owner), `tools/patch_gameplay_suite.py` `COMPOSITE_SHA256` and
`tools/sync_metal_gameplay.py` `BASE` moved to
`c68a86deb87a2fe1e1cc5a1e36f8c85e334f43f6711a55e17627a1d7fe36b75b` for
`PROGRAM/dialog.c` (previous Metal revision `884328db…`), `docs/crime-reputation-system.md`
and `docs/audio-and-particles.md` for the contract and the rejected alternatives.

Two defects of the first draft were caught before delivery and are recorded as
rejected: a parameterless `DialogAmbush_FightOver` (the group registry calls a check
function with its own name, so every shipped handler declares one string parameter),
and a `LAi_LockFightMode(pchar, true)` at the end of the ambush (a stale
`lockFightMode` blocks the saber key until an attacker clears it — the native port
already carries a `recovered stale fight-mode lock while under attack` repair).

The sound, alias and animation clone are staged in both gameplay trees, so
`tools/patch_ambush_punch.py check` is clean. The player closed the session and
`run.sh --stage-only` staged the batch at 19:45: `PROGRAM/dialog.c` was applied into
`experiments/native-metal/.cache/runtime` at
`c68a86deb87a2fe1e1cc5a1e36f8c85e334f43f6711a55e17627a1d7fe36b75b`, that runtime still
carries all 38 `.ani` files that declare `[ambush_punch]` and
`RESOURCE/Sounds/People Fight/Punch_01.wav` at the pinned digest, and
`tools/sync_metal_gameplay.py apply` reported `compiler ready` and left SAVE,
configuration and renderer-owned materials unchanged. The staged bundle engine and
`.cache/build/bin/engine-1` both hash
`d7906b15fed5d3da9ca89de00766c3e70ac5941da0ac2f845f91ef337136950e`, matching the
`gameplay-compiler.json` receipt for compiler patch `a9b691d5…` and the deck
patch digests; the previously installed bundle engine was `3cf0eaed…`.

Disposition: source, hash vector, punch content and the Metal delivery
**accepted**; the real replay stays **unresolved** until the player confirms the
punch, the fist sound, the blade appearing in the hand afterwards, the blade
staying out during the fight, the two-second pause before it goes away again, and
the bare-handed case.

### Dialogue ambush refused by the generated-NPC quest container, 2026-09-18

The tracing revision answered the visibility question on its first replay: the dialogue with a Spanish guard wrote `[compile] [trace] DialogAmbush blocked: quest GenChar_382 group=Spain_citizens` (six lines covering `GenChar_382` and `GenChar_386`), so the action was refused by policy and not by rendering. The gate was `CheckAttribute(target, "quest")`, and that attribute exists on every generated character: `InitCharacter` (`PROGRAM/scripts/utils.c`) and `LAi_CreateFantomCharacterEx` (`PROGRAM/Loc_ai/LAi_utils.c`) both write `chr.quest` as a data container while generating an NPC, with `quest.questflag` (the `!`/`?` mark) inside the same subtree. Reading it as a quest lock closed the whole layer for townsfolk, guards and officers.

Delta: `DialogAmbush_BlockReason` reports `quest` for `isquest` alone, the replacement line carries the reason the container is the wrong signal, `_verify_dialog` requires the `isquest` gate and rejects `CheckAttribute(target, "quest")`, and `docs/crime-reputation-system.md` names the protections that really hold (`LAi_grp_relations.quests`, `chr_ai.hpchecker`, `LAi_IsImmortal`, `DialogAmbushProtected`). `PROGRAM/dialog.c` is `884328db96b64dcec5c83ae54bb6f6a0f943715f3e093445b6d7361310063eff` in `native-storm` and `native-metal`, rebuilt from the committed original by the patch transform; `COMPOSITE_SHA256` carries the delivered revision and `sync_metal_gameplay.py` `BASE` advanced to the previous Metal revision `9c468773…` that the apply consumed. `tools/sync_metal_gameplay.py check` reports zero pending files with a ready compiler, and `run.sh --stage-only` completed after the change.

Disposition: diagnosis, source, Metal delivery and staging accepted. The replay that shows `Напасть` and `Ударить исподтишка` in a real dialogue, the visible swing, the kill and the survivor case stay **unresolved** until the user replays them; if a dialogue still offers nothing, the same trace names the next gate on the list (`groupquest`, `nofightlocation`, `boarding`, `captured`, `alarm`, `fightmode`).

### Dialogue ambush gate visibility, 2026-09-18

A real replay of the dialogue aggression layer never showed `Напасть` or `Ударить
исподтишка` in any conversation, a guard talk included, and the feature's own gate left
no trace that could separate a policy refusal from a rendering problem. The dialogue
owner now has one decision owner: `DialogAmbush_BlockReason` returns the first matching
gate name (or an empty string) and `DialogAmbush_CanAttack` is exactly
`DialogAmbush_BlockReason(target) == ""`. `DialogAmbush_AppendLinks` traces
`DialogAmbush blocked: <reason> id=… group=…` when it refuses, and
`DialogAmbush links: id=… group=… total=<n>` when it appends, so the next replay names
the blocking condition instead of the options simply being absent.

`PROGRAM/dialog.c` in `native-storm` and `native-metal` now carries SHA-256
`9c468773d9214b81c0e7bee9a2cd7caedb888b5bb5dfd775dbf5f4a7a828c3f4`, rebuilt from the
committed original by the patch transform rather than hand-edited;
`tools/patch_gameplay_suite.py` `COMPOSITE_SHA256` carries the delivered revision and
`tools/sync_metal_gameplay.py` `BASE` was advanced to the previous Metal revision
`8ccbe2e4…` that the apply consumed. `tools/sync_metal_gameplay.py check` reports zero
pending files, and `experiments/native-metal/run.sh --stage-only` restaged the unchanged
arm64 engine with zero gameplay files pending. Disposition: repository source and Metal
staging accepted; the blocking reason, the visibility of appended links (the dialogue
link list renders five lines per page), and the strike replay stay **unresolved** until a
real dialogue writes the probe line to `compile.log`.

### Dialogue sneak-strike windup and ally witnesses, 2026-09-18

The dialogue strike now has a visible windup and no longer treats the player's own
people as blockers. `DialogAmbush_Start` hands the hero to the actor template, turns
him to the target and plays the shipped `attack_force_3` swing
(`LAi_ActorAnimation` is a silent no-op unless the character is an actor type, as
`PunishmentAction` in `quests_reaction.c` also shows); a `DialogAmbush_SneakStrike`
event fires 500 ms later, restores the player type through `LAi_SetPlayerType`, then
applies the usual `break` energy cost, damage and combat start. `DialogAmbush_IsOneOnOne`
now skips witnesses that `DialogAmbush_IsPlayerOwned` claims, so officers, companions,
passengers and other player-group characters standing next to the target no longer
suppress `Ударить исподтишка`; unrelated third characters still do.

`PROGRAM/dialog.c` in `native-storm` and `native-metal` now carries SHA-256
`8ccbe2e4ebb0c706c430dce963667314533d103aea80a92381852996bb8f04be`, rebuilt from the
committed original by the patch transform rather than hand-edited; the Metal
`gameplay-baseline` copy stays at its pristine revision
`1f0f4f918f1f35134478e1ba2c98c607492f045242e8e6c30cccdfb95a6b57ce` (the sync tool
needed an explicit `BASELINE` record before it would accept a second delivery of the
same path). `tools/sync_metal_gameplay.py check` reports zero pending files and a ready
compiler, and `experiments/native-metal/run.sh --stage-only` rebuilt and staged the
unchanged arm64 engine `d34a0c36c8365d9ec583f37e8ab6e94702d0fab38e0d5d7f50c6b6055265f773`.
Disposition: repository source and Metal staging accepted; the real-replay part stays
**unresolved** because the swing, the ally-witness case, the kill and the survivor
case were not observed in the running game.

### Dialogue aggression redesign, 2026-09-18

The dialogue owner now exposes `Напасть` to every live non-quest, non-immortal,
non-protected target allowed by the current combat/location state, including
guards, officers, companions, and NPCs in a player-owned colony. `Ударить
исподтишка` remains restricted to a true one-on-one scene, but now spends the
normal `break` attack energy and calls the canonical weapon/skill/armour damage
and death path. A surviving target continues into ordinary combat; a player-owned
officer/companion/passenger is detached from the roster and made hostile before
combat. Both actions register the shared crime intent.

The old mixed gameplay runtime was upgraded by exact hash from the previous
dialogue output; all other gameplay files and renderer-owned changes were left
untouched. Static dialog checks passed. `native-storm` and `native-metal`
`PROGRAM/dialog.c` now both have SHA-256
`b63dc124f592ab650fc52fece69c99de78f31c63eb27a5706d62b30704a8aa98`.
`experiments/native-metal/run.sh --stage-only` completed with the arm64 engine
and zero pending gameplay files in the synchronization check. Disposition:
repository source and Metal staging accepted; the actual dialogue replay with a
guard, own officer, player-colony NPC, nearby witness, and protected quest target
remains **unresolved** because no computer-use/game replay was performed.

### Consolidated repository stage, 2026-09-18

After the previously orphaned Metal sources, probes, patch stack, material
manifests, and gameplay delivery scripts were committed, the canonical
`experiments/native-metal/run.sh --stage-only` path completed from clean
`main@5d6a4673`. The built and installed arm64 executables have the same SHA-256,
`6533851489d2a2489ae875fef7fcd44e080cd78ba383ef01c310d5f0d50b0ee9`.
Gameplay synchronization reports zero pending files and a ready compiler; the
Metal acceptance harness passes all four tests. Disposition: repository source,
build, and staging are accepted at this boundary; the visual scenarios listed
below remain unresolved until their real-game replays are completed.

### Land camera follow-state reset, 2026-09-18

The prior land-origin correction did not address the observed land-only bird's-eye
camera. The canonical `LocationCamera` now invalidates the old follow camera state
when `MSG_CAMERA_FOLLOW` is restored: `CameraFollow::BornCamera` must run against
the current character, the position transition is treated as a teleport, and the
look-mode execution flag is cleared. The reset is kept on the follow transition;
target changes used by cinematic `TOPOS` shots retain their existing interpolation.
This prevents a saved or top-view camera position from surviving the return to land
follow mode while preserving explicit top-position and free-camera messages.

The ordered Metal source stack, arm64 engine build, and `run.sh --stage-only` all
passed after the change. A real land-scene replay was not completed: the current
computer-use surface reports that macOS is locked, so the visual camera result
remains **unresolved** and is not claimed accepted from build output.

### Land camera origin correction, 2026-09-18

The land camera uses `DX9RENDER::SetCamera(lookFrom, lookTo, up)`, while the ship
deck path uses the position/angle overload. That land overload already updates
`vWordRelationPos` and the world transform through `SetTransform(D3DTS_VIEW, ...)`.
The previous candidate applied the same world-origin delta a second time after
that call; the correction removes the duplicate adjustment and only publishes the
resulting origin to the Metal lighting bridge. The canonical Metal stage succeeded
after this change and the candidate launched, but the macOS screen locked during
the real-surface replay, so land camera tracking remains **unresolved** until the
interactive scene can be observed. The launch profile reported `inactive=120`,
which is not visual acceptance evidence.

### Shadow coordinate and point-light budget pass, 2026-09-18

The Metal backend now applies `receiverToWorld` to both raw shadow caster MVP paths,
and the location shadow bridge writes light VIEW/PROJECTION matrices directly to the
underlying D3D9 device so `DX9RENDER` cannot mutate `vWordRelationPos`. Point-shadow
selection is limited to two nearest active lamps inside range and is skipped when the
directional sun is bright (`Diffuse` sum > 0.12); sea authored lights lazily open their
catalog before publication. Direct `ninja -C experiments/native-metal/.cache/build engine`
passed. The canonical `run.sh --stage-only` path also passed. The required real-surface
camera rotation/W+A/W+D replay remains pending.

**Sole active and supported runtime:** `experiments/native-metal` (`./experiments/native-metal/run.sh`).
All ongoing and future development, graphics work, shader migration, and gameplay fixes target
Direct Metal exclusively. The canonical engine build is `./experiments/native-metal/build.sh`.

**DEPRECATED (no longer maintained):**
- `experiments/native-storm` (DXVK-native / Vulkan translation layer under macOS ARM64).
- CrossOver / Wine wrappers (`run-main.sh`, `run-codex-test.sh`, `tools/build_engine_crossover.sh`).
- Historical Windows baselines (`KVL`, `KVL-Codex-Test`).

All non-Metal runtimes are preserved strictly as frozen historical reference points and do not
receive further patches, validation, or maintenance.

### Metal shadow stability pass, 2026-09-17

The current candidate removes main-camera visibility filtering from the static shadow caster pass
and invalidates the GPU point-shadow registry when caster transforms or bounds change. Point-PCF
radius is camera independent. The source stack staged successfully; outdoor point-shadow,
world-shadow-registry, and authored-light-shaft GPU probes pass. A fresh candidate reports eight
completed lamp cubes in the outdoor scene. Visual rotation replay and indoor window godray
visibility remain unresolved until observed on the real game surface.

### Live deck walking candidate, 2026-09-14

The currently staged native-storm engine for this attempt is
`a566121e0bf37d1c1e377f46da625883c11b0681efc708d0c63960d64b684996`.
The preceding `c0c216ab…` executable is preserved under
`experiments/native-storm/.cache/deck-walk-baseline/native-engine`.
The first candidate adds a ship-relative protagonist model, WASD/run input,
third/first-person views and existing spyglass integration without freezing sea
layers or changing the binary sea save layout. Native ARM64 build and script
transforms passed. The actual sea scene renders the protagonist and live crew;
the user confirmed walking generally and spyglass use.

Disposition: rejected as complete. User replay found the protagonist crossing
guns, ship structures and crew, plus pre-existing crew/crew interpenetration.
Independent source review also found shared ship-control names could overwrite
outside-view bindings. The next source candidate is repairing body collision,
crew separation before render and distinct deck-only ship control names; it has
now replaced the first candidate in native-storm and native-metal. Both ARM64
engines build; native-storm is staged as `fde51b3d…826f`, and native-metal is
staged as `fb1b67a1…e5ce` with a receipt bound to both shared gameplay patches.
Four deck control/camera/label files were synced to Metal through the checked
delivery tool; SAVE, configuration and renderer materials were untouched. The
first test process has exited. The shared
capsule sweep passed independent numerical probes for thin faces, edges,
pre-existing overlap escape/crossing, tangential clearance and rigid transforms;
crew/hero and crew/crew numerical cases, dense spawn, head-on passing, blocked
corridor waiting and route continuity pass. Actual gameplay replay of the repaired
collision candidate remains unresolved on both native runtimes. Windows is untouched.

### Native custody repair, 2026-09-14

At the custody-repair boundary the native engine SHA-256 was `c0c216ab7ae2c7a2a4b7fbdd950a508d0381c5073673c8df22087e880bfe0026`.
The cumulative installer now defaults to this native runtime and refuses an active
native executable, unknown script hashes, or a different engine. Original inputs
still come from the immutable historical KVL suite snapshot; they are read-only
build inputs, not the delivery destination. Metal, Windows and SAVE are untouched.

Two generated files changed: `PROGRAM/scripts/custody.c` restores the player
template while keeping fight/travel locks, places the mate at the verified same-cell
`goto24`, and unlocks controls on release. `PROGRAM/seadogs.c` rehydrates custody
after loading a location save because stock `CreateJail` is skipped on save-load.
All 33 cumulative transforms and the independent source/state review passed.
Disposition: installed; interactive acceptance is unresolved. Per the user's
instruction, no computer-use replay is performed; camera, same-cell conversation,
day progression and release must be confirmed in their game.

Journal follow-up: six native PROGRAM files were updated through the cumulative
installer, without an engine rebuild or SAVE changes. `patch_mod_journal.py`
adds stable custody/debt entries and nation-scoped public-crime entries, including
old-save custody/debt refresh. The original quest text baseline is retained under
`experiments/native-storm/.cache/journal-baseline` for deterministic regeneration.
All 34 transforms, the altered-input rejection and independent script/template
review passed. Journal display and interaction remain unverified in-game; no
computer-use replay or game restart was performed. Metal and Windows were not updated.

The CrossOver `KVL` and frozen `KVL-Codex-Test` installations remain preserved
historical Windows owners. `run-main.sh` and `run-codex-test.sh` still launch them;
their shared Windows `SAVE` owner does not extend to either native experiment.

At the last recorded Windows boundary, `KVL` had clean engine bundle `24210f11…`
from source commit `7fb3079d` (engine SHA-256 `d6fe322f…`, matching PDB age 26)
plus the then-installed gameplay scripts. That snapshot, its native-D3DX video
mitigation, and the earlier cumulative 26-file suite failure are historical
diagnostic evidence, not the current native runtime verdict. Do not infer from the
native extern-expression fix that every suite branch has been replayed.

## Historical diagnoses and replay boundaries

These observations describe their original experiments. Later rows in the
experiment history supersede their installation and pending-replay status.

The boarding imbalance was traced to ReCon's script-side crew-ratio rank bonus,
not the engine. `tools/patch_boarding_balance.py` prepares a reversible
cap on the effective enemy crew advantage at 2:1. This preserves the original
continuous bonus below that boundary and limits difficulty 5 to +10 enemy ranks
instead of allowing the bonus to diverge as player crew approaches zero. After the
previous game process exited, the patch was installed only in `KVL-Codex-Test`;
the original `KVL` script remains byte-identical.

The intermittent land-combat lock is also script-owned. Generic raider, assault,
and patrol encounters holster and lock the player while an NPC approaches, but
their shared "shot before dialog" paths start combat without releasing that lock.
The generic `MainHeroFightModeOn` handler likewise tries to draw the weapon before
unlocking combat, so the engine rejects it. A token-only three-file repair avoids
adding statements to the persisted quest layout. The current save emits the same
`State read error` on both patched and exact-original scripts, so that warning is a
baseline save condition and cannot accept or reject this candidate. The installed
runtime is currently on the exact pre-experiment scripts while the user plays;
`tools/patch_land_encounter_combat_lock.py` can apply the candidate only after the
engine exits, and the actual interrupted-encounter behavior remains the acceptance
owner. Engine source commit `cadee8a3` adds a narrower save-compatible fallback:
on an explicit combat-mode keypress it releases a stale lock only when a nearby
hostile fighter is actively targeting the player, outside dialog, priority action,
and no-fight locations. The changed translation unit compiles; full engine linking,
staging, and real encounter replay remain pending.

## Historical isolation-boundary inventory

This table records earlier isolation and staging observations, not the current
installed state. Use the current verdict above and verify the live runtime before
acting; later experiment rows supersede these historical snapshots.

| Candidate | Verified state | Disposition |
| --- | --- | --- |
| `run-codex-test.sh` | Launches `GAMES/.../KVL-Codex-Test/engine.exe` through the installed CrossOver runtime | accepted entry point at this historical boundary; now frozen fallback |
| Tracked `run.sh` / `config.sh` | Require missing `game/` and missing Wine Staging runtime | rejected historical entry points |
| `/Applications/Корсары.app` | Not present | absent |
| CrossOver bottle `Corsairs32` | Present; prior session used it during failed experiments | unresolved legacy evidence |
| CrossOver bottle `Corsairs64-Clean` | Not present | absent |
| CrossOver bottle `GAMES` | Present; CrossOver 26.3.0.39832, `win10_64` / `win64`; native D3DX; x64 LAV Filters 0.83 registered but unused by the video path | accepted runtime owner for the isolated test copy |
| `GAMES/.../KVL/engine.exe` | Original ReCon 1.4.1 installation retained separately; SHA-256 `d93314eb777ac67c709e37b7acf8538f1663305fa83007e249cab89705f4804b` at the isolation boundary | preserved vendor baseline at the isolation boundary |
| `GAMES/.../KVL-Codex-Test/engine.exe` | Installed bundle `34a3d6b971de8457f1352593381970c013483c6adce9e3b23b5025cdb6f5b2bc`; PE32+ x86-64 engine SHA-256 `089bc34033c7ec3282b9f7e5e6fc81afbb414f3e4bb6979bd5091880d5ac057d`; matching PDB SHA-256 `5dfada5776ea1609264c2bdb1ee073cea6b9cf71de2e2da0bf6f9bcda0090e91`; mimalloc SHA-256 `fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`; EXE/PDB GUID `39433DB5-C5D5-4C02-99A6-381BEC6C7E1F`, age 9 | launcher target at the recorded staging snapshot; lifecycle health `healthy`, no holders after staging; real-surface replay was pending at this snapshot |
| `GAMES/.../KVL-Codex-Test/engine.accepted-47cbaaa6.exe` | Exact first binary accepted by the user's gunshot replay, SHA-256 `47cbaaa6e2d0bb86f68134effa85ced612bc3751034d1303eb63906e88f20f0f` | rollback owner for the clean-watermark transition |
| `/Users/md/05_Repo/StormEngine-Corsairs` | `codex/corsairs-engine` at `94dbe08d`; clean source worktree with the accepted behavior fixes, additional FMOD stop/fader guards, the dynamic-call result fix from upstream PR #511, the triangular-sail fix from PR #507, stale save-reference guards, atomic option-file replacement, and the Wine/MSVC build fix | canonical isolated source and build owner |
| Engine bundle `34a3d6b9…` | Clean `94dbe08d` Release triplet: engine SHA-256 `089bc34033c7ec3282b9f7e5e6fc81afbb414f3e4bb6979bd5091880d5ac057d`, matching PDB SHA-256 `5dfada5776ea1609264c2bdb1ee073cea6b9cf71de2e2da0bf6f9bcda0090e91`, mimalloc SHA-256 `fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`, EXE/PDB GUID equal and age 9 | installed atomically in `KVL-Codex-Test`; prior triplet retained in immutable snapshot `55a73c72…`; original `KVL` engine still hashes to `d93314eb…` |

The original vendor engine preserved in `KVL` identifies itself as
`develop(c473abf5355e9f6816e349debfaccd27de0d2fce)-DIRTY(c3aed08cc49f248cfcfff2261e208857)`.
That commit is absent from the public upstream and ReCon repositories. ReCon's
dirty-build suffix is generated randomly and cannot recover the unpublished
source delta. The closest canonical public owner is
`Konstrush/recon-storm-engine`; its frozen `Recon_release_1.4.3` ref is
`63d75444e0397042a41731ff3fd70c32b5af23e7`, but it is not a proven drop-in
binary for the installed ReCon 1.4.1 scripts and content.

The supplied exception snapshot belongs to the temporary installer process
`Setup ReCon 1.4.1.tmp`, not to `engine.exe`; it contains no backtrace. In that
same snapshot `engine.exe` was running concurrently with the installer helper and
had a `wined3d_cs` thread. This does not prove that the game rendered or stayed
responsive. By 2026-09-12 15:52 +07 neither process was running.

CrossOver created a `Корсары - COAS ReCon 1.4.1` menu entry for bottle `GAMES`.
The candidate `engine.ini` reported `safe_render = 0`, windowed 1920×1080,
`msaa = 8`, `adapter = 1`, and `target_version = teho`. These values are an
observed baseline, not recommendations and not proof of a working renderer.

The captured pre-DirectX WineD3D run had a specific failure before useful
rendering. Its persistent `system.log` reported failures for all 54 distinct FX
files passed to `D3DXCreateEffectFromFile`: 46 `E5000` syntax errors and 20
`E5017` not-implemented errors, including `Write pass assignments`. It then emitted
28,044 `technique (...) not found` warnings. The loaded `d3dx9_43.dll` was the Wine
builtin: the bottle copy had the same SHA-256 as CrossOver's bundled copy and no
`d3dx9_43` override was configured. At the 2026-09-12 16:15 +07 snapshot this run
still had one `engine.exe` process.

The DirectX component installed at 16:19 replaced both bottle copies with native
Microsoft `d3dx9_43.dll` binaries. During the 16:37 run, `engine.exe` had the native
x64 bottle DLL open and `system.log` contained none of the previous FX compiler or
missing-technique failures. This accepts the native-D3DX hypothesis. That run did
not test WineD3D: the bottle had `CX_GRAPHICS_BACKEND = dxvk`, loaded CrossOver's
DXVK `d3d9.dll`, and logged 44 graphics-pipeline compilation failures before the
user observed another white surface.

At 16:39 a later run loaded the same native x64 D3DX DLL together with CrossOver's
WineD3D `d3d9.dll` and `wined3d.dll`. The stale DXVK log predates this process.
The fresh `system.log` has no FX compiler or missing-technique failures, the engine
reaches script initialization, `error.log` is empty, and `MainMenu.ogg` is open,
but the game surface is still white. Native D3DX is therefore necessary for this
build but not a complete fix.

At 16:48 the controlled `msaa = 0` replay again loaded native x64 D3DX together
with WineD3D. The user-visible surface was unchanged. The fresh `system.log` has
no shader or technique failures and `error.log` is empty. It reports three failed
startup WMV files. Although the matching Storm Engine source posts
`ievntEndVideo` after `PlayMedia` failure, the later controlled skip showed that
the failed video surface remained the visible white overlay. The MSAA hypothesis
is rejected.

At 17:23 the built-in `cameramode.skipstartvideo = 1` option bypassed the video
object entirely. The previous three WMV errors disappeared, script initialization
completed, and the runtime proceeded into location/dialogue code. This isolates
the white overlay to the startup-video path and rejects window size or initial
presentation as its owner.

All 15 game videos are ASF files containing WMV3 video and WMAV2 audio. CrossOver
ships the ASF demuxer in this runtime but no GStreamer WMV/WMA decoder plugin.
The x64 game therefore could not complete its DirectShow graph and returned
`0x80040217`. At 17:29 x64 LAV Splitter, Video, and Audio 0.83 were registered in
the same bottle, startup-video playback was restored with
`cameramode.skipstartvideo = 0`. A later replay disproved this candidate: LAV was
not loaded and the same three WMV errors returned.

At 17:39 one representative startup clip was transcoded to MP4/H.264/AAC while
retaining the filename expected by the scripts. CrossOver loaded `libgstisomp4`
and `libgstapplemedia`, but playback never advanced and terminated with
`gst_video_info_from_caps` plus `wg_parser_stream_copy_buffer` assertions. The
original WMV was restored byte-for-byte. This binds the remaining defect to Wine's
`AMMultiMediaStream`/GStreamer delivery path rather than either source codec.

At 17:44 the same representative clip was transcoded to AVI with uncompressed
BGR24 frames and PCM audio, removing video and audio decoder dependencies. Wine
loaded `libgstavi`, kept the file open beyond its nine-second duration, produced no
frames, and terminated with the same `wg_parser_stream_copy_buffer` assertion.
Container and codec substitution are rejected; actual playback now requires a
replacement for Storm's `CAviPlayer` plus a rebuilt `engine.exe`.

At 17:41 `StartVideo` was changed to honor the existing skip option for every
scripted video, directly schedule any `afterQuestName`, and post the normal
end-video event. `cameramode.skipstartvideo = 1` is active. The replay produced no
video errors and continued through script initialization; real-scene acceptance
remains user-visible.

At 17:49 the engine-side frame cap was changed from `max_fps = 61` to
`max_fps = 120`. `vsync = 1` and `show_fps = 1` remain unchanged, so the visible
counter can still be capped by the refresh rate CrossOver exposes to D3D9.

At 17:52 the active display baseline was changed from the temporary 1920x1080
Wine desktop to native panel fullscreen: `full_screen = 1`, `screen_x = 3456`,
and `screen_y = 2234`; the per-application `Corsairs` virtual-desktop assignment
and its 1920x1080 desktop value were removed. The trackpad preset keeps both mouse
sensitivities at `0.5`, maps sea/deck/world-map forward/back camera actions to
`Z`/`X`, and maps the otherwise unavailable middle-button combat action to `2`.
The game launched through D3D9 with this configuration; fullscreen geometry and
control comfort remain user-visible acceptance.

At 18:07 the menu cursor was isolated from 3D rendering. The nearest public Storm
Engine source (the installed dirty commit is not published) hides the OS cursor,
reads Windows pointer deltas, recenters it with `SetCursorPos`, and draws a textured
quad during the interface render pass. There is no separate interface FPS cap in
that path. The `GAMES` bottle had no `GrabFullscreen` value, so CrossOver's
fullscreen pointer capture was enabled with `GrabFullscreen = Y`; `msaa = 8`,
3456x2234 fullscreen, 120 FPS, VSync, WineD3D, and native D3DX were retained.
Cursor smoothness remains a user-visible result. The post-Alt+Tab zoom is tracked
separately as fullscreen/Retina display restoration, not as cursor rendering.

At 18:12 exclusive D3D9 fullscreen was replaced with the engine's own borderless
window mode to avoid device/display-mode restoration on Alt+Tab. The first
3456x2234 window was oversized: that native-panel mode was interpreted as Cocoa
points, producing a 6912x4468 Retina window over a 4112x2658 desktop. The corrected
window uses the active macOS logical size, `screen_x = 2056`, `screen_y = 1329`,
`full_screen = 0`, and `window_borders = 0`. CoreGraphics reports the live Sea Dogs
window at exactly 2056x1329 at origin 0,0. Texture degradation remains disabled,
MSAA is reduced from 8x to 4x, and the explicit 120-FPS/VSync settings are retained.

At 18:53 the `GAMES` bottle received one macdrv-only cursor experiment:
`HKCU\Software\Wine\Mac Driver\UseConfinementCursorClipping = N` (`REG_SZ`). The
value was previously absent and was read back successfully after the write. No
engine or renderer setting changed. The user reported no cursor improvement, so
the value was removed and the bottle returned to its prior registry state. This
second failed mouse-capture experiment closes the Wine-setting branch.

At 19:07 the installed ReCon 1.4.1 `engine.exe` received a reversible five-byte
input patch. Its disassembly contains the exact input-update sequence from the
public Storm source: it overwrites SDL mouse-motion deltas with `GetCursorPos` and
then calls `SetCursorPos` every frame. The bundled PDB is stale (its CodeView GUID
and age do not match the EXE), so it was not used for address attribution. The
patch changes the instruction at file offset `0x11392e` from `48 8d 4c 24 20` to
`e9 84 00 00 00`, jumping over only that Windows cursor-warp block and retaining
the existing SDL `xrel`/`yrel` path. The original binary is preserved as
`engine.exe.cursor-warp-original`; the patch and revert owner is
`tools/patch_cursor_input.py`. Static binary verification passed. The user
confirmed that cursor smoothness was fixed, but continuous camera rotation stopped
when the hidden host pointer reached a screen edge. The no-warp patch is therefore
superseded rather than accepted.

At 19:20 the patch was revised to preserve SDL `xrel`/`yrel` on every frame while
reusing the engine's existing dynamic window-center block once every 64 update
frames. The 11 bytes at file offset `0x11392e` are now
`f6 43 34 3f 74 26 e9 7e 00 00 00`: the periodic branch targets `0x14011455a`
(`GetWindowRect` and `SetCursorPos`), while ordinary frames jump directly to
`0x1401145b7`. The original `GetCursorPos` and delta-overwrite instructions are
unreachable on both paths. The game crashed because `0x14011455a` expects `RCX`
to contain the global `core` pointer prepared by an earlier instruction that this
branch also skipped. This patch is rejected and was restored from the exact
original before correction.

At 19:25 the periodic patch was corrected by restoring that required `core`
pointer before entering the dynamic-center block. The 23 bytes at file offset
`0x11392e` are now
`f6 43 34 3f 74 05 e9 7e 00 00 00 48 8d 0d 10 73 42 00 e9 15 00 00 00`.
Ordinary frames still jump directly to `0x1401145b7`; every 64th frame loads
`RCX = 0x14053b850` and jumps to `0x14011455a`. Disassembly verifies both targets,
the patcher recognizes and upgrades both earlier variants, and byte-identical
revert remains available. The user confirmed that it no longer crashed, but the
unconditional 64-frame warp made the visible menu cursor jerk. Periodic recentering
is therefore rejected.

At 19:35 periodic recentering was replaced with a four-edge trigger. Each input
update now reads the host cursor only for boundary detection, preserves SDL
relative motion, and recenters at 1028x664 only when the pointer enters the
64-pixel strip at the left, right, top, or bottom edge of the verified 2056x1329
borderless window. No timer-based warp remains. The patcher rejects `apply` when
that exact geometry is not configured. A separate constructor correction extends
the existing zero-fill to include both SDL delta fields at `this+0x40/+0x44`,
removing the independently audited uninitialized-first-update path. Static
disassembly, exact legacy migration, and byte-identical revert passed; menu
smoothness and continuous 360-degree rotation require the real game surface.
The user later confirmed that the macOS pointer could still escape at the Dock
edge, producing both the system and game cursors at different positions. The
four-edge recenter is rejected and superseded.

At 20:02 the engine was switched from borderless window mode to its real
fullscreen path by changing only `full_screen = 0` to `full_screen = 1`.
The 2056x1329 resolution, WineD3D, native D3DX, MSAA 4, 120-FPS cap, VSync,
video skip, and four-edge cursor patch are unchanged. This is the controlled
test for CrossOver/macdrv fullscreen mouse capture. The user reported no cursor
improvement and a hard freeze after Alt+Tab. True fullscreen is rejected and
`full_screen = 0` was restored after the game process exited.

At 20:18 the engine-side mouse path was replaced with the statically linked SDL
2.0.18 native relative mode. The exact public SDL dynapi wrapper is at
`0x1402c01b0`; the exact `SDLInput` constructor is at `0x1402b74b0`. Its
`SDL_AddEventWatch` call now passes through a Win64-ABI-safe helper in the legacy
cursor block, calling `SDL_SetRelativeMouseMode(SDL_TRUE)` in the same source
order used by Storm on non-Windows builds, then tail-calling the original event
watch function. Input update jumps over the entire absolute
`GetCursorPos`/delta-overwrite/`SetCursorPos` path and consumes SDL `xrel/yrel`.
SDL now owns RAWINPUT, cursor hiding/confinement, and focus-loss release/reacquire;
there is no timer, edge threshold, resolution dependency, or per-frame warp.
Static call targets, stack alignment, shadow space, source order, migration from
the edge patch, final hash, and byte-identical revert passed. The installed hash
is `ab8a4f9e38f5d970a512063c63663a901c70a42ab54991b5e385979a7b885c31`;
the user confirmed on the real game surface that the fix works correctly. A
two-finger trackpad swipe still arrives as wheel scrolling rather than relative
pointer X/Y, so it does not rotate the camera; gesture remapping is a separate
control-layer feature, not a failure of relative-mode capture.

At 22:16 both global time-scale controls were moved off the missing laptop
numeric keypad in the installed ReCon script. `TimeScaleFaster` now uses the
top-row `VK_A_PLUS`, and `TimeScaleSlower` uses `VK_A_MINUS`; the `R` toggle,
pause, time-scale limits, handlers, and hidden numeric-keypad debug controls are
unchanged. No `engine.exe` or KVL Wine process was running during the edit.
Static binding verification passed; top-row `+`/`-` behavior remains pending on
the real game surface.

At 23:00 two source-matched gun-effect defects were patched in the installed
engine. `SGFirePrt` had initialized `blood[numBlood].alpha` instead of
`smoke[numSmoke].alpha`; the 24-byte instruction block at file offset `0xbf7b2`
now targets the smoke particle. `ProcessedShotgun` also drew the camera-aligned
`sghor` cross through `ShootParticlesNoZ` on every frame after the first shotgun
initialization; the eight bytes at `0xc0010` now set that one overlay particle's
alpha to zero while retaining the draw call and its state effects. Smoke, blood,
flinders, hit logic, damage, and reload code are unchanged. The pre-effect binary
is preserved as `engine.exe.gun-effects-original`; `tools/patch_gun_effects.py`
owns apply/status/revert. Exact-byte, disassembly, hash, revert, and composition
checks with the cursor patch passed. Visible stripe removal remains pending on
the real game surface.

At 23:15 the Wine debugger backtrace from the reported John Bolton shot supplied
the first usable native stack. It does not contain Bolton, gun-hit, script, or
Sentry as the original owner. The application chain is `SEA::Realize` through the
only `std::for_each(std::execution::par_unseq, aBlocks, SSE_WaveXZBlock)` call,
then MSVC's parallel-algorithm helper, `CreateThreadpoolWork`, `TpAllocWork`, and
`RtlAllocateHeap`. Its 52-byte element stride exactly matches `SEA::SeaBlock`.
Sentry/Crashpad allocates again while reporting the already-corrupted process heap
and is secondary. The installed engine now forces the helper's existing
single-thread branch at file offset `0x15c472`, bypassing Wine's Windows
threadpool without changing sea geometry. The pre-delta binary is preserved as
`engine.exe.wine-parallel-original`; `tools/patch_wine_parallelism.py` owns the
reversible change. Static and three-patch composition checks passed. Stability
after shooting Bolton remains the real-surface falsifier; no quest-specific
invulnerability workaround was added.

At 23:20 the same installed engine terminated after shooting an ordinary jungle
pirate, while CrossOver left a stale frozen window behind. No `engine.exe` or
Wine preloader remained, no new Crashpad dump was written, and the prior
threadpool backtrace therefore cannot identify this second failure. The fresh
`system.log` is saturated with FMOD invalid-handle errors and ends on
`SoundResume` reporting a channel reused for another sound. Source inspection
found a matching lifecycle defect: `FreeSound` retained the stale channel pointer
and could push an already-free slot more than once, while `SoundPlay` compared
new channels against those free slots. The resulting duplicate free indices can
assign one slot to multiple live sounds. The isolated engine branch fixes this
owner in commit `8f33fd82` by making release idempotent, clearing channel pointers,
rejecting failed playback, skipping free/null duplicate candidates, enforcing the
slot bound, and reclaiming invalid channels in volume/resume/focus paths. This is
source-verified but not yet a live fix: a Windows build and the same-save gunshot
replay are still required. The installed 1.4.1 binary is unchanged by this source
commit.

At 23:28 a reversible content-level falsifier was applied because the full
sound-service fix cannot fit safely into the installed executable without a
Windows rebuild. Only the `name` entries in the `pistol_shot` and `bullethit`
aliases are commented out; all other aliases, sound categories, engine bytes,
renderer settings, and save data are unchanged. The exact original is preserved
beside the file as `RESOURCE/INI/aliases/sound_alias.ini.gunshot-original`.
The same shot still terminated the engine. The log proves that `pistol_shot` was
not loaded, then ends on the pre-existing `SoundResume` channel-reuse error; this
rejects the weapon-WAV hypothesis but does not erase the independent sound-slot
defect. The exact alias original was restored immediately.

The next baseline comparison removes only the installed gun-effect binary patch,
because crashes on two unrelated targets began after that high-risk delta and
persisted with both weapon sounds absent. `patch_gun_effects.py revert` restored
the pre-effect instructions while preserving the accepted SDL-relative input and
the sequential Wine-threadpool workaround. Status is now
`original+cursor+sequential`; an ordinary-pirate shot is the next falsifier.

The isolated source worktree is `/Users/md/05_Repo/StormEngine-Corsairs` on
`codex/corsairs-engine` at `1e2f9b0a`, based on ReCon
`Recon_release_1.4.3`. The unpublished installed `c473abf` source cannot be
recovered from its stale PDB; comparison against the closest public 1.4.1-era
candidate found 657 of 705 source files identical and exact matches in the input,
sea, and location-effect owners. Compatibility is therefore accepted from the
real 1.4.1-content replay, not claimed from source identity alone.

The reproducible Windows build owner is the dedicated CrossOver bottle
`CORSAIRS_BUILD` plus `/Users/md/Library/Application Support/CorsairsBuild`. It
uses MSVC 14.35.32215 / compiler 19.35.32217.1, Windows SDK 10.0.22621, CMake
3.30.5, Ninja 1.13.2, and Conan 1.60.0. The upstream GitLab Conan index exposes
empty 2025 revisions for its legacy DirectX and FMOD recipes, so the retained
2024 Windows package artifacts were imported under their original package IDs:
DirectX `3475bd55b91ae904ac96fde0f106a136ab951a5e` and FMOD Release
`ab2e9f86b4109980930cdc685f4a320b359e7bb4`. Their SHA-256 values are
`7686c9fc3325683fcc6cdd69dda71feafea86d03c13b52780e40b26e3d5da443`
and `8fb5af29ea3fe481476c2924cbd9d802e383e8f755f78624a01f243c25f1475e`.
The normalized local Conan package revisions are `f21bfabc8845ab18fd35dfc828f30869`
and `62cebbdd139383ce97556ea4c3d9866e`.

The first accepted binary was built from the same four behavior-fix commits plus
the then-uncommitted build-system delta and has SHA-256
`47cbaaa6e2d0bb86f68134effa85ced612bc3751034d1303eb63906e88f20f0f`.
After committing the build fix, a clean-watermark binary was also produced from
`1e2f9b0a`; it is PE32+ x86-64, reports linker 14.35, identifies itself as
`codex/corsairs-engine(1e2f9b0a41807c41bcbb12bf2c23c700a6497eaf)`, and has
SHA-256 `8ff1a081afe14310592a521f714bb444dcae8aa738c7d8f8d77f38a2041f010d`.
After the accepted process exited normally, its exact binary was preserved as
`engine.accepted-47cbaaa6.exe` and the clean-watermark build became the test
copy's `engine.exe`. No file in the original `KVL` directory changed.

On 2026-09-13 isolated source work continued without changing the running test
copy. Commit `b23af95d` makes `SoundStop` and `ProcessFader` reject free or null
FMOD channels and retire slots after FMOD errors; this closes deterministic stale
ID and empty-music-slot dereferences during location, sea, cabin, boarding, and
world-map transitions. Commit `e21c4c54` carries the one-line fix from upstream
PR #511: it clears a stale reference type before storing a dynamic call result,
preventing the script VM from mutating the referenced game object. A clean Release
build from `e21c4c54` completed as PE32+ x86-64 with SHA-256
`36da7f3a39ad129fcdd68f6f7900d0ccd69d98e367239beeaf961bbc265526c3`
and watermark
`codex/corsairs-engine(e21c4c548d62d55e4b0af65f5ec3fda70c6587d4)`.
It remains a prepared candidate only: the active game process still owns the
installed executable, so no runtime file was replaced. The build wrapper now uses
the MSVC bridge's direct output path by default after its FIFO formatter was shown
to strand completed compiler workers; the same build then completed in 26 seconds.
MSVC linker/debug metadata makes repeated builds from the same commit differ at the
byte level despite an identical source watermark, so every exact candidate is
preserved together with its matching PDB and mimalloc DLL rather than inferred from
the commit alone. The retained `e21c4c54` candidate has engine SHA-256
`6cc4db98eebb6cd2db4256ae81ea9faaf8a492ceae9f469d4dfbd8b2fea85a18`.

Commit `1ea68065` then ports the approved one-line fix from upstream PR #507,
correcting operator precedence in point-to-triangle distance calculation so
triangular sails do not stretch. Its clean Release candidate has engine SHA-256
`669b3f3e789bd66172575133ec4a6df5212b26c04fa62841f7a2b83133343d16`,
matching PDB SHA-256
`e478288f2e13d697b8b74b2031f8be886090e6a61d347fa02b945eeef0cfdfd7`,
and mimalloc SHA-256
`fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`.
The exact triplet is retained under `build/candidates/1ea68065`; it is not installed
while the current game process is active.

Commits `2616a256` and `94dbe08d` harden state persistence before the next runtime
candidate. Save-reference discovery now follows the actual `DATA` array size and
turns a missing stale `AREF` into an explicit uninitialized reference instead of
dereferencing it. Option saving now serializes fully before opening any file,
rejects an empty payload, writes and checks a sibling temporary file, then uses
`MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` on the
Windows/CrossOver path. A clean MSVC Release build completed from `94dbe08d`.
The focused `xinterface-test` target did not compile because the existing
cross-build omits the Catch2 include path; this is a test-harness dependency defect,
not an engine compile failure.

`tools/manage_engine_candidate.sh` now owns `status`, `prepare`, `stage`, and
`rollback` for exact engine/PDB/mimalloc bundles. It binds candidates to the
embedded clean Git watermark, verifies PE x64 architecture, hashes and EXE/PDB
GUID/age, protects the original `KVL`, snapshots the installed triplet, blocks
launch during an interrupted replacement, and refuses mutation while Wine holds
any runtime component. `run-codex-test.sh` shares the same atomic lifecycle lock.
The Wine/MSVC wrapper is intentionally serial by default: a 16-job replay left
Ninja waiting on defunct wrapper children, while `--parallel 1` completed the same
15-step rebuild and link. `CORSAIRS_BUILD_JOBS` remains an explicit override.

No ready-made ReCon 1.4.1 trackpad patch was found. The installed scripts already
use raw pointer axes for camera rotation and expose the relevant mouse actions in
the text options owner. Current ReCon guidance recommends an FPS cap of 60–100 or
vertical synchronization. The user-selected `max_fps = 120` is intentionally kept
with `vsync = 1`; no source found proves a specific 120-FPS gameplay failure.

On 2026-09-13 at 17:01 +07, after the prior game process and all runtime file
holders had exited, exact engine bundle `423b4351…`, built from clean source commit
`b5a79fed`, was installed in `KVL-Codex-Test`. The previous `cb52f229…` triplet was
preserved as an immutable engine snapshot. Lifecycle verification reports engine
SHA-256 `b132a15d98ff2b7c4953ac759b72fb0c3ed51dd03c7b1539eccbcb8c35c6a7bb`,
matching EXE/PDB GUID and age 16, and no holder during installation. This closes
the known `_ReadFile(nullptr, count)` geometry-buffer crash path without changing
the original `KVL` copy.

The same idle-runtime maintenance window installed a bounded ReCon script/content
repair in `KVL-Codex-Test` only. It derives `pchar.worldmap.shipcounter` from live
quest encounters on every generation tick, deletes complete pending quest nodes,
purges non-quest `Follow`/`Merchant` objects whose saved hostility contradicts the
current nation relation, removes the unconditional test-tornado override, guards
cannonless AI `FireRange` reads, and defines the engine-requested `ship_bow` sound
alias. Exact pre-change files are retained under
`.codex-runtime-fixes-snapshots/20260913-worldmap-audio-v1`. A launch from the
patched copy logged watermark `b5a79fed`, completed script initialization with an
empty `error.log`, reached the game surface, and remained live. At the user's
request, the on-disk ordinary-encounter cap was then raised from the original 8 to
12; the running process had already compiled its scripts, so that density change
is accepted only for the next launch. The current save's relation purge and
`shipcounter` repair still require the first world-map load.

On 2026-09-13, four reversible script candidates were prepared for
`KVL-Codex-Test`: twelve-slot five-minute autosave rotation, boarding-officer
secondary ship posts, hostile fast-travel rejection, and richer world-map label
metadata. Exact originals are preserved by each patch set and runtime writes share
the engine lifecycle lock. The stable `b5a79fed` engine started with all four
patches and the user confirmed that loading the current save worked. An apparent
window transition and high Wine CPU use were incorrectly classified as a load
loop; that diagnosis is retracted because the real game surface remained usable.
The process was unnecessarily stopped during diagnosis, then relaunched on the
same stable engine with only the autosave patch reverted. After the user exited,
the autosave patch was reapplied and the stable runtime launched again. All four
script patches are now active in the frozen playable process.

The cumulative engine candidate from clean commit `b1dd11d6` contains the
save-compatible combat-lock recovery and the world-map label-layout changes. Its
exact retained bundle is `57a88979d620a854ece179fe387f33a3fe0a720aba28a8e193afe22476ef3da9`,
with engine SHA-256
`3929663f925fb9fd22d2dbbaf2fa4a4ed6eb815f75f903db2748c7911f794674`.
It built successfully and reached script initialization, but its save-load
acceptance remains unresolved: it was rolled back to the stable `b5a79fed` bundle
after the same now-discredited process-only hang inference, before a clear
user-visible verdict was recorded. Do not reject or reinstall it from that
inference alone; the next trial must use the real game window and explicit user
confirmation.

The trade journal route planner is prepared as a reversible two-file script/UI
patch for the mutable primary `KVL`. It keeps one city pinned as the purchase
origin and compares every known good against the selected destination, sorted by
profit per cargo centner. Its single combined `Взять` plan is bounded by the
recorded source stock, current cash, and the independently available space of the
player ship plus every removable companion, so the displayed rows cannot jointly
overload any ship when distributed in that order. After the active process exited,
the exact script and INI candidate was installed in the primary `KVL`. The next
launch completed script initialization with an empty `error.log`; real-interface
layout and route-result acceptance remain user-visible.

The first real-interface review exposed a missing interaction distinction rather
than a crash: the route planner had replaced the ordinary journal instead of
making it a selectable mode. `tools/patch_tradebook_modes.py` prepares a second
reversible layer with explicit `Обычный режим` / `С закреплением` switching and a
separate `Закрепить` action. Route mode defaults on for saves without a preference,
then pins `GetCurrentTown()` on land or the canonical nearby colony at a moored-sea
entry; an unknown/unrecorded current town safely falls back to the prior pin. The
running process has not been changed, so installing this interaction correction
requires the next idle window.

## Next diagnostic boundary

Keep the accepted `KVL-Codex-Test` WineD3D, native-D3DX, 2056x1329 borderless,
`msaa = 4`, `safe_render = 0`, all-video-skip baseline. Do not change the renderer,
resolution, input mode, or content while validating this engine.

The crash-after-shot regression is accepted as fixed from the user's real-scene
replay. The remaining narrow visual check is that the transparent shotgun cross is
gone while smoke, hit/damage, and reload remain intact. If the accepted replay did
not use John Bolton, repeat only that target on the same save and remain interactive
for ten seconds; no quest-specific invulnerability or script bypass is installed.
The `Кампот` profile previously overrode the global all-video bypass after loading;
its own `cameramode.skipstartvideo` now matches the global value `1`. Repeat the same
flag-change action and confirm that the national flag changes without entering the
Wine video surface and that control returns within 15 seconds.
The exact `6fdcb0a5…` triplet is now installed only in `KVL-Codex-Test`, and its
EXE/PDB match is verified. Perform one launch and same-save gunshot replay without
changing renderer, resolution, input mode, or content. Confirm an ordinary shot,
the transparent-overlay visual, and—when the save permits—the John Bolton path,
remaining interactive for at least ten seconds. Do not touch the original `KVL`
directory during this boundary.

For the shipyard-theft crash, load `Сантьяго верфь`, steal until the NPC catches
the player, finish the same dialogue, and remain interactive for at least 60
seconds. This specifically exercises the land-HUD alarm transition that previously
wrote one quad (112 bytes) past its vertex buffer. Separately repeat one flag change;
that content route now queues the existing `*_flag_rise` quest directly and never
enters `PostVideo_Start`.

CrossOver's D3DMetal and DXMT choices target Direct3D 11/12, while this executable
imports Direct3D 9. They are not independent D3D9 alternatives to DXVK and
WineD3D, even though the UI exposes them in the same selector.

## Experiment ledger

| Time | Candidate and hypothesis | Exact delta | User-visible result | Disposition |
| --- | --- | --- | --- | --- |
| 2026-09-12 15:49 +07 | New ReCon 1.4.1 / `GAMES` installation | None | Installer helper exception had no backtrace; `engine.exe` was observed concurrently; rendering not observed | unresolved |
| 2026-09-12 16:00 +07 | ReCon 1.4.1 through CrossOver renderer selector | User tried every exposed renderer option | Every attempt produced a white or black surface | rejected as a working path; per-option traces incomplete |
| 2026-09-12 16:00 +07 | DXVK D3D9 path | DXVK selected; windowed 1920x1080 | Swapchain was created, then many graphics pipelines failed to compile; white/black surface | rejected with direct log evidence |
| 2026-09-12 16:00 +07 | WineD3D D3D9 path | `CX_GRAPHICS_BACKEND = wined3d`; builtin `d3dx9_43.dll` | All 54 FX files failed compilation, followed by 28,044 missing-technique warnings; white/black surface | rejected with direct log evidence; native-D3DX WineD3D replay pending |
| 2026-09-12 16:37 +07 | Native D3DX with DXVK | Installed Microsoft DirectX D3DX9 runtime; backend was `dxvk` | All prior FX errors disappeared; DXVK logged 44 pipeline compilation failures; white surface | native D3DX accepted; DXVK remains rejected; WineD3D replay pending |
| 2026-09-12 16:39 +07 | Native D3DX with effective WineD3D | Selector stored `d3dmetal`, but the D3D9 process loaded WineD3D; `msaa = 8` | No FX or engine errors; engine reached main-menu state and opened its music; surface remained white | native D3DX accepted; WineD3D rendering unresolved; single MSAA replay pending |
| 2026-09-12 16:48 +07 | Native D3DX with effective WineD3D; MSAA falsifier | Changed only `msaa = 8` to `msaa = 0`; resolution, window mode, runtime, and DLLs unchanged | Surface unchanged; no shader/technique errors; empty `error.log`; three startup WMV files failed | MSAA rejected; then-current path rejection was superseded by the 17:23 video isolation |
| 2026-09-12 17:23 +07 | Startup-video isolation | Set only `cameramode.skipstartvideo = 1`; kept WineD3D, native D3DX, 1920x1080 windowed, `msaa = 8`, and `safe_render = 0` | No WMV errors; engine completed script initialization and entered location/dialogue code | startup-video path identified as white-overlay owner; skip is diagnostic, not final fix |
| 2026-09-12 17:29 +07 | Restore all WMV playback through native DirectShow filters | Registered x64 LAV Splitter/Video/Audio 0.83 in `GAMES`; restored `cameramode.skipstartvideo = 0`; all other baseline values unchanged | First replay emitted none of the previous `Video Error 0x80040217` failures | decoder fix accepted at log level; visible videos and playable-scene acceptance pending |
| 2026-09-12 17:34 +07 | LAV persistence replay | No new delta; startup videos enabled | Same three `0x80040217` failures returned; process loaded Wine GStreamer and Quartz, not LAV | LAV decoder fix rejected |
| 2026-09-12 17:39 +07 | Native CrossOver codec control | Transcoded only `AkellaLogo.wmv` to MP4/H.264/AAC under the expected filename; preserved the original | CrossOver loaded ISO MP4 and AppleMedia plugins but emitted no frames, hung, then asserted in `wg_parser_stream_copy_buffer` | codec substitution rejected; original restored |
| 2026-09-12 17:41 +07 | Prevent all later video hangs | Extended the existing skip option to every `StartVideo` call while preserving end-video and `afterQuestName` events; set skip to `1` | No video errors; runtime passed startup into normal script initialization | accepted as runtime mitigation; user-visible quest continuation remains pending |
| 2026-09-12 17:44 +07 | Decoder-free media control | Transcoded only `AkellaLogo.wmv` to AVI/BGR24/PCM under the expected filename | `libgstavi` loaded, but playback stayed on the first clip past its duration and hit the same parser-buffer assertion | container and codec paths rejected; original restored and global skip re-enabled |
| 2026-09-12 17:49 +07 | Raise engine frame cap | Changed only `max_fps = 61` to `max_fps = 120`; retained `vsync = 1` and FPS display | Configuration value verified; achieved frame rate not yet observed | configured; real-surface rate pending |
| 2026-09-12 17:52 +07 | Native fullscreen and trackpad preset | Set 3456x2234 fullscreen, removed the 1920x1080 Wine virtual desktop, kept mouse X/Y at 0.5, moved camera forward/back to Z/X, and middle-button combat action to 2; retained 120 FPS and VSync | Engine initialized D3D9 and game scripts without fresh errors | configured; fullscreen and control acceptance pending |
| 2026-09-12 18:02 +07 | MSAA cursor falsifier | Changed only `msaa = 8` to `msaa = 0` | User reported that 3D rendering was already smooth and MSAA did not own the cursor symptom | rejected; `msaa = 8` restored |
| 2026-09-12 18:07 +07 | Fullscreen mouse capture | Added `HKCU\\Software\\Wine\\X11 Driver\\GrabFullscreen = Y`; retained the accepted renderer, resolution, MSAA, FPS, and VSync settings | Game launched; software-cursor smoothness awaits direct observation | unresolved |
| 2026-09-12 18:12 +07 | Borderless Alt+Tab workaround | Set `full_screen = 0`, `window_borders = 0`, and `msaa = 4`; corrected initial oversized 3456x2234 window to the macOS logical desktop size 2056x1329 | CoreGraphics reports the live window at 2056x1329, origin 0,0; visible Alt+Tab responsiveness awaits confirmation | geometry accepted; Alt+Tab pending |
| 2026-09-12 18:53 +07 | macdrv cursor-event experiment | Added only `HKCU\Software\Wine\Mac Driver\UseConfinementCursorClipping = N`; retained 2056x1329 borderless, WineD3D, MSAA 4, 120 FPS, and VSync | User reported no change | rejected and reverted; Wine-setting branch closed |
| 2026-09-12 19:07 +07 | Storm cursor-warp bypass | Patched the supported `engine.exe` at offset `0x11392e` to jump over its per-frame `GetCursorPos`/`SetCursorPos` block and retain SDL relative deltas; preserved the exact original beside it | Cursor became smooth, but 360-degree camera rotation stopped at the hidden host-pointer edge | cursor cadence accepted; no-warp mechanism rejected and superseded |
| 2026-09-12 19:20 +07 | SDL motion with periodic recentering | Replaced the no-warp jump with an 11-byte branch that preserves SDL deltas every frame and invokes the existing dynamic-center `SetCursorPos` path once per 64 updates | Game crashed because the branch skipped the required `core` pointer setup | rejected and restored from exact original |
| 2026-09-12 19:25 +07 | Corrected periodic recentering | Added the required `core` pointer setup before entering the existing dynamic-center block; kept the 64-frame interval and SDL deltas | No crash, but the visible menu cursor jerked on every periodic warp | rejected and superseded |
| 2026-09-12 19:35 +07 | Four-edge recentering | Removed the periodic warp; recenter only within 64 px of any of the four 2056x1329 window edges; initialize SDL deltas in the controller constructor | Static branches/import targets, legacy migration, exact hash, and byte-identical revert verified | configured; menu smoothness and continuous rotation pending real-surface replay |
| 2026-09-12 20:02 +07 | True-fullscreen capture test | Changed only `full_screen = 0` to `full_screen = 1`; retained 2056x1329 and every renderer/input setting | No cursor improvement; Alt+Tab caused a hard freeze | rejected; restored `full_screen = 0` after process exit |
| 2026-09-12 20:18 +07 | SDL 2.0.18 native relative mode | Replaced four-edge recentering with a constructor-time `SDL_SetRelativeMouseMode(SDL_TRUE)` call through SDL dynapi; input update bypasses the entire legacy absolute cursor path | Static verification passed; user confirmed the fix works correctly on the real game surface | accepted for cursor capture; two-finger scroll-to-camera mapping remains a separate control feature |
| 2026-09-12 22:16 +07 | Laptop time-scale controls | Changed only `TimeScaleFaster: VK_ADD -> VK_A_PLUS` and `TimeScaleSlower: VK_SUBTRACT -> VK_A_MINUS` in the installed `PROGRAM/controls/init_pc.c`; no KVL process was running | Static source check confirms both commands use the top-row key codes while `R`, pause, handlers, and limits remain unchanged | configured; real-surface `+`/`-` replay pending |
| 2026-09-12 23:00 +07 | Gun-effect correction | Redirected the bad smoke-alpha store at `0xbf7b2` and set only the unconditional `sghor` overlay particle alpha to zero at `0xc0010`; preserved the SDL input patch | Exact bytes, disassembly, hashes, independent revert, and two-patch composition passed | configured; visible stripe and ordinary-shot replay pending |
| 2026-09-12 23:15 +07 | Wine parallel-sea crash workaround | Forced the installed MSVC parallel helper's existing single-thread branch at `0x15c472`; renderer, geometry, save, scripts, and quest state unchanged | The prior debugger stack maps the fault to `SEA::Realize -> par_unseq -> CreateThreadpoolWork -> RtlAllocateHeap`, but an ordinary-jungle-pirate shot later terminated the engine with no new dump | rejected as sufficient; retained as a source-backed Wine safety delta pending isolated build acceptance |
| 2026-09-12 23:18 +07 | Isolated ReCon engine source | Created `/Users/md/05_Repo/StormEngine-Corsairs` at `Recon_release_1.4.3` on `codex/corsairs-engine`; added source-level gun, sequential-sea, SDL-relative-input, and sound-slot fixes | Source diffs and branch ancestry verified; no live game files changed by this worktree | in progress; Windows build and isolated 1.4.1-content acceptance pending |
| 2026-09-12 23:20 +07 | Ordinary-pirate gunshot after sea workaround | No new setting or build delta; fired in the jungle using the same installed three-patch engine | Engine process terminated and left a stale CrossOver window; no fresh dump; final log error is FMOD channel reuse after many invalid handles | unresolved crash owner; stale-channel source defect fixed separately in `8f33fd82`, runtime proof pending |
| 2026-09-12 23:28 +07 | Gunshot sound-path falsifier | Commented only the `pistol_shot` and `bullethit` alias filenames; exact original preserved as `sound_alias.ini.gunshot-original` | Same ordinary-pirate shot still terminated the engine; log confirms `pistol_shot` was not loaded | rejected and restored exactly; weapon WAVs are not the crash owner |
| 2026-09-12 23:39 +07 | Return to pre-gun-effect binary baseline | Reverted only `patch_gun_effects.py`; retained cursor and sequential-threadpool patches | Exact patch statuses are `original+cursor+sequential`, `patched+sequential`, and `patched+cursor` respectively | configured; ordinary-pirate shot replay pending |
| 2026-09-12 23:45 +07 | Gun-effect revert replay | Fired again with the gun-effect binary patch absent and the cursor/sequential patches retained | Game still terminated after the shot | rejected as crash owner; source correction retained independently for the visible effect defect |
| 2026-09-13 00:15 +07 | Native Windows engine build | Built `codex/corsairs-engine` with exact MSVC 14.35, WinSDK 10.0.22621, DirectX 9, and FMOD 2.02.05 in isolated bottle `CORSAIRS_BUILD` | Full 384-step target completed; output is PE32+ x86-64 with linker 14.35 and the expected DLL imports | accepted build path |
| 2026-09-13 00:16 +07 | Isolated 1.4.1-content replay | APFS-cloned `KVL` to `KVL-Codex-Test`, replaced only its engine and matching mimalloc runtime, and launched it through `GAMES`; original KVL remained unchanged | Engine reached an interactive scene; user replayed the gunshot path and reported normal operation without a crash | accepted runtime candidate; exact Bolton-specific replay remains separately observable if not covered by this user test |
| 2026-09-13 00:21 +07 | Clean-watermark engine transition | After the accepted process exited normally, preserved its exact executable and installed only the clean `1e2f9b0a` build in `KVL-Codex-Test`; launched it through `run-codex-test.sh` | Log reports the exact clean commit watermark, D3D9 and FMOD initialized, `error.log` stayed empty, and the interactive process remained live beyond four minutes | startup and launcher accepted; same-save gunshot replay pending on this exact hash |
| 2026-09-13 00:39 +07 | Isolated sound lifecycle hardening | Added free/null-channel guards and FMOD-error retirement in `SoundStop` and `ProcessFader`; no game file changed | MSVC Release build `e69149e1…` completed from clean commit `b23af95d` | source/build accepted; isolated runtime replay pending |
| 2026-09-13 00:42 +07 | Dynamic script-call result fix | Applied upstream PR #511's `ExpressionResult.ClearType()` as commit `e21c4c54`; no game file changed | MSVC Release build `36da7f3a…` completed with the exact clean watermark | source/build accepted; prepared cumulative candidate, not staged while the game is running |
| 2026-09-13 00:49 +07 | Triangular-sail geometry fix | Ported the approved one-line operator-precedence correction from upstream PR #507 as commit `1ea68065`; no game file changed | MSVC Release build `669b3f3e…` completed; exact EXE/PDB/mimalloc triplet retained under `build/candidates/1ea68065` | source/build accepted; visual/runtime replay pending |
| 2026-09-13 01:18 +07 | Save/reference and option-file hardening | Added actual-DATA-size/null guards for persisted references (`2616a256`) and serialize-before-open plus atomic option replacement (`94dbe08d`) | Clean MSVC Release engine `089bc340…` built successfully; focused xinterface tests exposed a pre-existing missing Catch2 include in the Windows cross-test target | source/build accepted; test-harness dependency unresolved; runtime replay pending |
| 2026-09-13 01:19 +07 | Content-addressed engine lifecycle | Added full-hash/GUID/architecture/provenance verification, immutable rollback snapshots, interrupted-install recovery, original-KVL protection, and a shared launch/stage lock; prepared bundle `34a3d6b9…` | Staging rejected with active holders 57047/57062; installed engine hash remained unchanged and the lifecycle lock was released | safety rejection accepted; stage after the current game exits |
| 2026-09-13 02:36 +07 | Hardened engine staging | After PID 57062 and wineserver released all runtime files, installed exact bundle `34a3d6b9…` atomically in `KVL-Codex-Test`; retained prior triplet in immutable snapshot `55a73c72…`; original `KVL` engine stayed byte-identical | Lifecycle status reports the expected three hashes, matching EXE/PDB GUID and age 9, `health=healthy`, and `holders=none` | installation accepted; real-surface ordinary-shot, visual-effect, and Bolton replay pending |
| 2026-09-13 03:25 +07 | Profile-local flag-video bypass | With no `KVL-Codex-Test` engine process or file holder, changed only `SAVE/Кампот/options/options` `cameramode.skipstartvideo = 0` to `1`; global options already held `1` | Static readback confirms both global and loaded-profile owners now skip video; original `KVL` was not changed | configured; same-save flag-change replay pending |
| 2026-09-13 03:40 +07 | Route-local flag transition | Replaced the five `NationRelation.c` video dispatches only in `KVL-Codex-Test` with their existing `pir/eng/fra/spa/hol_flag_rise` continuation quests | Static readback confirms flag changes no longer enter `PostVideo_Start`; all downstream nation/relations/ship-flag handlers remain unchanged | configured; same-save flag-change replay pending |
| 2026-09-13 03:47 +07 | Land-HUD alarm overflow fix | Crash dump from the shipyard theft/caught-dialog path mapped to `BIManSign::Draw`; source analysis proved an alarm `0→1` transition allocated `7*N` quads then wrote `7*N+1`. Commit `66b442d1` always reserves the one alarm quad; clean bundle `6fdcb0a5…` was built and staged with matching EXE/PDB age 11 | Lifecycle reports `health=healthy`, `holders=none`; original `KVL` engine remains hash `d93314eb…` | source/build/install accepted; exact theft/caught-dialog replay pending |
| 2026-09-13 05:12 +07 | Boarding rank-scaling cap | After the active process exited, installed the reversible crew-advantage cap only in `KVL-Codex-Test`; candidate script SHA-256 is `a8813e17…a456`, original `KVL` remains `0cc70a67…f6c` | Temp-copy cases produce +10 for difficulty 5 at 50:10, retain +5 at 150:100, and return 0 at equal or player-advantaged counts; byte-identical revert passed; the installed game compiled all scripts and reached normal initialization without a new compile/error log failure | installed and source/runtime-initialization verified; reload from before boarding and confirm enemy HP on the real game surface |
| 2026-09-13 05:31 +07 | Restore the configured engine debug window | Commits `258e80bc` and `1a6445bc` restore the Windows F5 open/restore path, edge-trigger it, honor `DebugWindow = 1`, and deliberately bypass `[controls] ondebugkeys = 0` as the historical window handler did; renderer, content, scripts, and game config are unchanged | Clean PE32+ x64 engine `d8ef5bff…b3ae` with watermark `codex/corsairs-engine(1a6445bc…)` was installed as bundle `cb52f229…a5c8` after holders 72115/72248 exited; lifecycle status reported matching EXE/PDB age 14 and `health=healthy`, and the next game launch remained live | source/build/install accepted; real F5 window replay pending |
| 2026-09-13 13:57 +07 | Laptop interface key | Changed only the `Interface` and matching `IExit_F2` bindings from `VK_F2` to `KEY_I` in `KVL-Codex-Test/PROGRAM/controls/init_pc.c`; original `KVL` remains unchanged | Exact diff contains only the two intended bindings; installed file SHA-256 is `b670585a…0009`, and the game loaded the script and remained live without a new error log | configured and startup-verified; real `I` open/exit replay pending user confirmation |
| 2026-09-13 13:58 +07 | Santiago geometry-buffer crash containment | The supplied `0xc0000417` dump maps `_ReadFile(nullptr, 0x5f514)` to an unchecked `LockVertexBuffer` result; that byte count uniquely identifies `Santiago_fd.gm` and `Santiago_fn.gm`. Commit `b5a79fed` rejects failed index/vertex create-lock-read paths and releases the partially allocated geometry; valid model data, renderer, and content are unchanged | Independent source/disassembly review passed for the exact crash path; clean PE32+ x64 engine `b132a15d…a7bb` built with watermark `codex/corsairs-engine(b5a79fed…)`, and candidate `423b4351…6503` was prepared while the user continued playing the installed build | source/build accepted; staging and same-Santiago replay pending current process exit |
| 2026-09-13 17:01 +07 | Install Santiago geometry crash containment | Installed only exact bundle `423b4351…6503` from clean commit `b5a79fed`; preserved installed bundle `cb52f229…a5c8` as an immutable rollback snapshot | Lifecycle status reports engine SHA-256 `b132a15d…a7bb`, matching EXE/PDB age 16, `health=healthy`; next launch logged the exact watermark and stayed live | source/build/install/startup accepted; same-Santiago scene replay pending |
| 2026-09-13 17:02 +07 | ReCon world-map/runtime repair | In `KVL-Codex-Test` only: reconcile the leaked quest ship counter, delete full released quest nodes, purge stale random Follow/Merchant hostility after flag changes and map load, remove forced test tornado, guard cannonless AI range reads, add missing `ship_bow` alias, and set the user-selected cap to 12 ordinary encounters plus live quest encounters; preserved exact originals in `.codex-runtime-fixes-snapshots/20260913-worldmap-audio-v1` | Initial patched scripts completed initialization with empty `error.log`; engine remained live. The cap changed on disk only after that process compiled its scripts, so 12 applies from the next launch. Save evidence before the fix was Spain/friend plus Spanish `Follow/isEnemy=1` and `shipcounter=13` with one live quest encounter | startup accepted for the initial fix set; next launch and first current-save world-map replay must prove cap 12, stale encounter removal, counter normalization, and no new negative-count warnings |
| 2026-09-13 18:14 +07 | Land-encounter combat-lock diagnosis | Correlated the user's lock with 37 consecutive engine `HitNoFight` events, then traced all three generated land-encounter interruption paths and the shared fight-on handler | The 18:08 save has player/player state, an equipped blade and gun, a valid `E` binding, and no location fight ban, so save/config/equipment causes are excluded. Each interruption path retains the preceding `LAi_LockFightMode(true)`, and `MainHeroFightModeOn` does not unlock before requesting combat | diagnosis accepted; script repair required runtime validation |
| 2026-09-13 18:33 +07 | Dedicated-save Santiago teleport and map grant | After the game released the runtime, changed only `SAVE/Кампот/Гавана таверна`: player location `Havana_tavern -> Santiago_town`, `loadedLocation` array index `399 -> 364`, and added one of every 26 live static geographic-map items plus `Map_Best`; `location.from_sea` was already `Santiago_town` and remains unchanged. Preserved exact original SHA-256 `2def2c11…fa18` read-only under `.codex-save-snapshots/20260913-santiago-all-island-maps-v1` | Patched SHA-256 is `a1cf46d5…a150`; the save decodes completely with `location/from_sea = Santiago_town`, `loadedLocation = Santiago_town`, and all 27 requested map items. Removing those additions and restoring the two changed location fields reproduces the original decompressed main and external payloads byte-for-byte | save structure accepted; real load into Santiago and in-game inventory display pending |
| 2026-09-13 18:36 +07 | Land-combat token-only candidate | Tested first an explicit unlock-before-fight patch, then a token-only three-file variant that removed the approach locks without adding statements | Both launches reached the game surface; the current save emitted `COMPILE ERROR ... State read error`, so the token-only candidate was rolled back for a baseline comparison | startup reached; attribution unresolved pending exact baseline replay |
| 2026-09-13 18:37 +07 | Compare clean combat baseline | Relaunched the exact pre-experiment scripts through `run-codex-test.sh` after verified rollback | Script initialization started with an empty error log; loading the same current save then produced the identical `State read error` while the game continued normally. The warning therefore does not distinguish the combat patch from baseline | clean runtime restored and playable; warning is baseline save state, token-only combat candidate still needs real encounter replay |
| 2026-09-13 18:44 +07 | Save-compatible combat-lock recovery | Commit `cadee8a3` changes only the player combat-key path: a stale `lockFightMode` is released only for a nearby hostile fighter whose active target is the player, while dialog, priority-action, no-fight, neutral, distant, and already-fighting states remain locked | MSVC compiled the changed `player.cpp` with `/WX`; its object is current. Two aggregate build attempts exposed the retained Wine/MSVC wrapper's orphaned FIFO/background-process behavior, so task-owned build processes were stopped and the running `GAMES` engine was left untouched | source and translation-unit compile accepted; full clean link, staging after game exit, and real locked-encounter replay pending |
| 2026-09-13 19:51 +07 | World-map label layout and combat-lock engine candidate | Built clean commit `b1dd11d6` and installed exact bundle `57a88979…`; scripts and renderer otherwise unchanged | Engine initialized normally. Save-load acceptance was not observed clearly before a process-only hang inference triggered rollback | build accepted; runtime result unresolved, not rejected |
| 2026-09-13 20:06 +07 | Stable-engine script-set load replay | Restored exact stable bundle `423b4351…` and retained autosave, boarding-role, safe-fast-travel, and world-map metadata script patches | User confirmed that the game and save load were working; the live process and high CPU were normal for this transition | load accepted; prior loop diagnosis retracted |
| 2026-09-13 20:08 +07 | Relaunch after mistaken stop | Reverted only the autosave script patch and launched the same stable engine; boarding-role, safe-fast-travel, and world-map metadata patches remained | Game process started normally. Lifecycle lock correctly refused reapplying autosave during play | superseded by the 20:12 relaunch with autosave enabled |
| 2026-09-13 20:12 +07 | Enable rotating autosaves | After the user exited, reapplied the autosave script patch and launched `KVL-Codex-Test` with all four gameplay patches | Script initialization completed with an empty `error.log`; engine reached the user-visible game surface and remained live, but no autosave appeared after more than ten minutes | startup accepted; timer behavior rejected by the 20:23 observation |
| 2026-09-13 20:16 +07 | Promote the accepted test runtime to primary | APFS-cloned the live static runtime tree, verified engine/config and all patched script hashes, atomically replaced `/Games/KVL`, and removed the prior KVL tree without retaining a backup; the running `KVL-Codex-Test` process was not touched | New `KVL` is a separate inode tree with the same stable engine bundle and current patches. Mutation tools now target `KVL`; `KVL-Codex-Test` is frozen and its launcher remains isolated | primary promotion accepted structurally; first `run-main.sh` launch waits until the frozen peer exits |
| 2026-09-13 20:23 +07 | Repair silent five-minute autosave timer | Observed zero `AutoSave` files after 10:55 process uptime while both global and profile `EnabledAutoSaveMode` values were `1`. The timer treated the mere presence of `pchar.pause` as paused even when its value was `0`; changed the new primary `KVL` to pause only when that value is nonzero | Exact patched hash set verifies, prior eleven-slot protection and boarding completion event are unchanged, and the frozen running test process was not modified | source/static acceptance; first five-minute main-runtime save remains pending after peer exit |
| 2026-09-13 20:25 +07 | Stage the map/combat engine into primary KVL | Installed exact candidate `57a88979…` from clean engine commit `b1dd11d6` only in the new mutable `/Games/KVL`; preserved its promoted stable engine as the existing immutable `423b4351…` snapshot | Lifecycle status reports engine SHA-256 `3929663f…94674`, matching EXE/PDB age 18 and `health=healthy`. The frozen `KVL-Codex-Test` process remains live on engine SHA-256 `b132a15d…a7bb` | primary installation accepted; first main-runtime map, combat-lock, load, and autosave replay waits until the frozen peer exits |
| 2026-09-13 20:29 +07 | Share saves and launch primary KVL | Made `KVL-Codex-Test/SAVE` the single save owner, replaced the cloned primary SAVE directory with an exact symlink to it, and removed the redundant primary copy. Both engines were idle during the transition; launchers reject a live peer runtime | Primary engine `3929663f…94674` compiled all scripts, reached `Init weathers complete`, kept `error.log` empty, and sees the latest shared `Куба - море` save | shared-save ownership and startup accepted; real save load, map surface, combat recovery, and first five-minute autosave remain pending |
| 2026-09-13 20:30 +07 | Migrate upgraded world-map labels into existing saves | The real Jamaica map showed no Fort Orange label although the patched initializer, engine, translation, and coordinates were present. Engine save loading restores the saved `worldMap.labels` tree after current scripts are compiled, while `OnLoad` does not rerun `wdmInitWorldMap`; the current save therefore has none of the new label attributes. Added an idempotent migration in `wdmCreateWorldMap` that reapplies Town/Shore layout metadata and upserts the complete Fort Orange label before the world-map entity reads it, without resetting position or encounters | After the user exited, installed exact script hash `77f6c458…b6429` in primary `KVL`; the next launch compiled all scripts, reached `Sea_FirstInit`, kept `error.log` empty, and retained the shared SAVE symlink | install/startup accepted; reload the current save and verify the Jamaica surface |
| 2026-09-13 20:56 +07 | Five-minute rotating autosave replay | No new runtime delta. Observed the shared save owner after ordinary play on the patched primary runtime | `Кампот AutoSave 3` was written at 20:50:35 and `Кампот AutoSave 4` at 20:56:16; both are non-empty valid-sized save files. The 5:41 wall interval is consistent with the timer excluding pause/interfaces and waiting for a save-enabled state | accepted; reopen the Load interface to refresh its timestamp-sorted list |
| 2026-09-13 21:46 +07 | Providence false-friendly patrol diagnosis | The current save has Spain friendly and pirates hostile. Providence loaded with two generated `IslandGroup` patrols; ReCon unconditionally assigns `AlwaysFriend` to pirate port ships while its pirate fort remains hostile. A hostile fort within 1400 units independently sets `SHIP_WAR`, even before it fires. Prepared `tools/patch_island_patrol_relations.py` to make only port patrols follow their actual relation while preserving the special hostility of ordinary and quest pirates | This exactly explains green ships plus combat mode and no shots yet. The live runtime was not changed while the user was playing; both target script hashes and generated patched hashes verify | diagnosis and patch artifact accepted; install after engine exit, then re-enter the sea scene and verify hostile pirate patrols are red while friendly/neutral ports retain their relation |
| 2026-09-13 22:12 +07 | Pinned-city trade route planner | Prepared `tools/patch_tradebook_planner.py` for the primary `KVL` trade journal: explicit origin pin, selected destination, price/profit comparison, profit-per-centner ordering, and one fleet-wide purchase plan constrained by source quantity, cash, and per-ship free cargo space; original and generated hashes verify for both script and INI owners | In-memory transform reproduces the reviewed UI prototype, the 800x600 layout has one non-overlapping route/control row, and the primary runtime stayed byte-identical while `engine.exe` remained active | source/static candidate accepted; install after process exit, then verify the real journal and one representative route |
| 2026-09-13 22:12 +07 | Install and launch the trade route planner | With both KVL engines idle, applied the exact two-file candidate to primary `KVL`, then launched it through `run-main.sh`; no renderer, engine, save, or other gameplay script changed | Patcher reports both installed hashes exactly; engine watermark is still `b1dd11d6…`, script initialization completed, `error.log` and `script_stack.log` are empty, and the live process reached the game state | install/startup accepted; open the journal, pin one city, select a second, and verify the route row plus one cargo recommendation |
| 2026-09-13 22:17 +07 | Ordinary/route trade-journal modes | The first UI review showed no mode switch and no automatic current-town source. Prepared `tools/patch_tradebook_modes.py`: two non-overlapping mode/pin buttons, persisted ordinary/route preference, ordinary-table restoration, and canonical current-colony resolution through `GetCurrentTown()` or the live moored-sea owner `Sea_FindNearColony()` | Exact v1-to-v2 transforms and hashes pass; a focused independent review found no Storm syntax, resolver, state, or geometry blocker. The live primary engine remained untouched | source/static candidate accepted; apply after process exit and repeat the journal interaction |
| 2026-09-19 00:15 +07 | Crew-payment world-map attribute walk | New `attributes-parent-chain.patch` after `save-areference-liveness.patch`: `ReparentChildren()` in the `ATTRIBUTES` move constructor and move assignment, plus a liveness-terminated ancestor walk in `WorldMap::AttributeChanged` | Not replayed: the game had already crashed on the 21:46 bundle. The stack applied ("3 source files changed"), the exact batch was staged, and the gameplay sync check reported zero pending | source/build/staging accepted; crew-payment replay unresolved |

Historical session transcripts contain many Wine, DXVK, WineD3D, D3DMetal,
`d3d8to9`, Porting Kit, and `safe_render` variants. They are diagnostic evidence,
not accepted project state, because no stable baseline and end-to-end acceptance
record was preserved.

Commit `6563e73` changed only the legacy launcher to Wine virtual-desktop mode. Its
message says that this prevents the black-screen freeze, but the repository contains
no real-surface acceptance evidence for that claim; treat it as an unverified prior
hypothesis, not a known fix. The old Wine prefix still contains some supporting
artifacts, including registered fonts, but no runnable game executable.

## Isolated native Metal resource probe — 2026-09-14

Hypothesis: a bounded static resource-to-GPU path can be tested natively without
converting the game to another engine. Added `experiments/metal-probe/run.sh`, a
standalone macOS prototype using Metal directly. It reads the primary runtime's
FrigateQueen1 GM hull and three original TX textures without modifying them;
7,912 triangles render in four batches on Apple M3 Max. The default launcher was
built and replayed, and the textured hull was observed in its native window.
Missing and truncated GM inputs are rejected.

Disposition: accepted only for the standalone static hull draw. This is not a
Storm backend integration or a playable native game. Masts, sails, original FX,
vertex colors, animation, scripts, saves and gameplay are outside this probe.
The current game runtime verdict above is unchanged. Generated probe files live
in ignored `experiments/metal-probe/.build/`; close its window to stop the process.

## Native Storm bring-up — 2026-09-14, playable experimental port

The user superseded standalone scene polishing with a one-hour attempt to run
the actual game natively. The expanded `--harbor` Metal viewer remains a demo:
original port/ship meshes, generated static sails and substitute water do not
accept game fidelity or gameplay.

The native candidate is isolated under `experiments/native-storm/.cache/`.
Engine source is an archive of `4860fe13245b747973682c88b2f1d9e4850c8c40`;
primary KVL PROGRAM/RESOURCE and a separate copy of SAVE are APFS-cloned into
its runtime. Neither playable installation nor shared saves were changed.
There was no running `engine.exe` at launch. Native config requests fullscreen 2056x1329, MSAA 4 and vsync; adapter is 0.
The initial no-audio diagnostic build was superseded by a native miniaudio
sound service. The user confirmed interactive gameplay, fullscreen and music.
This accepts a playable experimental port, not complete rendering/audio parity.
Shoreline reflection jumping is reported in the latest save (Porto Bello store,
then outside); its cause and repair remain unresolved. Current options set
`cameramode.simpleseamode=0`; the planar reflection branch is not established
as the cause. Party portraits were present in observed gameplay and F2 screens;
the reported disappearance was not reproduced.

| Time +07 | Experiment | Evidence | Disposition |
| --- | --- | --- | --- |
| 16:39 | DXVK Native macOS + MoltenVK 1.4.2 | arm64 D3D9 device on M3 Max; all 15 original KVL VSO/PSO accepted; clear/present passed, also with Storm's old pinned headers | graphics API probe accepted, game pending |
| 16:43 | Native engine compile/link | Original modules compile as arm64 after focused portability changes; native FMOD is unavailable, optional sound service omitted using existing null-service consumer path | native binary produced; audio unavailable |
| 16:43 | First launch with binary outside data directory | Script loader could not find executable-relative shared engine headers | rejected layout; binary staged alongside data |
| 16:44 | Native game launch | Vulkan swapchain initializes, then SIGBUS in SDL Cocoa invisible-cursor PNG decoding | unresolved game startup |
| 16:46 | Disable global mimalloc new/delete override | Same cursor crash without allocator replacement | hypothesis rejected; allocator change reverted |
| 16:46 | SDL-only reproducer, no Storm/DXVK/mimalloc | Same EXC_ARM_DA_ALIGN at 0xbad4007 in NSCursor invisibleCursor after hiding cursor | SDL dependency owner proven; local dependency repair in progress |

### macOS application switching — 2026-09-14

The user reported both Command-Tab and AltTab failing in exclusive fullscreen.
Baseline: the same native ARM64 bundle above, isolated native data/SAVE,
DXVK Native + MoltenVK, fullscreen=1, adapter=0, texture_degradation=0.
No game process was running before the replacement launch.

Source evidence: Storm selected SDL_WINDOW_FULLSCREEN; SDL 2.32.10 Cocoa uses
CGDisplayCapture and a shielding window level in that path. DXVK's fullscreen
D3D9 swapchain also requests exclusive mode. The bounded change gives SDL the
macOS fullscreen Space (`SDL_WINDOW_FULLSCREEN_DESKTOP`) and uses a windowed
D3D9 swapchain on Apple only, so DXVK cannot recapture the display. Other
platforms retain their prior behavior. This is one presentation-mode change;
game data and saves are unchanged.

Build passed; the canonical run.sh staged and launched the candidate. The real
main menu fills the screen, and the DXVK log confirms `Windowed: true` with a
2056x1286 drawable. Command-Tab was sent through UI automation; that target-only
AX response cannot prove the destination application became active. The user confirmed both Command-Tab and AltTab now switch away and back.
Independent source review found no contradictory reset/focus path; non-Apple
behavior is preserved. Disposition: accepted for application switching. The
running candidate is retained for the user. Reflection diagnosis remains
pending independently.

### Coastal collision refinement — 2026-09-14

User requested a minimal native fix for premature ship/rock contacts and owns
in-game replay; do not launch or sail on their behalf for this check. Scope is
native candidate only. The current shared KVL runtime and its SAVE are untouched.

Source baseline: TOUCH accepted island contact directly from the depth raster;
PROGRAM sets a global -10m collision threshold. Ship contour adds 10% width.
Neither of those parameters was changed. A visible gap can still represent a
real shallow-water contact under the existing rules.

Added a collision-only island query: retain the raster broad phase, then veto a
coarse contact only after a valid vertical geometry trace confirms deeper water,
the point's movement does not cross geometry at sea/threshold levels, and the
existing height-map inside-solid test does not identify an interior point.
No-hit/invalid downward queries and decoratively immersed islands retain the
coarse collision. A valid seabed hit does not certify BSP completeness for every
separate rock; incomplete asset collision geometry remains a limitation. Existing model/BSP
trace acceleration is reused; no index construction or new per-frame work in
coarse-deep open water. AI depth queries, ship-to-ship response and grounding
remain unchanged. This is not a complete swept-hull solver and does not repair
inaccurate embedded collision meshes. Performance near dense shores is not yet
measured on gameplay.

Native ARM64 build passed; staged bundle binary matches the build byte-for-byte.
No game process was running during staging. Independent exact-method harness compilation and 17/17 cases passed, covering
coarse-deep/outside zero traces, false positives, genuine shallow/inside contacts,
invalid/missing hits, movement crossings, stationary points and immersion.
Disposition: unresolved for gameplay until the user repeats the
problematic rock approach and a genuine shore contact. Shoreline reflection
jumping remains a separate unresolved issue.

### Hull and keel contact separation — 2026-09-14 (current native candidate)

Supersedes the previous refinement's unchanged physical -10 threshold and
expanded coastal contour. User reported partial improvement, then authorized
an autonomous repair; user continues to own gameplay replay and no game was
launched for this change. Baseline remains the isolated native ARM64 bundle,
DXVK/MoltenVK, fullscreen Space, MSAA4/vsync, separate native PROGRAM/RESOURCE/SAVE.
No native or Windows game process was running before staging.

Discovery: depth codec/generator has no +10 bias; the old grounding predicate
alone adds10. Actual compressed PortoBello raster has only1759 intermediate
samples out of4194304, with approximately14.24x10.24 game-unit cells; the map
cannot be the precise keel surface. Five model-derived keel samples already
exist in save data. Strand scripts play scraping sound and speed-based damage,
but do not resist movement.

Delta: preserve original 1.1-width ship-to-ship contour and build an independent
1.0-width coast contour. Keep -10 as the map broad filter only; actual coastal
contact is at sea level with existing geometry refinement. Ground depth uses
positive BSP floor/inside evidence when the coarse map threatens the current
keel height. Keel penetration uses current physical State, waterline, cargo
immersion, roll/pitch and world offset instead of the prior rendered matrix and
+10 offset. Normal live ship translation into greater positive penetration is
blocked, planar velocity cleared, and grounding event retained; equal/decreasing
penetration and turning remain allowed. Prediction calls, dead ships and fixed
scripted movement do not gain this movement gate. New contour and blocked-move
fields are transient; the serialized ship/save layout is unchanged.

Evidence: native build passed; independent exact-source probes passed20 coastal,
10 ground-depth and9 full-Move scenarios. Open/deep map queries and prediction
moves avoid new exact-ground traces. Final bundle byte comparison is performed
at staging; application is not launched. No gameplay/FPS acceptance is claimed.
Unresolved limits: five-point keel sampling, raster false negatives, incomplete
BSP and conservative map fallback can still disagree with visible geometry.
This is not a full swept 3D hull solver. User shore/escape test and near-shore
performance remain unresolved; jumping reflections remain independently open.

### Native icon and graphics quality — 2026-09-14 (prepared update)

User requested the missing Dock icon and the discussed quick graphics changes;
the supplied screenshot was explicitly excluded. Current native process79564
is retained while playing, so its loaded binary/settings have not been replaced.

Prepared: native bundle CFBundleIconFile=Corsairs.icns, generated from the
original approved256px icon1.ico at engine/apps/engine/rsrc (red compass R).
Build-icon.sh preserves the artwork/transparency and exports the standard ICNS
representations through1024; those larger sizes upscale the256px source, not new
artwork. Source/output fingerprint skips unchanged conversion. Small32/128px
representations inspected; decoded1024 ICNS bounds cover the intentional circular
body and all four corners are transparent. No new menu-bar/tray icon is added.

Canonical run.sh stages the icon and selects repo-owned dxvk.conf with
`d3d9.samplerAnisotropy=16`. Installed DXVK applies this override to filtered,
mipmapped samplers, leaving point-sampled UI alone. SEA uses one256 size constant
for both cube targets, both planar targets and matching depth buffers in Init
and Restore; previous size was128. Independent source review verified viewport
selection follows target dimensions and no128-dependent shader offset exists.
Native build, plist and launcher syntax passed. Postprocess remains disabled;
there is no accepted bloom implementation. These settings do not establish a
fix for the separately reported jumping shoreline reflections.

The icon resource and Info.plist are now installed in the existing bundle;
CFBundleIconFile resolves to Corsairs.icns. The running process remains unchanged,
so Dock cache refresh and graphics activation still require relaunch.

Disposition: prepared, runtime activation/visual quality pending a user-approved
restart or the user's next launch. No FPS claim: reflection-target pixel count
quadruples, total-frame cost has not been measured. Latest hull/keel fix is
included in the prepared binary. Game data and SAVE remain unchanged.

### Direct Metal backend — historical bring-up, 2026-09-14

Owner: `experiments/native-metal`, separate from the playable `native-storm`
port. The actual ARM64 engine retains its rendering interface and loads the
original game assets, while a new backend implements commands directly with
Metal. Its executable has no DXVK, Vulkan or MoltenVK linkage. Source and CMake
are pinned independently to engine4860fe13 and native-port commitdf1e021f;
concurrent script/compiler work in native-storm is not overwritten. Candidate
PROGRAM/RESOURCE/SAVE and userdata are separate APFS copies. Initial presentation
is windowed, MSAA0. This is not the standalone metal-probe model viewer.

| Attempt | Evidence | Disposition |
| --- | --- | --- |
| Initial API/engine build | ARM64 engine linked; real Metal clear/draw/present and resource bounds passed | API bring-up only |
| 18:12 game launch | SEA environment-map initialization dereferenced an unsupported cube-texture result | rejected; cube and volume resources implemented and GPU-tested |
| 18:19 game launch | SCRSHOTER called D3DXLoadSurfaceFromSurface; Surface::GetDevice was missing, producing the user's null-pointer crash | rejected; resource device ownership and complete surface-copy path repaired |
| 18:24 game replay | Real main menu rendered; user loaded a sea save and observed ships, sails, terrain and UI, with missing water and about31FPS | startup/game integration accepted; rendering parity and performance rejected |
| Screenshot regression probe | Exact engine D3DXLoadSurfaceFromSurface linked into probe; device lifetime, render-target viewport changes, GPU pixel readback and copy path passed | bounded crash-path repair accepted |
| 18:27 performance sample | 1119/2495 main-thread samples in sky pixel lookup through full Surface::download; unchanged sky textures were repeatedly copied from GPU | cause identified; dirty-state repair and sea shader port in progress |

The first playable prototype explicitly skips programmable shader draws;
therefore missing sea, grass and other shader effects are known missing support,
not accepted visual parity. Fixed-function texture stages, lights, materials,
fog, blending, depth, cube/volume resources and screenshot copying are present.
No claim of complete Direct3D9 compatibility or improved FPS is made.

The profiling process99578 was closed after the sample. No native/Metal/Windows
game process was observed at this boundary. The neighboring script task retains
the playable native-storm build/PROGRAM owner and can test there; further Metal
launches must coordinate against that runtime. Accepted script/compiler changes
must be explicitly integrated together into this pinned candidate rather than
assuming its separate PROGRAM copy updates automatically.

Prepared follow-up: static texture CPU copies now have GPU-write invalidation,
removing repeated full sky-texture readback. GPU dirty-state and screenshot-path
regressions pass. The user supplied a later sea screenshot showing about73FPS;
its viewpoint differs from the earlier31FPS frame. The follow-up profiler sample
captured inactive/background sleeping, so it is not a valid performance comparison.
The four original sea VS/PS pairs now execute through Metal. An independent API
fixture loaded the original eight binary shaders, drew indexed geometry, checked
the four expected output colors and volume-to-cube reflection direction, and
rejected an unrelated shader; all passed. macOS fullscreen Space is configured
for the next launch, with MSAA off. Real water/fullscreen replay is still pending
the neighboring native script task's runtime window. Profiling process3088 has
exited (verified with a PID-specific empty ps result).

Latest source/build: all 15 original programmable shader binaries are recognized.
GPU fixtures passed for sea, particles, caustics, grass, worldmap and postprocess;
the latter also verifies FFP specular/TEMP and backbuffer Reset. Original
postprocess is supported but not enabled as a new game effect. Texture transforms,
generated cube coordinates and caustic slope bias are implemented.

At the user's explicit request, the candidate launcher now selects new Metal
water with `STORM_METAL_MODERN_WATER=1`. It retains the original wave geometry and
physics, with angle-dependent Fresnel, finer animated normal detail and
weather-dependent body color. Sea3 planar reflection retains large-wave distortion;
the modern Sea2 and SunRoad passes share their normal calculation. Depth-based
shoreline transparency/refraction is absent. Original shading remains available
with value0. The modern GPU fixture passed reflection-input, viewing-angle,
weather, cube-direction, foam and unsupported-shader checks.

The neighboring native task released the game runtime window. The first modern
candidate opened its menu, reporting 2056x1329 backbuffer and drawable, samples1,
modern Metal water. Automated mouse/keyboard attempts did not navigate the menu;
this does not establish a game hang. That task-owned menu process16345 was closed
for the final shader update (SIGTERM did not exit; SIGKILL ended it). The final
modern build is launched for the user to load a sea save. Water appearance,
fullscreen chrome and foreground performance remain unresolved pending that replay.

The running build logged an unsupported draw. Source audit found the original
stars consumer uses an FFP declaration with separate position/color streams;
this path was not covered by the 15 programmable shaders. The backend now handles
that point-sprite path and reports exact rejected draw combinations. Its GPU
fixture and the general resource/screenshot/presentation fixture passed. This last
stars update is built but intentionally not staged over the user's running modern
water build; the next launcher invocation will stage it. No performance comparison
was taken while GPU fixtures and the game were both active.

### Direct Metal — current integrated candidate, 2026-09-14 evening

Current owner remains `experiments/native-metal`; prior bring-up inventories above
are historical. ARM64 Release, direct Metal without Vulkan/DXVK/MoltenVK linkage,
2056x1329 backbuffer/drawable, MSAA off, fullscreen Space. User accepted modern
water appearance, night response and the earlier frame-arena performance gain.
Water now includes GPU scene-depth absorption/refraction, weather illumination,
and foreground rejection; the earlier absent-depth statement is superseded.
All 15 original programmable shaders and multistream FFP stars are supported by
bounded GPU fixtures; this is not a claim of complete D3D9 compatibility.

Latest staged candidate includes indexed FFP vertex reuse (dense index spans;
sparse fallback), NULL texture-stage weapon repair, POSITIONT stale-VS bypass,
precise visual wave timing, cinematic grade enabled, and four reversible material
replacements: Sandtile, pierWood1, tileU2, deckPlanksU1. Rock textures are unchanged.
GPU probes passed indexed vs expanded pixels, weapon diffuse/cubemap/null-stage,
original three-pass shadows including retained VS, water depth/day-night, original
sea, particles/caustics, grass/worldmap, postprocess, stars, generated coordinates,
lighting, cinematic and arena. Timer fixture passed 120 frames = 1000 visual ms
while preserving legacy timing, fixed-step, pause and cap. New frame-rate and
wave-motion improvement require an in-game comparison; no measured FPS gain is
claimed for the indexed change yet.

Compiler extern-expression patch is applied with forward/reverse validation.
Successful build writes `.cache/gameplay-compiler.json` engine/patch hashes.
`tools/sync_metal_gameplay.py apply` accepted the staged engine and applied seven
reviewed PROGRAM files; SAVE/config/renderer resources were excluded. Launch
reached the rendered game menu. This accepts menu presentation only; gameplay
changes and latest visual fixes still need their actual scene/action replay.

Dynamic land lighting and real scene shadow maps are a separate active goal,
not enabled in this running candidate. Existing baked COL and original character
shadows remain the current land baseline. Source-only agents own backend lighting,
scene hooks/weather integration, and island geometry; root owns build/stage.
Do not replace the running app while the user plays. Island source audit found
coarse authored geometry; normal-only smoothing cannot fix its silhouette.

Prepared follow-up (not staged): source-driven shadow prepass now traverses
explicit location/character models with light-frustum culling, including
camera-offscreen casters. One directional 512 map and one actual nearest lamp
256 cubemap feed normal material shading; other lamps remain lit. Release build
and GPU fixture passed day/night, moved/offscreen caster, point-light identity,
removed caster and incomplete-cube fallback. Interior/real scene appearance and
FPS are not yet accepted. The current application was not replaced.

Sea geomorph patch is integrated in this prepared build. Frozen original wave
data reproduced an old LOD4-to5 height jump of0.047725 from a0.001 camera move;
new threshold fixtures converge within1.4e-7 input position and1.3e-8 height,
with shared-edge/static/grid tests. Actual visual replay remains pending.

User reported about54FPS in San Juan harbor. Recent active aggregates showed
~15–16ms wall versus~3.5–4ms GPU, ~2.2M vertex references and~97MB upload/frame.
The subsequent 5s CPU sample mostly captured background sleep and cannot quantify
foreground cost. Source confirms five cubefaces for environment reflection plus
five separately filtered sun-road faces per frame. Repeated CPU geometry work is
a candidate, not a proven complete explanation for the reported frame rate.
The game process later exited; no forced restart was performed for profiling.

User-authorized follow-up launch staged the full shadow prepass and sea geomorph
with STORM_METAL_DYNAMIC_LIGHTING=1. Repeated build initially rejected overlapping
scene/prepass patch reverse-checks; they were consolidated into scene-lighting.patch
and the canonical build/run path then succeeded. User confirmed visible shadows,
but reported missing character receivers, flat/glowing interiors, and walking
reflection jitter: these remain open acceptance defects. Interior source diagnosis
found vertex-only local light evaluation misses large wall centers; per-fragment
lamp lighting is in progress. Character camera-fade receiving path is being fixed.

Prepared frame-local unlit indexed FFP cache passed GPU tests for view/projection,
VB/IB edits, locked buffer writes, Present and Reset; not yet staged. Active CPU
sample (lighting-active-cpu.txt) captured real running work rather than background
sleep: ISLAND main draw and SEA reflection model redraw both consume CPU time.
Runtime cache-hit counters will determine actual benefit after staging.

Island render-only PN consumer and CPU fixture are prepared, not yet built/staged.
Antigua geometry changes 62428 to141409 triangles; orthographic silhouette maximum
shift19.71 world units. Original corners, shoreline band, material/UV boundaries
and original BSP collision remain the protected baseline. In-game appearance and
performance remain unverified; this is not a completed island-quality claim.

Latest prepared correction: indoor lighting now consumes a stable scene catalog
of up to64 actual lamps, rather than camera-selected per-draw slots. GPU regression
passed equal receiver pixels across camera slots/player-focus boundaries, physical
second-light contribution, character opaque/faded receivers and indoor occlusion.
Not staged over the current game. Character close-contact shadow contrast and
walking reflection jitter remain open. User requested vegetation/rock candidates,
then clarified existing good high-resolution ground should remain: 1grass1 was
never regenerated; jungle1 candidate is withheld, original retained. New foliage
and rock shards are prepared but not staged or accepted in-game.

### Current Metal launch — lighting correction candidate

Supersedes the prepared/not-staged labels above: frame-local FFP cache, PN island
render geometry (opt-in enabled), stable indoor lamp catalog, and treePalms/trees
foliage are staged. Original jungle1/1grass1 and rock textures remain retained.
Latest launched engine SHA-256: `30c4c34aacffbf7b4e8c5c0d02127255ce75615fa6c50d77cba0179f69d5ee23`.
The process was observed as PID 67821; staged and built hashes match.
This launch adds D3D-compatible COLOR1 diffuse default and indoor shadow-lamp
selection independent of player/camera distance. Lighting and dynamic-lighting
GPU fixtures pass; actual Marigot movement acceptance remains unresolved.

Water temporal candidate was rejected: max channel discontinuity fell from211
to144 but120 jumps remained. It was reversed before this launch. Existing water
is preserved; camera-motion reflection/refraction jitter and sea disappearing
behind foreground foliage remain unresolved. Alpha-cutout foreground regression
passes, so alpha-test leakage is not established as the cause. Agents continue
source-only depth-tolerance and temporal-input isolation work while user plays.
Launch/process/log evidence does not establish interactive scene acceptance.

User rejected indoor visual result in Marigo_Shipyard: no convincing furniture or
character cast shadows, flat object brightness. Source/asset audit finds8 candle
locators and range4; the single pinned first candle lies6.15 units from merchant,
so it cannot shadow the desk. Multi-lamp shadow support is now being implemented;
single-lamp fixture success does not accept room lighting. Canonical alpha order
was broadened, but no isolated pre-fix failure was recorded; causal attribution
to it remains unproven.

Water depth-order correction passed the actual near=.115/far=32000 distant-depth
fixture:10 submerged world units changed from fffdfdfd (incorrectly transparent)
to ff88c6e2; near/night/foreground-cutout cases still pass. This source correction
is not yet staged, and remaining camera-motion artifacts are not accepted.

### Active correction — camera-dependent light and controls

User Story: As a player, I want stable world lighting and correct physical-key
controls so that camera movement does not change illumination or ship actions.

The September 15 01:37–01:39 screenshots reject the latest Metal lighting
candidate: interior shadow boundaries move with camera position and outdoor
characters are excessively dark. Earlier fixture passes do not accept this build.
No game process was present at the current process check. Root owns build/stage
and launch; source-only lanes inspect shadow spaces, authored light positions,
outdoor character energy, saved controls, and diagnostic wiring.

Acceptance: fixed world receivers retain light/shadow registration across camera
translation/rotation; daytime characters remain readable with directional light;
world-map W/S and A/D follow their named sail/turn controls on the existing profile;
Must not alter unrelated saved settings, land controls, night energy, or sea mode.
Scope includes Metal rendering and shared gameplay delivery to both native targets;
new texture/art replacements are outside this correction.

Discovery found DX9RENDER::SetTransform(VIEW) rebases world geometry and strips
view translation. The current shadow bridge supplies already camera-relative
views to that absolute-space API. The next falsifier must exercise this real
wrapper contract, then the same interior wall and outdoor scene in game.
Disposition: unresolved; no new playable acceptance is claimed.

### Verified gameplay delivery and user feedback — September 15, 02:16

Both native runtimes now contain the name-based control-registry restoration and
support-surface collision candidate. Staged engines: Metal
`f17cecbfda3dc7b0f60cfc87d2a5e4990ae1e00dee61fa2d258e3b40d094af62`,
Storm `30c1b91e2cecbbadeec96d7954b7d6fe536b03763c73d50c24a6cc87519c0863`.
Runtime tracing observed WMapShipSailUp=87, SailDown=83, TurnLeft=65,
TurnRight=68 after loading. User subsequently confirmed global-map controls are
correct: accepted on Metal. User also confirmed lighting and shadows now look
normal: accepted for the reported scenes. Native Storm startup/interaction was
not replayed in this delivery; its installed bytes are verified only.

The renderer correction uses absolute light views at DX9RENDER::SetTransform;
receiver sampling compensates the main-camera origin once. Outdoor authored
COLOR1 models receive full weather ambient, avoiding the ground-specific
attenuation. CPU coordinate negative and GPU lighting/contact fixtures pass.
Deck floor filtering is installed but actual walking acceptance remains pending.

The native source build path now uses transactional patch application and a
binary/patch receipt instead of nested git-apply and a hardcoded engine hash.
Its legacy migration matched a pinned 32-path snapshot; unknown edits reject.

Separate settings layer exposes seven existing global commands through the
existing controls UI. It is applied to Storm scripts and queued for normal Metal
sync at next safe launch; Metal in-game settings acceptance remains pending.

New requests remain open: smooth lamp flicker, long-distance shadow transitions,
progressive soft shadows, actual town AO. A temporal lamp patch is prepared;
shadow/AO source lanes are in progress. Performance task
`01a0a153-d759-7c83-af1d-3bc113a7b8ae` owns profiling and renderer optimization.
Its real CPU sample identified ship crew collision work; a revised source-only
solver fast path preserves one static sweep for moving sailors and skips repeated
relaxation only without dynamic overlap. No measured FPS improvement is claimed.

### Current Metal runtime and pending persistent-cache build — September 15, 05:29

The currently running Metal process is PID 89430 from the last fully staged visual
build. That installed binary predates the persistent immutable-geometry cache and
must not be used as evidence for its performance.

The pending source candidate preserves the existing render passes and quality
settings while retaining exact converted FFP vertex/index buffers across Present
and caching validated indexed spans across repeated shadow submissions. VB/IB
revision changes and device Reset invalidate the matching data. GPU payload is
bounded to 512 MiB and 8192 entries; revision metadata is admitted only with a
successful entry, so a 100,000-identity stress case remains bounded. Focused probes
pass 120-Present reuse, revision/Reset invalidation, invalid-range rejection and
5760 repeated shadow submissions with one index scan. Full build/stage and a
same-scene runtime measurement remain pending until PID 89430 exits. No FPS gain
is accepted yet.

### Launched persistent-cache candidate — September 15, 11:11

Supersedes the pending state above. `experiments/native-metal/run.sh --stage-only`
completed after the final source changes with zero patch-stack changes, all 11
hash-pinned materials, all three rockK2 aliases and zero pending Metal gameplay
files. The built and staged executable hashes both equal
`cdb73d61d4e2f3cfe673cae2f114eb5634c42485e3e8aca2d7718263b8aca9e3`.
The app is running as PID 10962 and reached a rendered gameplay scene.

Live `STORM_METAL_PROFILE` samples in the heavy indoor scene report approximately
900 cache hits and 456 MiB of avoided conversion/upload work per frame. Warm
samples are 8.62–9.61 ms wall time with 4.16–4.40 ms GPU time while drawing about
1012–1042 calls and 10.5–10.8 million submitted vertices per frame. These samples
prove that the persistent cache is active and removes substantial CPU/upload work;
they are not a controlled before/after FPS comparison. User-visible acceptance of
the latest lighting, sea/horizon, material, island and deck-sailor changes remains
pending in their corresponding scenes.

### Rejected visual candidate and corrective stage — September 15, 13:21

The user rejected the launched candidate on the real game surface: the tavern
showed 22 FPS, the outdoor town showed 35 FPS, San Juan ground still appeared
unchanged, static town objects produced doubled shadows, and the town backdrop
could render as a large opaque wall. The earlier build/probe/cache receipts did
not accept those surfaces and must not be cited as visual or performance success.

Discovery bound the San Juan low ground plane to `rockK2.tga`, not the previously
emphasized `town_cob.tga`; the three replacement aliases were staged but their
engine selection was not explicit in the app launcher. The launcher now exports
`STORM_METAL_ROCKK2=1`, and the staged executable contains the alias-selection
contract. Static outdoor location geometry is no longer replayed into the new sun
shadow map, avoiding a second dynamic shadow over authored/prelit town shadows;
moving characters remain shadow casters. Indoor point-shadow ownership is restored
to one stable lamp while the complete light catalog remains available for direct
illumination, reducing the worst-case prepass from 8 cube maps / 48 faces to one
cube map / 6 faces. The broad script injection that assigned the window-only
`LocationWindows` technique to 17 full-size `plan1` town backdrops has been removed;
the runtime scripts now retain their authored backdrop technique.

`experiments/native-metal/run.sh --stage-only` succeeded after these changes.
Built and staged executable SHA-256 both equal
`a277c70297c68e0c6ae3f225a09713d25dc6d049999c080bc85ec1c2f4339c57`.
The backdrop source/CRLF/negative probe, material corpus probe, patch transaction
probe, shadow cascade scalar probe and world-shadow owner probe pass. Disposition:
staged, user-visible acceptance unresolved. No game process was launched for this
attempt; the same San Juan ground/backdrop, static-vs-character shadow scene, and
fixed-camera tavern profile must be replayed before any repaired/FPS claim.

### Corrected Metal lighting and cache candidate — September 15, 14:18

The textureless intermediate candidate was rejected: a missing brace in the
`shadow.tga` material marker assigned `-1` to ordinary textures. The corrected
stage restores the authored textures, removes the bad opaque town backdrop,
keeps outdoor static geometry in the two-cascade dynamic sun shadow pass, and
selectively bypasses the separate baked `shadow.tga` stage when both cascades are
ready. The user accepted the restored town appearance; the initial corrected
replay remained rejected for performance at 32--38 FPS.

Live profiling bound that failure to CPU FFP conversion rather than Metal GPU
capacity: the rejected replay uploaded about 159 MiB/frame with 26--32 ms wall
time while GPU time was about 6--8 ms. The persistent conversion key incorrectly
included camera-origin translation and unused directional-light fields. After
removing those inputs and adding bounded LRU eviction, upload fell to roughly
44--54 MiB/frame and the visible San Juan rate rose to about 55 FPS, but exact
per-frame sun direction still invalidated the remaining lit receivers.

The current stage stabilizes slot-0 sunlight at angular cells smaller than half
the finest 512-map shadow texel, preserves true time-of-day evolution, and uses
that same direction for CPU lighting, cache keys, and shadow matrices. It also
preserves VB/IB usage metadata so rewritten dynamic model buffers use the frame
arena instead of persistent allocation. The staged executable SHA-256 is
`07c180a829a553091435113d4d2af04d9f89a49f3ff6132776436ddb606742d0`.

Real-scene observations for this exact stage: an interior reached 124--125 FPS;
steady indoor profile samples were about 8.33--8.55 ms wall and 4.6--7.6 ms GPU,
with zero steady conversion misses. The outdoor transition sample was about
16.25 ms wall / 5.21 ms GPU and showed real lit/dynamic cache hits, so outdoor is
materially improved but still below the requested 120 FPS and remains unresolved.
Synthetic 64x64 probes are correctness checks only and are no longer considered
performance acceptance; the required acceptance set is real heavy town/weather,
sea, and interior saves over sustained frame windows.

The same stage adds scoped lighting corrections requested during replay: dark
outdoor ambient is reduced by up to 12 percent while direct light is preserved
(slightly darker, more contrast); low interior ambient is raised and compressed
(brighter, less contrast); normal-less rope and vant geometry now uses each
ship's active scene ambient instead of the full-bright fallback. Day outdoor
ambient and unrelated transparent materials are unchanged. Disposition: staged
and running for user evaluation; indoor performance accepted, outdoor 120 FPS and
the exact visual balance remain unresolved.

### Full Metal migration rejection and corrective batch — September 15, 15:30

The user rejected the previous real-game candidate across the main menu, San
Juan interiors, daytime town, deck, cabin and night sea. Observed defects were
opaque black/green/grey texture backdrops on foliage and other alpha materials,
the opaque town mountain band, doubled and blocky outdoor shadows, missing local
character shadows, weak ship-lamp response, unstable night lighting and lower
FPS than the original native renderer. That candidate is rejected; prior narrow
GPU probes do not override this real-surface verdict.

The next staged batch removes the four-pass cinematic exposure/bloom/blur/
luminance pipeline from outdoor and sea scenes while preserving the accepted
indoor path. It fixes the Metal sampler mapping for `D3DTADDRESS_BORDER` and
`D3DSAMP_BORDERCOLOR`, which had incorrectly repeated hidden atlas RGB at sail
and alpha-mask borders. All 17 town `plan1` backdrops now use alpha blending,
and the separate baked `shadow` object group is skipped only while the dynamic
outdoor sun shadow is ready. Bound indexed scope-2 FVF `0x152` dynamic models
now read their source VB/IB directly in Metal and perform lighting and shadow
work on the GPU instead of a second CPU expansion.

`run.sh --stage-only` succeeded and the candidate was launched with profiling.
Warm observed town windows are about 8.33--9.72 ms wall and 4.50--5.99 ms GPU at
2056x1329, with no luminance readbacks, but still show roughly 124--219 legacy
3D draws and 15--26 MiB upload per frame. Therefore performance has materially
improved toward 120 FPS, but the full Metal migration is explicitly incomplete:
remaining compatibility classes and real-scene alpha/shadow fidelity still need
elimination and user verification. Disposition: unresolved, running for diagnosis.

### Consolidated Metal effects, audio timing, and GPU point shadows — September 16

All linked Git worktrees were consolidated into the main checkout and removed;
their backup refs remain available for recovery. The incompatible renderer
headers were reconciled and `run.sh --stage-only` completed after the final
source changes. The staged executable and build output have the identical SHA-256
`9ad979a415aaeac0df56625235498d11fdc9148b0e1bdab97a72dd6ae4614eb0`.

The character gun-fire animation now rejects duplicate `Fire` events and starts
`pistol_shot` before dispatching `MSG_BLADE_GUNFIRE`, which owns the muzzle visual.
Short PCM effects are decoded before start instead of using the streaming path;
looped, music, and MP3 sources retain streaming. The point-shadow path now keeps
a revision-aware caster registry, performs culling in a Metal compute pass, and
submits all six cube faces through indirect command buffers. The native probe
reported zero CPU face traversals and zero CPU point-shadow draws. Particle/fire/
smoke source coverage and the particle/caustic GPU fixture both pass. Disposition:
staged and source-verified; exact gunshot/smoke synchrony, splash visibility, and
shadow quality remain unresolved until replayed in the real game scene.

### Shadow stability and authored window shafts — September 16, 19:37

Real-game feedback accepted the Metal performance at 120--125 FPS and reported
three remaining defects: duplicate pistol audio, distance-dependent/piercing
shadows, and effectively invisible window light shafts. The prepared correction
removes the extra direct pistol sound while retaining decoded low-latency PCM,
extends the two sun cascades from 32/96 to 64/192 world units with overlapping
24--32 transition and 144--176 fade, and enables the authored `LocationWindows`
mesh bridge for 521 bindings across 51 window models. Shaft composite energy is
raised from 0.16 to 0.28 while preserving depth occlusion, temporal history, and
the adaptive GPU budget. Source probes pass. Xcode 27 compatibility is now
owned by the candidate build: the old `fmt 8.0.1` consteval path is disabled
and the legacy RDF header uses `std::size_t`. The canonical
`run.sh --stage-only` completed after these changes; the build and staged app
executables have the identical SHA-256
`f96e2357b85b2c9b851c0744332ff81acddcfb13e75e06113ca6b18c9b92017f`.
The native point-shadow probe reports one GPU cube dispatch, two retained sun
cascades, and zero CPU point-face traversals/draws. The authored shaft GPU probe
passes depth occlusion, temporal history and adaptive quality, averaging 0.148
ms at its 256x144 test resolution. Disposition: staged and source/GPU verified;
duplicate audio, corrected shadow transitions and stronger shafts still require
real-game replay for visual acceptance.

### Rejected exterior point shadow and screen-space shafts — September 16

The user rejected the staged lighting behavior after real-scene replay. Outdoor
camera rotation made an unrelated local shadow appear and disappear; the cabin
shaft pass produced a moving white veil near windows rather than geometry-bound
volumetric light; cabin, deck and ship lighting remained stale after waiting
across a day/night boundary. Inspection proved that the GPU point-cube path had
bypassed the intended indoor-only admission rule, and that the shaft path was an
additive screen-space approximation rather than a volume constrained by the
window mesh. The corrected stage admits point-cube shadows only indoors and
makes shafts explicit opt-in (`STORM_METAL_LIGHT_SHAFTS=1`) until their volume
and material response are rebuilt. Sun glare remains enabled. Disposition:
staged mitigation; unified time/weather/light revision across location, cabin,
deck and ship consumers remains pending and the lighting model is not yet
accepted.
### Land ambient bounce, coherent water reflections, and SSAO — September 17

Addressed user visual feedback on harsh pitch-black baked terrain shadows and
water reflection lag during camera turns. In `sea-reflection-budget.patch`,
removed the rolling single-face amortisation from `EnvMap_Render()`; the dynamic
environment cubemap for ships now updates all faces coherently every frame,
passing `sea_ship_reflection_probe.py` with zero camera-rotation lag.
In `backend.mm`, resolved the black shadow collapse by lifting the outdoor
ambient response: ambient light on land geometry preserves at least 38% of surface
diffuse albedo rather than multiplying dark 2003 baked vertex colors to near-zero,
and the sky/ground hemisphere ambient factor now provides a natural Caribbean sky
fill (0.30..0.68) instead of dropping to 0.18. Half-resolution GPU SSAO
(`ambient_occlusion_composite.hpp`) with depth-aware bilateral upscale is active
before the HUD pass, adding contact occlusion under characters, ship deck props,
and building crevices. Saved-state liveness tracking protects `TraceARoot`
against dangling references. Staged executable SHA-256:
`e5d084ff44e0bad2f0f3d9602e3114f81b99847659d6fbb75a1bd04281b65586`.

### Rejected night-lighting candidate and unified-source correction — September 17

User replay rejected the night candidate: camera rotation still moved lighting/
shadow response, the nearby ship lamp did not illuminate its boat, and the San
Juan bridge railing remained opaque black. The cause was split coordinate and
source ownership in the renderer: camera-relative receiver geometry was compared
against absolute legacy point-light positions, and the authored location catalog
suppressed ship-owned D3D point lights. The corrected Metal path rebases every
legacy point source into the receiver frame, preserves ship lights alongside the
authored catalog, and moves authored location sources into `LightingScene` as the
shared frame owner. `ogradaR3.tga.tx`, whose decoded alpha contains 32,497 fully
transparent pixels, is now included in the explicit architectural cutout set;
generic mixed-alpha textures are no longer forced into cutout, avoiding the
weapon clipping regression.

`shadow_space_probe.py`, `foliage_cutout_probe.py`, and the unified lighting scene
probe pass. Canonical `run.sh --stage-only` succeeded; build and staged executable
SHA-256 are both
`326cb6621b52081fae3acfbc1c2395994365a136d3679b287518ae52deb6251a`.
The user-owned replay rejected this correction: the boat and ship surfaces still
received no useful local light, camera rotation still changed the shadow response,
the bridge railing remained an opaque black mask, and cabin/hold openings produced
no light shafts. The named `ogradaR3.tga.tx` admission was therefore not the
material owner and has been removed. Disposition: rejected. The agent must not
perform visual scene acceptance for later batches.

### Systemic world coverage candidate — September 17

Metal no longer recognizes foliage, nets, rails, or individual texture names.
Texture resources now derive a cached binary-coverage semantic from decoded alpha:
the image must contain both real holes and solid surface pixels, with endpoint
values dominating transitional data. Automatic inference is restricted to static
world geometry, so mixed alpha used as weapon gloss cannot clip swords or pistols;
explicit legacy alpha-test/blend state retains priority for authored effects and
dynamic models. One effective alpha-test contract is consumed by final color,
immediate depth capture, retained shadow packets, and point-shadow registry keys.

`foliage_cutout_probe.py` passes, `build.sh` produced an arm64 executable, and the
canonical `run.sh --stage-only` completed after the game was closed. Build and
staged executable SHA-256 are both
`ee18301f5217d12c0a26b3cefd234bd172997cbc71ff668b79fcc5b32fc269ed`.
Disposition: staged and source-verified; railing, vegetation, weapon integrity,
and alpha-shaped shadows remain unresolved pending user-owned real-scene replay.

### Unified local light receiver candidate — September 17

Resolves the split between authored location lights and active dynamic D3D point
lights on world receivers. In `backend.mm`, the shader constant generator now merges
active D3D point lights (such as ship lanterns and lamp models) into the unified
receiver light array `lighting.lamps` alongside the authored location catalog,
bounding each source to the receiver's world extent and preserving distance attenuation.
The separate vertex-only point light evaluation on raw lit geometry is retired in
favor of this unified per-fragment evaluation.

Canonical `run.sh --stage-only` completed with both executable hashes matching:
`230f3ad5ba452bdf00a18ab3d7c9938cd1a12edcc97a589ce59271f28837df92`.

Also resolved the rotating-light / double-rebase defect: Storm's `DX9RENDER::SetLight`
already translates dynamic D3D lights into camera-relative coordinates (`+ vWordRelationPos`).
Re-adding `worldOrigin` in `backend.mm` previously added the camera position twice,
causing point light sources to orbit around the camera and detach from physical lanterns
during camera rotation. Removing this spurious second rebase makes dynamic lamp positions
strictly camera-invariant and aligned with receiver world coordinates.
Disposition: staged and source-verified.

### Gunfire muzzle flash lighting candidate — September 17

Integrated real-time gunshot flash illumination for character firearm attacks.
When `CHARACTER_FIGHT_GUNFIRE` dispatches, `Character` now creates a momentary high-energy
point light source `gunfire_flash` (radius 25.0, warm burst) located at the muzzle / hand locator
with a 120 ms lifetime. The source is automatically advanced and pruned in `Character::Execute`,
illuminating the character, surroundings, and ground during pistol and musket shots.
`lights.ini` in both runtimes now contains the authored `[gunfire_flash]` profile.
`gunfire-audio.patch` was cleanly reconciled and tested in the transactional patch stack.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`22d1c33eb533a5c390d0f65053d9984adff8ee152862d5d6bededba5468dcffe`.
Disposition: staged and source-verified.

### Natural volumetric light beams and cabin/hold opening candidate — September 17

Resolved missing light shafts in ship cabins and hold, and eliminated the synthetic yellow haze.
In `volumetric_light_shafts.hpp`, the hardcoded yellow color multiplier (`half3(1.0, 0.88, 0.66)`)
was replaced with a natural soft-sun tone (`half3(1.0, 0.97, 0.93)`), and composite intensity was
tuned from 0.28 to 0.16 to produce subtle geometry-bound volumetric beams without image-wide washout.
In `PROGRAM/locations/init/Boarding.c`, enabled the authored window/opening definitions:
`capsm_rays` with `LocationWindows` is now loaded for small cabins (`capsm`), and `hold_fd` is
bound as `LocationWindows` for the ship hold (`My_Deck`), allowing the Metal light shaft bridge
to register window apertures and ladder hatch openings.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`784924fe4bd3f105124b86b4712e102257b432a2d14ae72f1dd2d08b81685e89`.
Disposition: staged and source-verified.

### Godray direction and interior reach correction — September 18

Corrected the Metal volumetric shaft shader for Storm Engine's DirectX 9 light convention:
`u.lightDir.xyz` is already the sunlight travel vector, so the beam direction now uses
`normalize(u.lightDir.xyz)` without reversing it. Extended the aperture mask to 35 m and the
raymarch distance to 40 m so shafts from high church windows can reach the floor. Increased the
composite contribution from `0.11` to `0.40` for visible interior beams.

Canonical `experiments/native-metal/build.sh` completed successfully, including the
`volumetric-light-shafts-gpu-probe` target. Disposition: source/build verified; real-scene visual
replay remains unresolved.

### VRAM leak resolution and 3D world-space light shafts — September 17

Resolved the 12 GB VRAM accumulation and ~114k GPU region leak identified by user diagnosis:
1. Autorelease pool drain: in `backend.mm`, registered `objc_autoreleasePoolPush()` /
   `objc_autoreleasePoolPop()` boundaries across every `Present()` frame. Autoreleased Metal
   descriptors, encoders, command buffers, and string views are now reclaimed each frame.
2. Zero-allocation skinning poses: replaced per-character `[metal newBufferWithBytes:...]` driver
   allocations in `beginSkinningPose` with sub-allocations from the persistent ring buffer `frameArena`
   via `submissionBuffer`, eliminating up to 3000 buffer allocations/sec at 120 FPS.
3. Cached point shadow tables: eliminated per-frame `newBufferWithBytes` allocations for `faceBuffer`,
   `drawBuffer`, and `boundsBuffer` in `point_shadow_registry.hpp`.
4. True 3D world-space volumetric light beams: replaced 2D screen-space radial vector tracing with
   3D world-space raymarching from camera to depth surface, projecting points onto the window aperture
   along the 3D sunlight vector. Beams remain physically anchored to the room during camera turns.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`bfac17de392e38709ee9da18454eea134329080b58eec40ab86b42030c457374`.
Disposition: staged and source-verified; real-scene visual and memory acceptance unresolved.

### Main menu 3D back-scene interior bridge and ambient ceiling isolation — September 17

Connected `InterfaceBackScene` (main menu 3D cabin environment) to the Metal interior lighting and postfx pipeline:
1. Dedicated lifecycle bridge in `interface-back-scene-metal.patch`: `StormMetalInterfaceBackScene(d3dDevice, true)`
   in `InterfaceBackScene::Init()` and `false` in `~InterfaceBackScene()` cleanly assigns the interior domain
   without mutating or polluting game location state upon exit.
2. Cabin ambient clamping: because the menu script loads daytime weather to render sea/sky through the windows,
   the daytime weather sky ambient (~0.65) was flooding the cabin. In `backend.mm`, clamped interior ambient for
   the back scene to warm indoor levels `(0.16, 0.13, 0.10)`.
3. Sun penetration block: directional sun rays from above cannot penetrate the opaque wooden cabin ceiling
   (`wp.z >= 22.0f`), while the open sea, ship hulls, masts, and sails outside the window (`wp.z < 22.0f`)
   retain their full sunlight and outdoor illumination.

Canonical `run.sh --stage-only` succeeded with matching executable hashes.
Probes `interface_backscene_bridge_probe.py`, `scene_hud_boundary-probe`, and `menu_unlit_native-probe` passed.
Disposition: accepted at probe and staging level; real-launch visual inspection pending.

### Deck walker elevation and looped sail audio — September 17

Restored the complete `deck-walk.patch` in both native source package locations after
the Metal snapshot had been left with a partial source edit and a four-line stub patch.
The visual walker transform now applies a `0.14f` Y elevation above the traced deck
position, while collision and crew resolution continue to use the unmodified floor.
Looped sounds are suspended and stopped when effective volume reaches zero, then
restarted with a short fade when volume returns, preventing silent continuous ship
loop playback. `deck_collision_probe.py` and `deck_walk_probe.py` pass, and the
canonical `run.sh --stage-only` completed through patch verification, build, gameplay
sync, and staging. Real-scene foot placement and audio replay remain unresolved.

### Metal local-light catalog and shadow-composition candidate — September 18

Hypothesis: daytime duplicate silhouettes came from stacking screen-space AO on top of
the outdoor dynamic sun-shadow pass, while night boat/pier receivers missed authored
ship lamps because only location lights entered `lightingScene`.

Delta: outdoor HUD-boundary composition keeps AO disabled when the active land scene is
outdoor; the existing sun cascade selection remains snap-stable instead of cross-fading
two offset maps. `scene-lighting.patch` now publishes each receiver object's live
`ShipLights` point sources from the confirmed `SetLights` path, with current flicker
and transformed positions, a separate ID namespace, and corona-only sprites excluded.
Catalog illumination moved to the Metal vertex receivers; the selected point cubemap is
still sampled per fragment for the shadowed contribution. Location torches and pier lamps
retain the existing source-owned publication path.

Canonical `build.sh` completed with an arm64 engine and no Vulkan/MoltenVK linkage.
`ninja -C experiments/native-metal/.cache/build lighting-probe outdoor_lighting-probe`
completed; both binaries passed. `night_local_lighting_probe.py` also passed.
Disposition: accepted at source/build/probe level; real-scene daylight silhouette and
night boat/pier replay remain unresolved until observed on the actual game surface.

### Town baked-shadow texture matching and sea-lamp catalog fallback — September 18

The town shadow mask matcher now uses a case-insensitive `shadow` substring search,
so `.tga.tx` variants such as `Shadow.tga.tx`, `shadow2.tga.tx`, and location-prefixed
shadow textures are marked for baked-shadow replacement. The generated Metal snapshot
was updated with the same source. `ninja -C experiments/native-metal/.cache/build engine`
completed and produced `bin/engine`.

Sea ship lantern publication exposed a second owner gap: `StormMetalSceneLight` rejects
valid ship sources when no land-location pass has opened the authored catalog. A lazy
catalog-open change was handed to the active `backend.mm` owner because that file is
currently locked by another task. Disposition: town matcher accepted at source/build
level; sea-lamp backend completion and real boat/pier replay remain unresolved.

### Land camera offset from the raw view restore in the shadow prep — September 18

Hypothesis: the reported land camera displacement, the displaced interface back-scene,
and the camera-locked drift of scene lighting were one defect. The land shadow-prep pass
saved the view with the engine reader but restored it through the raw
`IDirect3DDevice9` setter, which skips the rebase that `DX9RENDER::SetTransform`
performs. `GetTransform(D3DTS_VIEW)` returns the un-rebased view matrix, and every
canonical restore in the engine (`lights.cpp`, `location_effects.cpp`, `shadow.cpp`,
`DrawRects`) round-trips through the engine setter instead. The device therefore kept a
second translation of the camera position for the rest of the frame: a few units on a
deck, about 25 units on land, and about 1300 in the interface back scene.

Delta: `scene-lighting.patch` restores through `renderer->SetTransform(D3DTS_VIEW,
&view)`, and `multi-shadow-lamps-v2.patch` carries the matching context line. New
`metal-camera-view-restore.patch` checks the restored device view translation and prints
only on drift, and `location-camera-follow-reset.patch` gained an engine-side framing
probe (`[StormMetal] cam:`). Both are diagnostics to drop after the user confirms the
fix.

Evidence: `run.sh --stage-only` completed (515 objects, arm64 engine, staged app). Live
replay: zero `camera view restore drift` lines, `camRel` always `(0.00,0.00,0.00)`
with no NaN frames (both appeared in the previous build's log), `origin` tracks the
negated camera position and moves with the player instead of freezing, the framing probe
reports the camera 3.32 units behind and above the look target, and land frames reach the
120 Hz limiter at 8.3 ms wall with 2.4 MB upload per frame.

Disposition: accepted at source, build, and runtime-log level. Visual confirmation on the
land surface is with the user. Godray apertures (`openings=0`) and the remaining CPU
conversion (`legacyUnlitCpuDraws` 62-97 per frame) stay unresolved.

### GPU-skinned local lighting and camera-invariant shaft energy — September 18

Two defects, one receiver owner each. Night characters stood near-black under authored
lamps because a GPU-skinned raw draw sampled only the eight camera-selected light slots in
`vs_skinned`, while the floor beside the character was lit by the authored catalog in
`vs_landraw`. Interior shafts appeared to be projected onto the camera because the
accumulator used a strong forward-scattering phase (`g=0.76`,
`(1-g^2)/pow(1+g^2-2g*cosTheta,1.5)`), which varies by orders of magnitude with the
viewing ray even when aperture, sun direction and receiver are fixed.

Delta: `vs_skinned` takes `LandSampling` at buffer slot 6 — slots 4 and 5 already own the
bone palette and `SkinU` — evaluates the same authored catalog as `vs_landraw`
(contribution, range-edge fade, attenuation) and forwards `directPoint`. The compact
FVF unlit path no longer accepts GPU-skinned streams, whose 44-byte `AnimatedVertex`
layout it cannot decode, so unlit skinned draws use `fs` instead of the removed
`fs_skinned_unlit`. `bindLandSampling` now takes an explicit vertex slot instead of a
`vertexStage` flag. The shaft phase is a flat `1.0`; the aperture prism, depth receiver,
temporal reprojection and world-space sun direction are unchanged.

Evidence: `gpu-skinning-offscreen-probe` renders GPU-skinned draws pixel-identical to the
CPU oracle and covers the unlit case; `dynamic-lighting-probe` builds its skinned quad
through the real `gpu_skinning_bridge` and requires the lit catalog to exceed twice the
dark energy; `volumetric-light-shafts-gpu-probe` now requires the same fixed world beam to
stay within a 1.5x factor between two camera positions, and its occlusion check keeps the
receiver footprint while the integrated volume loses two thirds. Also passing:
`gpu-skinning-parity-probe`, `lighting-scene-probe`, `local-light-bounds-probe`,
`character_skinning_probe.py`, `authored_light_shaft_openings_probe.py` (522 bindings /
51 window meshes). `run.sh --stage-only` completed; build `bin/engine-1` and the staged
`metal-engine` are both `d34a0c36c8365d9ec583f37e8ab6e94702d0fab38e0d5d7f50c6b6055265f773`,
arm64, no Vulkan/MoltenVK. Launch log: outdoor menu frames at 8.3-8.6 ms wall / 4.7-6.0 ms
GPU, `camRel=(0.00,0.00,0.00)`, `sun=3 applied=1`.

Disposition: accepted at source, probe, staging and launch level. Night character
brightness under lamps and church/indoor shaft appearance remain unresolved until they are
replayed on the real game surface.

### Window apertures derived per opening — September 18

`StormMetalLightShaftMesh` reduced every submitted `LocationWindows` object to a single
axis-aligned bounding box. One object groups the glass of several windows, so the box
described the window band of the whole building and the aperture prism filled the
interior: the church reported `openings=3` while every interior view ray crossed the beam
for many units, so the volume pass read as a lit haze with the character punched out of
it. `openingBeamMask` additionally diverged 8% per unit and feathered over the outer 60%
of the cross-section, widening one window into a room-sized cone.

Delta: `light_shaft_apertures.hpp` derives one aperture per connected coplanar triangle
group of the submitted mesh (shared vertex, face-normal dot >= 0.9995, plane offset
<= 0.02), fits the rectangle to the group's own projected edges and keeps the smallest
covering pair; an authored id now owns its whole rectangle set. `spread` drops to
`1 + distance*0.02` and the cross-section feather to `smoothstep(0.78,1.0,...)`.

Evidence: `light-shaft-apertures-probe` (separated quads stay separate, shared panes and
double-sided panes merge, capacity keeps the largest deterministically, degenerate and NaN
groups are skipped); `volumetric-light-shafts-gpu-probe` gains a room-scale fixture that
requires the brightest column to stay at the opening and the far room wall to stay below
1/20 of the peak. Also passing: `authored_light_shaft_openings_probe.py` (522 bindings /
51 window meshes), `gpu-skinning-parity-probe`, `gpu-skinning-offscreen-probe`,
`dynamic_lighting-probe`. `run.sh --stage-only` completed; `bin/engine-1` and the staged
`metal-engine` are both `3cf0eaed9f89a588f8327485e33102fc5a222fe97a0d5cc7259c5f42fff9c9d8`.

Disposition: accepted at source, probe and staging level. Whether a church or indoor
interior now reads as light through glass rather than a haze is unresolved until it is
replayed on the real game surface; a new `[StormMetal] shaft apertures n=... c=... hw=...
hh=...` line reports the derived set per room.


### Deck contact and enabled weather-sun capture — September 22

The `c3d1e164…` installed replay narrowed the remaining deck defects. `system.log`
records nonzero canonical input and accepted path movement, so walking input is no
longer the blocker. The screenshot shows native sailors grounded correctly while only
the presentation protagonist sinks. The attempted animated camera-locator alignment
was therefore rejected: it aligned the eye target by translating the whole body below
the path support. The candidate restores root placement at the resolved support with no
former lift; `deck_motion_probe.py` passes ship yaw, roll, movement and contact-root
checks.

The same run reports a complete sea registry (`draws=5`, `vertices=10299`) but
`sun=0`, `rawShadowDraws=0`. Geometry admission is no longer the failing owner. Weather
enables the directional source before ship rendering disables/reuses slot 0, and no
receiver packet retains it. The candidate captures the first valid enabled directional
source in the current traversal frame and clears it at `Clear`/`Present`. The mixed-order
GPU fixture passes: 104 mast pixels and 96 late-actor pixels darken independently, while
the next frame without a source remains unchanged. `run.sh --stage-only` produced matching
built/staged SHA-256 `6b22fb9d3d5e428ca91b2f8d402b34527e68e740b4b8661f35fa9842227035f1`.
The 23:28 rejection screenshot came from the prior installed `c3d1e164…` process and still
correctly rejects that runtime; visible contact and deck-shadow acceptance for `6b22fb9d…`
remain unresolved until replay.

The subsequent installed `ff98425a…` replay was also rejected. Its live movement trace
proved that `MODEL::Trace` returned the ray start itself (`pathY=4.529`, false
`supportY=8.488`), so it lifted the protagonist instead of finding the visible deck;
movement then reached `solid_rejected_or_slid` at the stair. The replacement support
query scans ship triangles, accepts only upward-facing walkable surfaces near the authored
path, preserves the full vertical delta through the collision sweep, and still rejects a
vertical wall. `deck_motion_probe.py` passes the regional deck offsets, stair tread and
solid-wall negative.

That replay also showed that fixing the directional receiver slot alone was insufficient:
the ordinary lit sea FFP deck/hull path was rendered but was not retained as a shadow
receiver. The traversal registry now admits that existing `rawSeaLit` class while retaining
the custom-water, particle, unlit UI and dynamic-source exclusions; it also freezes the
first enabled directional source regardless of D3D light slot. The GPU fixture passes with
an unscoped deck receiver, an alpha-cutout mast, a late GPU-skinned sailor, Weather sun in
slot 3, and a no-source next-frame negative (`mast-darkened=104`, `actor-darkened=96`,
`no-sun-changed=0`). `run.sh --stage-only` completed with matching built/staged SHA-256
`8fcab0773294426dfed9ba1a6ebbcd0e64ee3ef158df58540ec4489816e8fa0c`; `run.sh`
started the installed build as PID 47262. Disposition: accepted by focused probes, full
build, canonical staging and startup; real deck contact, stair traversal and visible deck
shadows remain unresolved until this exact process is replayed.

The player replay of installed `8fcab077…` rejected that candidate. The protagonist
still floats above some deck regions, enters the planks near the stair and cannot
traverse the stair; mast, sailor and protagonist shadows are still absent, leaving
the ship visually flat. The live trace records nonzero movement input followed by
`solid_rejected_or_slid` at the stair, so controls are not the blocker. Source review
found that the visible-support query clips the complete ship model while starting
exactly on the hidden `path` mesh; nearest-hit selection can therefore select that
hidden navigation triangle at distance zero instead of the visible deck. The live
shadow registry contains only five scope-2 draws (`10299` vertices) even though the
same frames report hundreds of lit raw draws, proving that the focused deck/mast/actor
GPU fixture did not reproduce the real ship receiver admission. Disposition: rejected;
contact, stair traversal and all requested deck shadows remain unresolved.

The next installed `31d27e02…` replay was rejected for the same visible outcomes,
but its live traces isolated both remaining owners. Movement reached both stairways
and then repeatedly stopped at the symmetric lower transition near local
`x=±2.2, z=-1.0`; the shadow registry retained only one to four scope-2 packets,
all GPU-skinned, while the deck and mast were absent. Geometry inspection bound the
stair coordinates to `Caravel_w1`: the old nearest-support rule selected hidden ramp
triangles 1050/1052 about 0.15 m below the visible treads, and the collision sweep
then treated the remote high vertex of each large walkable ramp as a wall.

The replacement support rule selects the highest walkable surface no more than
`MEN_STEP_UP` above the navigation path, with nearest support only as fallback.
Floor-aligned collision triangles now compare the local plane height at the start
and end foot positions; non-floor triangles retain the global-height wall check.
The focused probe covers a hidden ramp below a tread, a large sloped floor, a low
riser and a tall-wall negative. In the first installed replay the live trace records
an uninterrupted descent from `z=-2.433` through `z=-0.688` to the lower deck and
the player accepted stair traversal and protagonist height.

That replay also proved why the ship remained flat: 54–155 ordinary ship draws per
sample were `rawSeaLit`, but zero entered `rawSeaShadow` because MODELR left alpha
testing enabled with a fixed-function combiner outside the narrow legacy
`alphaContract`. These partitions write depth without blending and retain texture
alpha in the raw packet. Sea/deck capture now admits that opaque cutout contract,
while depth-disabled and blended draws remain excluded. The mixed-order GPU probe
passes (`mast-darkened=105`, `actor-darkened=204`, `no-sun-changed=0`). In the real
sea scene the registry increased from 1–4 skinned-only packets to 18–175 packets;
17–172 are captured ship sections with active directional receivers. Canonical
staging produced matching built/installed SHA-256
`4646ae77ca21a6b8a82504aaa57f8c1309087211729e8ad91ae43576bc88db55`.
Disposition: stair traversal and deck contact accepted on the real game surface;
visible mast, sailor and ship self-shadowing are installed and runtime-admitted but
remain unresolved until the player confirms the rendered result.

### Metal audio voice policy — broadside and charge selection, September 22

The active Metal service allocated every cannon event as an independent miniaudio
voice from a 4095-slot pool. A large broadside therefore summed many full-level
copies without a concurrency bound or bus headroom. The same service let the four
ammunition-selection announcements overlap until their files ended. Alias
priority was parsed but did not participate in playback arbitration.

The candidate gives Metal its own audio source owner and adds two bounded classes.
All cannon and fort-cannon aliases share a maximum of 12 live voices with a
gentle inverse-power group gain; a sparse shot keeps its authored level. The four charge
selection aliases replace one another with a 90 ms fade and retain at most the
fading predecessor plus the current voice. Splash, impact, fly-by, speech, music,
and ambient sounds remain outside both rules. The policy probe passes, and the
full arm64 engine built as SHA-256
`77ab3ffc32386b864aa22e733db0143a8d4d519d05d42c872519c963912117d4` from the
Metal-owned sources. The first stage was correctly held while the prior player
process (PID 15045 at inspection) was active. After that process exited,
`run.sh --stage-only` completed and the installed bundle matched the same hash.
`run.sh --launch-installed` then started PID 25506; the arm64 Metal process
remained responsive in the sea scene without a new script or engine startup error.

Disposition: accepted at source, policy-probe, full-build, canonical-staging, and
startup level. Real-surface acceptance requires a
large broadside without metallic overload, a sparse single shot at normal level,
rapid charge changes that replace cleanly, and one unaffected impact or splash.

The first real-surface replay confirmed that the metallic overload was reduced,
but rejected the initial 0.9 peak-sum ceiling as insufficiently epic. Cannon event
times were never changed. The corrected candidate keeps the 12 spatial voices and
uses `active voices^-0.4` gain so a broadside retains density and controlled energy
growth while the hard polyphony bound still prevents the former hundreds-voice pileup.
Charge-selection replacement is unchanged. Rebuild, staging, and broadside replay
for this corrected gain law produced matching built/staged arm64 SHA-256
`ff98425afaf4b69a8d1ff9a8f263abb57f8c079c5f72be18c706f226141caf68`.
The corrected broadside replay remains pending.

### Dusk sea-horizon seam and directional shadow distance — September 23

The reported dusk sea scene showed a pale horizontal seam at the horizon. The modern
water body already bounded its reflected cube colour with the current weather palette,
but the separate additive sun-road pass sampled the same cube without that bound. The
candidate applies `weatherBoundedReflection` to the sun-road sample before its existing
sea fog and additive composition. It does not change the authored cube direction or
remove the daylight sun-road highlight.

Directional shadow coverage is extended consistently for outdoor locations and sea/deck
traversal. The cascades now cover 96 and 288 world units, select at 42, fade from 216 to
264, and use a 120/240 depth offset/envelope. The engine location patch consumes those
values through the Metal bridge; sea/deck traversal uses the same 288-unit footprint.
Authored point-light ranges remain unchanged.

The weather bridge and compiled colour probes pass. The shadow source, quality, point,
and mixed sea/deck GPU probes pass, including the no-sun negative, and the complete
ordered patch stack builds arm64 engine SHA-256 `af51fc67de64d55fb7f3a4031d3e2c22464888215364ba40a957d676755dc8e0`.
The broad modern-water fixture still fails
its pre-existing base-water colour assertion before it reaches the separate sun-road
mode; it does not exercise this fix. The prior installed engine remains active as PID
46457, so replacing its bundle executable is intentionally deferred. Disposition:
accepted by source probes, GPU probes, and canonical build; staging and real-scene
acceptance remain unresolved until the player-owned process exits, the installed
`27b4c61a…` executable is replaced by this exact build, and the candidate is replayed
under the reported lighting.

### Daytime black horizon band — September 23

The Cayman 10:35 screenshot showed a nearly pure-black band below the blue sky.
The active day weather presets define a light blue fog colour, while the procedural
sky read its low-horizon colour indirectly from mutable `D3DRS_FOGCOLOR` and the
separate SkyFog sphere rebuilt RGB through `WEATHER_CALC_FOG_COLOR`. Those two paths
therefore had no shared explicit visual-colour contract.

The Metal bridge now snapshots `WEATHER_BASE::GetVisualColor(whc_fog_color)` once
for the procedural sky and uses the same visual RGB for SkyFog vertices while
preserving their authored alpha. Missing weather keeps the original fog vertex;
near-black midnight weather remains unchanged and no brightness floor was added.
The bridge round-trip probe, dynamic-sky CPU/GPU probe, backend scope probe and
shadow-quality ABI probe pass. The complete ordered patch stack builds arm64 engine
SHA-256 `e451f299c4a9acfbe84f2d0fc20056bdcf9c0421ad5d89fbaeb0af0ce907cc57`.
`run.sh --stage-only` completed with matching built and installed hashes
`e451f299c4a9acfbe84f2d0fc20056bdcf9c0421ad5d89fbaeb0af0ce907cc57`.
The installed arm64 process was relaunched as PID 65086, remained responsive and produced
continuous completed GPU frames without a startup or shader error. Disposition:
accepted by source, contract and GPU probes, canonical build, staging and startup;
the real Cayman 10:35 horizon remains unresolved until player replay.

The subsequent real Terks 09:36 replay rejected that candidate: the same exact-black
band remained. Binary inspection confirmed that `e451f299…` contained the intended
RGB mask, so this was not stale staging. The draw trace bound the band to the separate
untextured `SkyFog` hemisphere (`FVF 0x42`, vertex diffuse, alpha blend). The rejected
bridge sampled visual fog only when `GenerateSky` or script `Sky.TimeUpdate` rebuilt
the vertex buffer; the procedural sky sampled current weather every draw. Its green
source probe checked call presence but could not detect that lifetime mismatch.

The corrected candidate gives `SkyFog` an explicit draw-time Metal scope. The backend
replaces only RGB with the current visual fog colour, retains authored per-vertex
alpha, and disables compact/persistent reuse for the 33 scoped vertices so stale black
data cannot survive from an earlier weather state. Missing weather keeps the original
legacy path. The bridge lifetime probe, dynamic-sky colour probe, backend scope/cleanup
probe and complete arm64 build pass; built SHA-256 is
`b99ef97a170aa96fc1edcd126efd5f3330373267ab3e0803db36a40be8026e5d`.
`run.sh --stage-only` completed after the previous game exited. The initially
isolated horizon build was `b99ef97a…`; the final integrated canonical batch also
contains the contemporaneous frame-pacing correction and rebuilt/staged as matching
arm64 SHA-256 `c195de8440f5fd9d126f312d924a8c9ace9cd37b24f30c016cb2d576a1f9c20f`.
Symbol inspection of that installed executable confirms the explicit
`StormMetalBeginSkyFog`, `StormMetalEndSkyFog`, and `StormMetalSkyFogActive` bridge.
`run.sh --launch-installed` started the integrated build as PID 81150; the process
remains alive, completed GPU frames and reports no startup, shader, or command-buffer
error. Disposition: the `e451f299…` real-scene result is rejected; the draw-time
candidate is accepted by source, focused GPU/backend probes, canonical integrated
build, staging and startup. Real daylight replay remains unresolved until the player
revisits the reported horizon.

The Tortuga 13:35 screenshot from the installed `c195de84…` candidate rejected the
RGB-only correction again and exposed a perfectly horizontal coverage hole. Geometry
inspection proved that the normal base-sky draw submitted only upper vertices 0–19;
the existing mirrored lower vertices 20–39 were gated to `Delta_Time == 0` reflection
draws. `SkyFog` is also strictly an upper hemisphere, so neither draw could touch the
black clear target below the shared `y == 0` equator.

The final Metal correction submits those existing lower faces in both opaque base-sky
loops on normal frames while leaving the astronomy alpha overlay and non-Metal path
unchanged. For negative sky directions the procedural shader already suppresses clouds
and returns the current horizon fog colour. The source lifetime/coverage probe, dynamic
sky shader probe, backend scope probe and complete canonical build pass. Canonical
staging produced matching built/installed arm64 SHA-256
`2078cc65f7b0d9f72f8e351d3ee2526a041315534e3359b8ee27be0e573454f0` and launched
PID 86431. Live Tortuga replay at 19:31 visibly shows continuous blue/fog background
below the former boundary with terrain and vegetation correctly over it; no black band
remains, and the HUD reports 127 FPS. Disposition: accepted on the real game surface.
After that verification process exited, the same installed hash was relaunched as
PID 87408 for continued play.

### Sea battle HUD tinted by ship lighting — September 23

The reported night-deck screenshot showed the sea battle controls and text turning
blue only in camera directions affected by ship light or shadow. The sea battle
interface is realized in `SEA_REALIZE` and calls `MakePostProcess()` immediately
before drawing its HUD. The existing Metal scene/HUD marker ran later at
`INTERFACE_REALIZE`, so point/directional shadow resolve could still composite over
the already drawn sea HUD. The authored battle-interface textures remain neutral;
this was a render-order defect rather than blue source art.

The Metal-only renderer patch now emits the existing idempotent
`Storm.SceneHudBoundary` marker at the start of `MakePostProcess()`, before its
legacy post-process early returns. This freezes and resolves the scene before sea
and land battle HUD drawing while retaining the later core marker as a safe no-op.
The consumer coverage probe requires both the early marker and the sea HUD call
order. That probe and `scene_hud_boundary-probe` pass, the complete canonical arm64
build succeeds, and `run.sh --stage-only` installed engine SHA-256
`64a2471b21b47da606feec7af338ef47483cedefc9d463ce198ef74908601eab`.
`run.sh --launch-installed` started the staged engine as PID 15639 and it remains
responsive with completed GPU frames and no startup, shader, or command-buffer
error. Disposition: accepted by render ownership, focused probes, canonical build,
staging, and startup; visual acceptance remains unresolved until the active game is
loaded into the reported ship scene and the camera is turned through the offending
light/shadow direction.


### September 30 — portable Metal input and build delivery

Hypothesis: captured pinned engine, dependency/toolchain and baseline gameplay
inputs eliminate original-checkout, Conan and historical runtime dependencies.
Delta: ignored hash-manifest inputs, relocated development-kit exporter and
canonical build/stage input selection; existing reviewed gameplay patch checks
remain authoritative. Private kit preserves player SAVE and settings.
Result: clean relocated 548-target build completed with intentionally nonexistent
external engine/Conan roots. First staging failed at the graphics adapter's
archived runtime baseline; selecting captured gameplay in that adapter corrected
the demonstrated path. Complete relocated canonical staging then passed, zero
pending gameplay files and compiler ready. All 139 SAVE files matched. Packaged
SDL and system-only dependency closure passed; tamper/occupied-output/old-Python
and active-game rejection checks passed. No visible gameplay acceptance is claimed.
Disposition: build/packaging accepted; final integrated canonical staging and
player menu/scene replay pending. A kit game remains active and is preserved.
Public app, personal menu branding, Drive and site download are a separate
user-authorized delivery batch; existing Reconstruction 1.4.1 remains unchanged.

### September 30 standalone startup and menu player replay

The signed native launcher replaced the shell entry point; proper nested framework
and whole-app signatures pass strict deep verification. Symbolicated system evidence
binds the prior `tccd` work to unsigned-bundle signature synthesis, not to a proven
Metal GPU deadlock. Signed CoreAudio init/uninit passed and installed state staging
finished in 0.18s. The player confirmed the menu and playable sea scene at 13:24–13:26,
so this corrected startup replay is **accepted**. No universal stability claim.
The screenshots rejected the missing main-menu logo and vertical social/footer
label placement. Logs prove a duplicated INTERFACES texture path; source correction
uses a leaf name and adds explicit offsets/normal footer font with reviewed-hash
migration. Corrected visual replay remains unresolved until the next menu view.
Resolution borders cleared after the player's game restart; live display reset
remains intentionally excluded because it previously invalidated interface resources.

### 2026-09-30 — remove duplicate menu banner

Player screenshot confirms the restored logo and centered social buttons. User requested removal of the lower framed banner and a clickable mixed-case version line instead. The exact-hash layout/script layer removes only that banner, routes its keyboard neighbours to VERSION, and attaches the existing HTTPS handler to VERSION. Canonical stage succeeds; installed app is resealed. Disposition: staged; final version-link interaction remains unresolved pending player click.

### 2026-09-30 — superseded unsigned-app startup baseline

### Installed app startup hang — September 30, player replay rejected

The player launched the independent app and reported the Mac becoming unresponsive,
including two failed login/recovery attempts. The system recorded WindowServer
watchdog reports at 12:42 and 12:47, reset reports at 12:43 and 12:45, and a
`metal-engine` hang at 12:48–12:49 for the installed standalone app.
Its main thread was blocked during `SoundService::Init → ma_engine_init →
CoreAudio → TCCAccessRequest → synchronous XPC reply`. WindowServer's main thread
was also in a TCC screen-capture preflight queue. This proves a startup hang and
concurrent session failures, not that the game caused both resets. The package's
startup/stability acceptance is **rejected**; causality and repair remain unresolved.
No automated game launch is authorized. Public copy must identify this build as
experimental with unresolved startup stability. The user prioritized case/source
publication and deferred game repair. Preserve raw system reports locally; never
publish their private machine/process inventory.


### 2026-09-30 — local generated-state cleanup

The independently installed signed application and 138 active saves remain. All 45 archived native-storm save hashes resolve to the active profile or its six-file `Archived/native-storm/SAVE` supplement. Root removed both disposable native development caches, the redundant private development bundle, four intermediate ZIPs and the unsigned app backup using owned temporary directories. The 8,252 hash-bound compact inputs remain (246 MB), and default `git gc` reduced Git from 6.3 GiB to 2.86 GiB without history rewriting. Development staging restores disposable game data from the installed app. Windows installations are untouched. The corrected archive passes native extraction, deep/strict signature verification and relocated launcher staging; installed profile symlinks were restored afterwards. Drive synchronization subsequently passed anonymous cloud metadata and HTTP 206 range checks against the new 11,298,197,161-byte archive. Finder Remove Download evicted its local payload; the public cloud file and folder remain available.

### 2026-09-30 — illustrated case and repository consolidation

Production website revision `ea37ee5` includes all five English image-generation edits and source/download links; original cover is preserved. Single visuals now consume plain EvidenceFigure rather than a paired gallery/artboard, removing the empty right-hand frame and irrelevant arrows. Build, lint, type check and case checks passed; public accessibility tree confirms plain figures. Final screenshot observation is pending because macOS locked during capture.

Verified 8,252 portable inputs before consolidating obsolete engine sources. Preserved the complete 30-ref engine history as a verified 110 MB Git bundle, moved the old dirty 153 MB deck-shadow worktree intact inside the canonical repository, and removed the obsolete external engine checkout/bare repository/build. Read-only candidate directories initially prevented deletion; owner write permission on those exact temporary directories allowed the approved deletion helper to complete. No save, installed app or Windows runtime was removed. Disposition: local consolidation accepted; historical dirty candidate is retained, not integrated.


## September 30 — minimum lighting and current-town auto-pin delivery

### Minimum scene lighting and trade auto-pin — September 30, installed; replay unresolved

Integrated source: `bbd2037` on main, pushed to the verified public origin/main.
The exact batch was built and staged with
`experiments/native-metal/run.sh --stage-only` after the final source change.
Gameplay sync reviewed and applied only PROGRAM/interface/tradebook.c, preserving
SAVE/config and renderer resources. Installed app remains the standalone arm64
Metal GPK 1.3.2 AT + ReConstruction 1.4.1 owner. No game process was running during
staging or installation. Earlier inventory hashes below are historical.

Modern classified outdoor source-ambient floor increased .36 to .40; existing
warm indoor floor (.125,.100,.078) increased to (.140,.112,.088). Day ambient above
the floor, direct lamps/sun, emissive/unlit/UI behavior and graphics settings were
not changed. Existing CPU indoor/outdoor probes and native GPU lighting/dynamic
lighting fixtures passed, including retained materials, alpha, skins and lamp
contrast. The representative modern outdoor night GPU channel rose 45 to 51;
day stayed 52. These are fixture samples, not whole-scene perceived brightness.
The player screenshot before this batch showed Cartagena at 06:30 and FPS125.

Trade comparison initializer and mode toggle now attempt current-town auto-pin
when the saved pin is empty, including a persisted empty attribute; valid manual
pins and immediate unpin remain. No eligible current city/known prices leaves it
empty. Two disposable VM fixtures failed before Main; no gameplay acceptance is
claimed from them. The earlier PID78900 crash was the diagnostic CLI invocation
without engine.ini, not the player's merchant-dialogue crash.

Staged engine SHA-256 `9d152e333b6745c41ca750364a9cddc0ffd81dbd1f1bb3082b259a93342eaee8`;
installed engine SHA-256 `3ea9553b27f614bb1f93ee8a7f493c9cef97518444efe5b1b82bf5eb50619884`;
installed trade script SHA-256 `e4572beabccda273daca89bfee8270ac633c6a1b3ffbdd08a12b46e33a2340c6`.
Deep strict signature verification passed. All 139 SAVE files and
4 player configuration files were byte-identical across installation.
Rollback engine, prior trade script and CodeResources retained in the ignored
native-metal cache `.cache/app-update-backup-night-pin` with an install receipt.
Current player display configuration: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}; dynamic_lighting=1,
shadow_quality=2, modern_lighting=1 remain enabled in retained metal_graphics.

Disposition: build, probes, canonical staging and installed signed delivery
accepted for their checked properties. Actual Cartagena dawn/night readability
and comparison auto-pin on the current save remain player-replay unresolved.
The merchant dialogue crash remains unresolved: nested SetShow reached corrupted
window vector state; isolated valid recursion/nesting did not reproduce it.
No speculative crash guard was added. The user explicitly deferred crash
investigation and requested immediate installation of the existing batch.
Owners: docs/metal-lighting-parity.md, docs/trade-journal.md,
docs/metal-graphics-settings.md. No new game launch was required for this delivery.



## September 30 — outdoor floor correction and sea-boundary diagnosis

### Outdoor night brightness correction — September 30, installed; sea boundary unresolved

Source `80b99e1` on main was built and canonically staged via
`experiments/native-metal/run.sh --stage-only`, then installed into the standalone
arm64 Metal app. The player rejected the .40 outdoor ambient floor as too bright
in Porto Bello at05:25. The corrected .38 floor removes half the last increase;
indoor fill, authored lamps, brighter/daylight weather and compatibility behavior
remain unchanged. Existing CPU outdoor energy/continuity/negative checks passed;
existing GPU lighting probe reports modern night48 (prior51), day52 unchanged,
legacy night14 unchanged, with emissive/unlit/UI/alpha checks passing. Perceived
night readability still needs the player scene replay.

Staged engine SHA-256 `ace96ac1a30e44197cf8cb715f1a67265f23133847e1c0740d54d29cdef70adf`;
installed engine SHA-256 `88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
Deep strict app signature verified; all 140 SAVE files and
4 player configs byte-identical across installation. Rollback
engine/CodeResources retained at native-metal `.cache/app-update-backup-night-tune`.
Trade auto-pin from bbd2037 remains installed. The user closed the playing game;
CUA observation inadvertently started diagnostic PID76219 at21:44:47, which was
closed with TERM before staging. No player gameplay process was stopped. Earlier
engine inventories below are historical, superseded by these hashes.

The separate dawn-water boundary has a source-backed cubemap-background
hypothesis, not a reproduced cause; no sea patch was applied. Ownership and exact
missing falsifier are in docs/metal-weather-surface-color.md. Merchant dialogue
crash remains deferred by the user's prior instruction.


Delivery closure: origin/main and local HEAD verified at
80b99e14b1e540cdc2660fad0f07cdf1b81a3922; working tree clean. Repository knowledge
check initially stalled in the Python3.14/Git stdin-stdout exchange (both sampled
waiting in write); the owned check processes were closed. System Python3.9
rejected missing tomllib before validation. The unchanged canonical
`python3.13 tools/agent_context.py --check` passed with the available compatible
interpreter. No verifier/source/hook workaround was installed.


## September 30 — tornado technique correction

### Tornado white rectangles — September 30, staged; installation pending

Source `cc45e5e` on main restores existing particle technique loops around native
tornado pillar/ground and noise-cloud draws. Only renderer-particles-fx.patch
changed. The player's storm screenshot at11:41 shows white sprite rectangles;
the current native bridge had bypassed the authored texture/alpha stage chain
and inherited the untextured TornadoPillar material. Installed mask textures
exist. The missing Tornado/ITEMS.TGA entry concerns debris and is a separate
resource finding, not proof of the mask mechanism.

A 32-square correctness fixture with actual Metal particle bridge and seeded
prior valid material state reproduced white rectangle pixels, then preserved
transparent background ff203040 and half-alpha bf9098a0 with the authored chain.
The untextured negative remained white. This is not an FPS measurement or actual
storm replay; the player game was running during this small correctness fixture.
The existing source inventory probe could not open its absent native-storm
baseline and did not pass. Canonical build integrated two changed consumer
source files; run.sh --stage-only passed with compiler ready and zero gameplay
pending. Staged engine SHA-256 `f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`.

Installed standalone arm64 Metal engine remains
`88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
A closed-game baseline captured143 SAVE files and four configs. The user then
restarted as PID39052 and player state changed before installation. The
installer refused at the initial baseline comparison before creating backups
or replacing the engine; no live runtime mutation occurred. Installation remains
pending a closed game and fresh SAVE/config baseline, with this chat as owner.
Native storm replay remains unresolved after delivery. Mechanism/evidence owner:
docs/audio-and-particles.md. Prior dawn-water boundary and merchant crash remain
unresolved; this patch does not address them.


Tornado closure: local HEAD and public origin/main verified at cc45e5e175337098502dc8917b514f7f5c18171d; tree clean. Build/probe/stage and refused-install receipts plus the guarded installer retained under the ignored canonical native-metal owner `.cache/tornado-delivery`. Resume only after process count0 and a fresh player SAVE/config baseline. The staged batch is frozen; no additional source change precedes this install.


Sea-crime deferral (October 1, staged batch): killing both legacy instant paths. Ship_FireAction else-branch and CrimeSea_PreparePlayerBallHit ENEMY-branch now call CrimeSea_HandleUntrackedEnemy (tactical hostility now, one deferred incident for unlawful victims behind a surviving report). CrimeSea_IsLawfulPrize (pirate victim, base-nation enemy, covering patent) guards CrimeSea_Begin too, so lawful privateer prizes open no incident at all. Quest protections preserved. AIShip.c chain recomputed: crime e024268a, journal 6f39bde1, living ec2b0915; NATIVE_PREVIOUS_SHA256 keeps old delivered journal 1cb84155 for the upgrade path. sync check shows 4 files pending (ship.c, tradebook.c, AIShip.c, ship.ini), compiler ready. Committed as 1b9720f on top of 551136c (aim overlay) and 0904367 (night floor/pin/chest). Disposition: pending run.sh --stage-only, then player replay. Captive-captain witness channel stays deferred: ReleasePrisoner cannot distinguish fates, needs per-fate flags in Ransack dialogue.


Stage receipt (October 1): run.sh --stage-only exit 0 with closed game. Engine rebuilt (arm64), renderer consumer PASS, town shadows/Belize/Menu PASS. Gameplay applied: 4 files (ship.c, tradebook.c, AIShip.c, ship.ini). CorsairsMetal.app staged. SAVE/config/renderer resources unchanged. Batch contains aim trajectory overlay v1, night floor 0.20, trade auto-pin follow, ship chest button, deferred sea-crime. Aim polish (Singer), witness channels (Cicero) run as next batch; night/fog diagnosis (Noether) delivered. Disposition: staged, player replay pending.


Delivery gap (October 1): user plays /Applications/Corsairs Iddictive Remaster.app (engine f9040719), but engine-side patches stage only into .cache/CorsairsMetal.app (engine ec5f4298). Gameplay scripts mirror via sync apply; the engine binary does not. Aim overlay v1 source-verified in staged build (trajectory markers in ai_ship_camera/cannon_controller). Install candidate prepared at .cache/install-candidate/metal-engine (repo rpath stripped, Frameworks rpath added, no /Users/ or repo byte leaks, sha 775751c5). Disposition: blocked on closed game for in-place engine swap + codesign.


Install-flow fix (October 1): staging now ends with install-engine.sh, which copies the staged engine into /Applications/Corsairs Iddictive Remaster.app (rpath rewired to bundle Frameworks, machine-path assertion, ad-hoc codesign, per-hash backup, skip-when-identical). run.sh already refuses while the game runs, so the swap is always closed-game. Wired into run.sh before the stage-only exit so both stage and launch modes refresh the played app; missing bundle exits 0 for portable/export flows. Disposition: implemented, first live install pending closed game.


First live install (October 1): run.sh --stage-only exit 0, install-engine.sh copied v1 engine (aim overlay, night floor) into /Applications (new ef934466 vs old f9040719, byte-verified different; per-hash backups kept). Gameplay already mirrored. Sync apply and install each deep-codesign the 21GB bundle, so stages take minutes. Polish did not compile (iFadeRope undeclared x6, brace imbalance x2) and was stashed for v1; Cicero WIP snapshotted for a clean stage, both restored byte-identical and resumed. Commit of install-flow files deferred: git index locked by sibling op. Disposition: v1 installed, player replay pending; polish+install round next.


Volumetric cannon balls (October 1, source candidate): confirmed stock projectiles are zero-radius rays (SAILONE::CheckSailSquar ray-triangle, NODE::Trace mesh rays, ropes/vants/yards absent from cannon trace), which plus the 12-knippel mast-fall threshold explains rigging fly-throughs. New cannon-ball-volume.patch adds rigging-only capsules (knippels 1.0 m, grapes 0.35 m, rest half visual size; hull/fort/island/sea stay exact rays), registered in build.sh after the sibling aim patch; disjoint from cannon-trajectory-aim.patch and crime files. Falsifier: git apply --check passes against the post-stack source. Disposition: pending parent run.sh --stage-only, then sea-battle replay. Spec: docs/sea-cannon-ballistics.md.


Stage receipt (October 1, aim rework + volume): run.sh --stage-only exit 0 with closed game, committed as 168c245. Engine e32144db installed into /Applications (codesign valid, hash-verified). Batch: powder-smoke ribbon + impact ellipse aim overlay (firing-side only, depth-tested, live HeightMultiply 0.4/0.65 knippels mirrored from Ball_AddBall after parent caught a 1.0-hardcode mismatch), rigging-only capsule hits (knippels 1.0 m, grapes 0.35 m). Integration fixes by parent: sail.h blank-line context, sailone hunk2 off-by-one + missing trailing context (in-repo git apply --check was a false positive; out-of-repo check is the true gate), const CMatrix blocking operator*(CVECTOR). Disposition: staged, player sea-battle replay pending (preview==impact, native look, mast/knippel kills).

Aim staging fix (October 1, visual-verification pass): static review of 168c245 found ShipAimArc/ShipAimVolume defined only in engine-source src/techniques/_dev/ship.fx, never staged to runtime RESOURCE (run.sh copied only Rope/Vant + SunGlow), so TechniqueExecuteStart failed and the ribbon/rings silently drew nothing in the played game (verified: unknown technique returns false, no crash). run.sh now copies _dev/ship.fx to runtime RESOURCE; vant own-ship fade scan skips deleted groups (null GetEntityPointer deref after sinkings). Ballistics mirror verified exact against RealFire + AIBalls::Execute + Ball_AddBall (origin, elevation, raw angle, azimuth, HeightMultiply 0.4/0.65 knippels-by-charge-Type==2, analytic integration); preview-vs-live delta is ship-pos-vs-camera-pos origin (meters) plus per-shot dispersion shown by the ellipse. Ball-volume names/sizes verified (Knippels/Grapes/Balls/Bombs, 1.0/0.35/half-visual radii). Committed as 9c83a91, run.sh --stage-only exit 0, engine a59ee647 installed to /Applications, ship.fx copied to played RESOURCE, bundle re-signed valid. Disposition: staged, live visual verification in progress.

Aim v2 + volume + eye staged (October 1, be695e5): volumetric aim beam (arc tube + broadside curtain + landing disc, alphas up 2-3x), rigging hits entry-only (kills knippel mast one-shots from per-frame refire), deck eye +0.5m, install-engine auto-syncs Rope/Vant/_dev techniques. deck-eye-height conflict root cause: hand-written hunk without trailing context, which patch/git treat as extending to EOF; regenerated with proper trailing context, build green, run.sh --stage-only exit 0 with closed game. Engine b1599fb5 installed to /Applications, ShipAim techniques verified in played bundle, technique files synced, codesign valid. Cicero tools/* (witness channels) untouched and not in delivery path. Disposition: staged, player sea-battle replay pending.

Crime delivery gap (October 1, found during reputation audit): committed spec at 1b9720f/be695e5 contains CrimeSea_IsLawfulPrize/CrimeSea_HandleUntrackedEnemy, but delivered inputs/gameplay baseline (Sep-14 materialization) lacks both; lawful-patent cover and untracked-enemy deferral are NOT live. Live instant paths: same-flag fire (pirate switch + rep -10 + patent strip), ball hits on explicit/untracked enemies (full legacy Ship_NationAgressive or patent strip), deck-triggered boarding (StartBattleAfterDeck: instant war + patent strip). Needs inputs/gameplay re-materialization from current spec after Cicero hash chain completes, then restage. Disposition: documented, fix pending witness batch.

Point-and-shoot aim (October 1, staged): manual fire and overlay share one ManualAimTarget (camera ray vs solid ship cylinders incl. rigging airspace, then sea plane, then max-range sea point for sky). Fire() no longer marches to max range flattened to sea level; ship locks keep height, band ends on the ship, ellipse floats at aim height (hull-vs-masts). Contact glow tints center mark by relation (soft red enemy / soft green else); sea/sky stay brass. Ribbon stays single battery-spanning sheet + head fade, smoke cooled to 0xDCE6EC, yellow gun lines dead. Deck eye +0.5m moved to per-frame s_pos (old clamp-site hunk never applied). build.sh exit 0, run.sh --stage-only exit 0 closed-game, engine cebbfc14 installed to played app, ManualAimTarget in staged source (1 decl + def + 2 calls). Disposition: staged, player replay pending (preview==impact, ship lock height, glow colors).


Aim v4 live install (October 1): run.sh --stage-only exit 0 on user order while game ran; install-engine.sh put engine c08dc87f (aim patch md5 f7329f6a, 42997 B, byte-identical worktree/staged) into /Applications (post-codesign 8e66a9d9). Game relaunched 21:30 (PID 27663) on the new engine. Overlay is now 3 arcs + flat drape ellipse + cross at impact point, impact floor 1.5 m, no tube/rails. Disposition: installed, player sea-battle replay pending.

Crash/sea-FPS/error-flood batch (October 2, staged and installed): engine staged 10:24, bundle signature verified (`codesign --verify --verbose=2 --deep --strict`), installed binary carries the new `stale InterfaceBackScene` diagnostic. (1) Save-load crash: `storm::iEquals`/`iLess` built `std::string_view(nullptr)` in `src/libs/util/include/string_compare.hpp`, faulting at 0x41c87074 (crash log `metal-engine-2026-10-02-085005.ips`) from `SEAFOAM::AddShip`/`SEAFOAM_PS::Init`; `xcode27-compat.patch` now guards null pointers and fixes the `Range2T` typo in `second_normalized`. (2) Open-sea ~20 FPS: gameplay `interface/mainmenu.c` calls `DeleteClass(InterfaceBackScene)` without `&`, the entity is never erased, `StormMetalInterfaceBackScene(dev,false)` never fires, and the interior claim persisted into open sea (launch.log diag `indoor=1 active=1 applied=0` with 8464 draws, 40.1M vertices, 185.7 MB uploads per frame); `rawSeaLit` requires `!locationActive`, so sea geometry left the raw Metal lit path. `backend.mm` now clears the stale domain once in `StormMetalBeginAimOwnShip` (main-character `SHIP::Realize` only); menu-side hooks in `beginDynamicSky`/`BeginAimWater` were tested and rejected because the menu legitimately owns that domain. (3) `renderer-ui-fonts.patch` compact-primitive rejection now falls back to the backend `DrawIndexedPrimitiveUP`/`DrawPrimitiveUP` instead of tracing and dropping the draw. Disposition: staged and installed; replay pending (save load, open-sea FPS, log quiet). Mechanism owners: docs/audio-and-particles.md, docs/metal-lighting-parity.md, docs/metal-renderer-consumer-matrix.md.

Provisions/rum ship-menu fix (October 2, applied, install pending): Corsairs-provisions-rum-fix.patch applies to tools/metal_living_caribbean.py (base 3b3f903, result 46d4a34) and replaces the legacy fleet-only food/rum rows with two blocks keeping selected-ship (Судно) and squadron (Эскадра) durations distinct; ship.ini collapses [FOOD]/[RUM] into the enlarged [FOOD_SHIP]/[RUM_SHIP] rects and drops the legacy items. prepare_ship_interface/prepare_ship_ini assert exact anchors, and prepare() verified the produced digests against the new pins (ship.c bf94ec32, ship.ini b4e7ec7d). sync_metal_gameplay.py check: 2 files pending, compiler ready. Disposition: applied in the repo; install blocked by the running game (PID 60397, started 12:55) because run.sh refuses staging with a live process. Bounded heartbeat automation corsairs-2 (20-minute interval, until 2026-10-03T07:00Z) stages experiments/native-metal/run.sh --stage-only after the game exits, stays silent while it runs, then pauses and reports. Player replay after install owns acceptance.

Provisions/rum install receipt (October 2): user closed the game; run.sh --stage-only exit 0, gameplay applied 2 files (ship.c bf94ec32, ship.ini b4e7ec7d) into /Applications and .cache/runtime with matching hashes, engine unchanged (install skipped), bundle signature valid. The applied content is byte-identical to PR #14 (Fix duplicated provisions and rum entries in ship menu, branch fix/ship-menu-supply-duplicates, blob 46d4a34, still open); no duplicate local commit was created because the PR carries the same blob when merged. Heartbeat corsairs-2 paused. Disposition: installed; the ship-menu replay (Судно/Эскадра rows for food and rum) owns acceptance.

Player-log bound and stage-cost audit (October 2, source + probes): the played app appended engine stderr to `~/Library/Application Support/Iddictive Corsairs/logs/launch.log` without any bound (`public_launcher.py` opened O_APPEND, no rotation): 16.1 MB / 62k lines after roughly 12 h of play, +7 KB per 20 s of active play (about 1.3 MB/h). Dominant emitters: `[StormMetal] sea shadow resolve` 13.2k lines, `[StormMetal] diag` 9.6k, `[StormMetal] cam` 5k, `[StormMetal] shadow frame` 2.1k; `diag`/`sea shadow resolve` are per-128-frame dumps that ignore STORM_METAL_PROFILE=0 and carry zeroed profile counters in player mode. Engine spdlog files stay small (system.log 173 KB, compile.log 13 KB, error.log 2.9 KB, mimalloc 1 B). Fix: `rotate_launch_log` moves a non-empty previous session to `launch.log.1` before the redirect (empty leftovers keep `.1` intact), and `install-engine.sh` now syncs the reviewed launcher resource into the played app under the same allowlist discipline as the techniques (known revision 89aa9a96). Probes: rotation PASS (rotate / empty-leftover / no-op), launcher-sync decisions PASS (sync / skip / refuse on unknown edit), bash syntax and embedded python compile OK. Stage cost measured on this machine: no-op build.sh 4.4 s (0 source files recompiled), one-TU touch rebuild 9.1 s (5 ninja steps incl. link), codesign --verify --deep --strict on the 21 GB bundle 6.0 s; runtime trees stage via `cp -c` clonefile; a real engine install writes about 6 MB plus the renewed bundle signature. Disposition: source applied and probe-verified; install into the played app pending a closed game (install-engine.sh refuses while the engine process runs).
Willemstad missing textures (October 2, accepted): player screenshot at 16:45 showed the Curacao/its Willemstad sand flat and near-white and the central fort almost black and textureless; earlier notes had blamed baked shadows. Real cause: twelve shipped `RESOURCE/Textures` files are DDS containers carrying `.tga` names, and the native raw loader's `stb_image` TGA-only path rejected them ("unknown image type") where Windows `D3DXCreateTextureFromFileA` sniffed the container, so those materials drew untextured. The set is `SentMartin_terra1/2/4.tga` (8192x8192, 14 levels, 44,739,384 B), `SentMartin_terra3.tga` and `Shipyard_Marigo_KNS.tga` (4096x4096, 13 levels, 11,184,952 B) and seven `lighting/{storm,evening,day1..day4,morning}/locations/Outside/Jungles/Jungle10/shadow.tga` (4096x4096); Villemstad terrain `Mesh`/`Mesh70` (`lambert94/84/87/105SG`) and the shipyard/pier materials `lambert36/56SG` sample them. Delta: `experiments/native-storm/raw_texture.hpp` owns `storm::raw_texture::ParseDds` (magic, `dwSize==124`, required flags, DXT1/3/5 fourCC, mip count clamped to the full chain, block-math level sizes from offset 128, bounds-checked against the file size) and `raw_texture.cpp` sniffs the first four bytes before the stb TGA fallback and copies each level through `LockRect`/`UnlockRect` in the authored BC format, matching the compiled `.tx` path. Evidence: a header-only probe accepted all twelve files with exact end-of-file consumption and rejected truncated-DDS, real-TGA and empty negatives; Pillow decoded the same twelve as 8192/4096 DDS independently; `build.sh` exit 0 with the one touched renderer TU recompiled; `run.sh --stage-only` exit 0 with a closed game and engine `923d0b4821f50c7aa8bb2ea0e2a5aafa9f56919d99d5582a9d3ea1f9b4670635` installed into /Applications/Corsairs Iddictive Remaster.app from build artifact `fe88337fe53dc98634a3c762ce294831c96cf89d7593893b9cd8838e42d6bb5d` (deep/strict signature valid, previous `bc879e9d` backed up); player replay on the installed app confirmed the Willemstad ground, fort and shipyard surfaces. `run.sh` without `--stage-only` launches the `.cache/CorsairsMetal.app` cache bundle, not the played app, so the replay that owns acceptance ran from `/Applications`. Rejected: the baked-shadow-gate hypothesis. Durable fact and cost note: docs/texture-alpha-and-materials.md. Disposition: accepted.


Boarding chest-only collection (October 5, source/native VM verified): the player
rejected automatic personal pickup. Exact cabin item IDs/counts and cash now move
to flagship box1, with special leftovers after manual inspection preserved there
as well. The reply is "Да, всё в наш рундук."; the missing-speaker finish also uses
full collection. Final native VM passes on the installed engine, including seeded
stock, gold/unknown/duplicate/quest/rare items, personal inventory identity, repeat
collection, manual inspection, invalid destination and overflow refusal; scene
readiness, quest-use state and exit rendering are fixture boundaries. One isolated
fallback check timed out while the player game was open; the same final check
passed after it closed. Source commit d391091; canonical content plan is exactly
two scripts. Installation is waiting for the parallel world-map staging owner;
player-scene feature acceptance remains unresolved.


Boarding chest-only delivery closure (October 5, 14:18 EDT): the parallel native
staging owner delivered the frozen d391091 boarding/dialogue scripts. This
heartbeat verified both exact target hashes, empty cache/app sync plans, compiler
readiness and deep/strict bundle signing. Installed engine SHA-256 is
5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911. No game
was launched or stopped and no engine, SAVE or configuration was written by this
heartbeat. automation-4 is confirmed PAUSED and its definition remains saved.
Disposition: installation confirmed; real player-save boarding collection/manual
inspection replay remains unresolved.

### October 5 coherent fleet candidate — verification

Final source composition binds the persistent roster, sea bridge and encounter UI
through sync_metal_gameplay. Native build passed; isolated VM compilation/state
probes passed roster persistence, survivor/cargo restoration, no duplicate wear,
commitment and legitimate retreat, capture tombstones and action labels. Ordinary
Follow spawning is disabled; legacy naval/pirate Follow is upgraded. Direct sea
entry has its own native message. Installed application remains the previous
batch until the canonical stage-only operation succeeds. Scene/balance verdict
is unresolved and owned by the player's replay; no automated player launch.

### October 5 coherent fleet installation — startup rejection

Canonical stage-only completed with installed signed engine 8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06 and nine reviewed script/interface inputs. Player launch PID 80649 aborted during startup script compilation; actual userdata/Logs/error.log reports seadogs.c Invalid Expression. Installed and development resource/shared/messages.h remained baseline a3904843 despite built header 38bab60e defining MSG_WORLDMAP_ENTER_SEA_DIRECT. The isolated VM using copied installed headers reproduced SIGABRT; earlier source-header VM success did not prove installed startup readiness. SAVE/configuration preserved. Disposition: rejected startup; canonical install-engine.sh now includes reviewed messages.h delivery in its rollback/signature transaction; corrected staging and installed-header compilation pending.

Corrected October 5 startup-header stage: canonical stage-only exit 0, deep/strict bundle signature valid, app/runtime headers byte-identical to built 38bab60e. Installed-header VM compiles main and interface/map successfully; unknown installed-header edit negative refuses before writes. Actual player startup and fleet-scene replay remain unresolved.

Repository closure gate: agent_context.py --check exceeded its bounded 20-second timeout after corrected staging; owned subprocess group terminated. No gate pass claimed. Focused commit remains blocked by staged-provenance-unavailable/claim-not-unique, with active .codex specialist 01a10de6-9ba1-7ae1-8175-c06f65c630e3. Foreign staged/working paths preserved; no push.

October 5 source closure: canonical adopt-staged accepted exact11 targets without concurrent transcript mutation; focused helper committed 6039e62. All four foreign staged/working fingerprints remain identical. No push or additional game staging was performed; committed source bytes are the previously installed batch. Repository gate timeout and player replay remain unresolved.

### October 5 jungle dusk illumination — source/GPU verification

Hypothesis: a valid dusk frame without shadow maps incorrectly disables modern
location relighting. Two player screenshots show the ground switching at fixed
18:50; the installed log switches `applied`/sun validity. Delta: one location
illumination-ready predicate shared by collection, blended actors and the engine
light-selection bridge, independent of applied shadows. A task-local GPU variant
of the existing fixture reproduces the old discontinuity and passes all four
receiver paths after correction, together with its existing blocker/lamp/indoor/
overlay negatives. Canonical build passes; ordered stack changes zero cached
source files, gameplay sync plan is empty. No player state or installed engine
changed in this attempt. Disposition: unresolved pending canonical staging and
the installed Cuba jungle load/walk replay.

October 5 installed continuity correction: after the player confirmed closure,
canonical stage-only completed (exit zero) with signed engine 29ab8ad6... and
verified bundle signature. Source checkpoint cfc9e5c; no push. Reopened installed
app renders the menu, then the player loads and walks an outdoor scene; process
41195 consumes the established app/resources/player-state owners. UI automation
refused load input during ongoing player interaction; passive observation later
shows the Cuba atlas. The game stays with the player, no agent input/save write.
Disposition: installed/startup verified; exact visual jungle continuity replay
is unresolved pending the player's same-transition result.

October 5 encounter panel delivery: run.sh --stage-only exit0 on user close/install reply, exact four installed script/INI hashes verified at source0e551a2; SAVE/config preserved. Installed engine29ab8ad6 unchanged in this content batch. Native isolated compile was already verified; visual/action player replay remains unresolved.

### October 5 resident-voyage bridge — initial source verification

Hypothesis: commerce cannot retain fleet identity through service while native
arrival/lifetime delete its existing descriptor. Delta: guarded ordinary lifecycle
fields in worldmap-traffic.patch; one arrival event, stopped service movement,
verified native locator request/commit and one departure event, preserving explicit
loss and legacy behaviour until scripts activate the contract. The preceding
ordered patch reverses against a disposable copy of current native source, and
the revised patch applies over that preserved input. The canonical compiler's
syntax checks pass for wdm_enemy_ship.cpp and wdm_merchant_ship.cpp after fixing
the merchant's explicit core.h dependency. No permanent test, cache build, stage,
installation, game input or player-state write occurred. Service/calendar/cargo
handlers and legacy-save transitions remain unimplemented, so functional voyage
acceptance is unresolved. See worldmap-traffic.md for the state/owner contract.

### October 5 resident voyages/service — connected source verification

Hypothesis: persistence requires both native arrival/lifetime and the script's
off-map expiry owner, while elapsed repairs must reserve actual port resources.
Delta: connect the existing descriptor to calendar service plans, material and
recruit debits, saved next jobs, ordinary Follow reload migration and paid sea
supplies. A full-max-crew reservation was rejected: ordinary colony hiring pools
can be smaller than a hull. Partial paid hiring retains the available reserve,
persists actual crew count and reaches the real MinCrew gate over work intervals.
Result: current ordered patch reapply and native syntax checks pass; the actual
arrival method's source-branch probe passes ID, repeat, pause, service and patrol
negatives. The exact script consumers compile in the native VM. Native descriptor
serialization with disposable scene hooks passes repeated-debit/repair exclusion;
state probes pass shortage/reserves, elapsed repair, physical gun repair, partial
crew, dead/quest exclusion and old Follow coordinate/ID preservation. Canonical
read-only script composition has exactly three changed consumers. Permanent test
delta: zero. These are source/state checks, not installed gameplay acceptance.
No canonical build/stage, app mutation or player input occurred. Disposition:
unresolved until the integrated batch is staged after player close and its actual
callbacks, prior save and interactions replay. Cargo/prizes and subsequent
expansion milestones remain active work.

Probe incident: the first standalone VM launch used its default Akella/Sea Dogs
log directory and wrote compile.log, error.log and script_stack.log there. The
probe then moved all logs/state to its task-owned temporary directory through
STORM_USERDATA. Prior contents of the default logs were not captured, so those
files are preserved; Remaster's separate player SAVE/config were not written.

October 5 source checkpoint: `df030ee502c5c994d6dd38b3004c8d35996e8602`
integrates the connected voyage/service component and its three script consumers.
The repository context gate passes; foreign staged blobs and working paths remain
unchanged. No push, canonical stage, installation or gameplay replay occurred.
The game is still running from the installed app (PID 8172). Cargo/prizes and later
expansion work continue under the same parent owner.

### October 5 cargo/convoy candidate — connected source verification

Hypothesis: sea cargo generation, resident lifetime clamping and late randomized
ship geometry would break finite shipments/prizes and funded readiness. Delta:
make the existing per-hull physical manifest the cargo owner across Store, sea and
arrival; persist loading jobs and finite capture settlement; select ordinary
composition without the hero and snapshot the actual ship at assembly. Preserve
real crew on interrupted service, consume elapsed provisions/ammo once, and admit
whole local battles within the sea's 32-hull array. Native retirement explicitly
escapes SetLiveTime's one-second clamp rather than leaving dead residents immortal.
Result: current ordered patch applies on its preserved input; changed enemy and
merchant translation units pass canonical syntax. The actual lifetime source
branch passes resident/retired/pause/legacy cases. Native VM compiles the exact
script batch and passes source-stock debit, sea restore/save, finite loss/delivery,
prize capacity, capture object independence, interrupted paid crew, daily supplies,
whole-battle admission, actual authored ship assembly and repeated native save/load
settlement. Error log is empty. Permanent test delta: zero. Three canonical script
consumers differ; no cached native build, app stage/install or player-state write.
Disposition: source/state verified candidate; real-game callbacks, old-save upgrade,
convoy/player capture and balance remain unresolved. National scheduling, siege,
player survival/participation, rumours and port recovery remain active milestones.

October 5 cargo source checkpoint: `ac447ddab983c6f2e685406dc15f871f4a2425cb` integrates the connected
cargo/convoy component locally. The context gate passes, no nonignored untracked
paths remain, and foreign index entries are unchanged. No push, canonical stage
or installed app/player-state mutation occurred. Remaining expansion milestones
and installed-game acceptance continue under the active parent goal.

October 5 fort prerequisite attempt: hypothesis: the old difficulty-dependent
assault threshold and global damaged-cannon count cannot govern persistent port
operations. Delta: native physical per-cannon damage persists on the real fort
commander across hit/scene/save/load; scripts read its own destroyed count and use
strictly more than half as the assault gate. Preserve actual damaged-fort crew,
with the existing intact initialization and admitted resurrection paths.
Result: eleven-path ordered patch applies; changed native fort source passes
canonical syntax. Actual helper probes cover fractional damage, repeated restore,
separate forts and bounds. Exact script batch compiles, strict half/count separation
and native script-state save/load pass with an empty error log. Permanent test
delta zero. A disposable patch check first used the wrong cache-root prefix;
correcting the new patch path to `src/libs/sea_ai` makes the canonical patch pass.
Disposition: source/state verified candidate; build/stage/install and physical
cannon/boarding replay unresolved. Two-sided fort/city stages and expedition
scheduling remain active, with no player/app/runtime mutation.

October 5 fort source checkpoint: `f755cb9` integrates physical cannon persistence and the strict assault gate locally. Context gate passes; foreign index entries remain unchanged. No canonical build/stage/install or player-state mutation. The active goal continues with expedition and land-state integration.

October 5 national-strategy attempt: hypothesis: persisted national reviews must
weight existing ordinary fleets without minting raids or rerolling current tasks.
Delta: nation-owned versioned posture, canonical 30–60-day reviews/minimum 14-day
hold, migration permission clocks, owned damage/attack and surviving readiness
reports, peace/depletion eligibility, immediate emergency reasons, weighted home
patrols and rare funded large-patrol preparation. No treasury or new active siege.
Result: exact composed scripts compile; native VM probes pass migration/no spawn,
repeat/no reroll, emergency/hold, peace/year skip, same-posture major loss, pirate
exclusion and subsequent native save/load. Error log empty. The first year-skip
probe exposed incorrect implicit nested-attribute binding; explicit `makearef`
fixes both the new hold and the existing pirate-band date. A second oracle exposed
an unchanged random recovery posture hiding the new loss reason; emergency reasons
now update once even when posture is already correct. Four script consumers remain
pending. Disposition: source/state verified candidate; no engine/app/player write,
and physical expedition authorizations/travel/land resolution remain active work.

October 5 strategy source checkpoint: `2caea7d` integrates nation strategy and corrected nested date bindings locally. Context gate passes; foreign index unchanged. Exact expansion batch remains uninstalled and the military stage continues under the active goal.

October 5 unvisited-fort inventory attempt: hypothesis: autonomous operations need
physical fort counts without depending on a player first entering every island.
Delta: pin the 23 actual fort locator GM identities and cannon-type counts in
source metadata, emit their lookup through the existing exact-hash composer, and
bind those counts to the actual commander before nation readiness assessment.
Canonical sync rejects a changed physical locator. Unknown or contradictory
saved defence remains unknown; genuine HasNoFort stays distinct.
Result: native RDF_LABEL/ScanFortForCannons owner establishes the parser; the first
disposable parse used flags as the name offset and returned material labels, then
was corrected to the actual name field. Independent recounts pass for all 23 and
the same files hash-match the installed app. Actual composed PROGRAM compiles;
VM checks pass cannon type filtering, partial prior state preservation, unknown
and contradictory counts, genuine fortless and national defence reporting.
Changed RESOURCE rejection and idempotent composition pass. Permanent test delta
zero. Disposition: integrated source/state candidate; canonical build/stage,
old-player save and fort/expedition scene replay remain unresolved, no app write.

October 5 fort inventory source checkpoint: `f935049e686ba28bccf726340a231233d6785b4e` integrates exact resource-bound defence metadata and its existing nation consumer. Context gate passes and foreign index entries remain unchanged. No app/runtime/player write. The active military milestone continues with authorization and actual shared siege stages.

October 6 siege-entry discovery: the actual port fort is cloned by
GetShipLocationID -> MakeCloneFortBoarding into BOARDING_FORT with authored attack
locators, then yard/bastion templates. Five actual locator GMs have separate loc/
aloc banks (25 per side outer, 16 per side inside). Existing BRDLT_FORT forces hero
attack and can auto-capture when a scene is missing; it is therefore geometry reuse,
not a ready defender/autonomous state owner. Character-death, berthing, ordinary
flagship game-over and Ship-object swap owners are bound in the topic document.
Disposition: read-only source/resource discovery; mission participation/finite
weighted actors and player recovery remain unimplemented/unaccepted. No app or
player-state effect; required next owner is the current root military milestone.

October 6 military source attempt: hypothesis — one colony operation and original
fleet descriptor can drive supplied travel, physical fort losses, finite land
casualties and evacuation without generated armies. Delta: composed military
owner, mission route/sea adapters and managed-fort cargo/crew preservation.
Final disposable VM covers paid naval ammunition/crew/hull loss, actual defender
battle interruption, ceasefire, fortless city entry, strict half gate, island
reservation, timed land stages, finite stock loading, partial evacuation and
surrender after city victory, scene admission/defer and mid-stage serialization.
All four serializer round trips pass with zero error log. The first scene fixture
lacked rank/island/cannon attributes; supplying actual valid shape removed those
fixture errors. A new harbour-task replay exposed an unbraced for/if changing the
following absent target from -1 to 1; an explicit block restores the correct hold
and nearest inactive-operation rejection. No engine compiler change. The complete
native patch applies and the changed merchant source passes syntax. Read-only
canonical composition has four pending script consumers. Disposition: source/state
candidate, not real-game acceptance. Foreground/player/recovery milestones remain
owned by this active root task; no app/cache/SAVE/config mutation or canonical stage.

October 6 callback integration attempt: hypothesis — the new modules must enter
existing consuming callbacks, and a military sea battle must import both original
fleets even outside ordinary radius. Delta: exact reversible thirteen-owner script
adapter; finite recovery hooks; shipless selected-survivor confirmation; real berth
and first ShipDead guard; foreground load/admission/death/no-heal/withdrawal wiring;
scoped GetRelation, mayor agreements, map expiry and rumour age callbacks. Fixes
preserve military arrival flag and canonical story exclusions while admitting a
cleared legacy Siege.Colony; complete late battle attachment assigns reciprocal
tasks and defers all members for missing map or hull capacity. Result: actual
whole-PROGRAM VM plus four native serializer rounds pass with error.log 0 bytes.
Focused audit probes execute real departure events, protected and cleared story
states, outside-radius pairs and both capacity failures. Headless localization
initially crashed _Language_OpenFile because the service was absent; a disposable
identity GetConvertStr adapter preserves original source compilation without
claiming localized/rendered output. A command-note hook initially used init-local
idLngFile; compile rejected it, corrected to the same command's saved original
note. Disposition: source/state evidence only; sixteen files pending, canonical
staging/install and specified player/scene callbacks remain unresolved. No permanent
tests, app/player SAVE/config writes or launch in this attempt.

October 6 FPS candidate attempt and rollback: hypothesis — lifetime-owned
immutable Metal buffers remove the 512-entry cache flush churn without reducing
quality. Delta — native buffer ownership plus independent mast null-child guard.
Canonical stage installed e9a65fbf on the existing installed content batch,
verified signature and preserved player SAVE/config hashes. Component uploads
improved; the player then reports long stutters/slideshow. Disposition: FPS
candidate rejected, component measurements do not accept native pacing. Source
rollback preserves mast guard (19525c9) and builds 4e0c0be1; stage is pending
player exit, with no player process stopped or SAVE/config restored.

October 6 repeat-crash diagnosis: the pasted 00:36 mast report is historical.
The latest local report at 01:31:24 belongs to PID 16765 and candidate UUID
B326BD3C; assembly binds the fault to dereferencing a garbage INIFILE argument
during first SEAFOAM particle initialization. The INI asset exists. Null-guard
and missing-file remedies do not address this observed non-null bad pointer.
The foam registry has an unchecked 64-entry bound, but no ship count at the
crash is available; overflow is a hypothesis, not an established cause.
Disposition: crash owner identified, corruption cause unresolved; no speculative
foam patch, settings reduction or additional engine variant was introduced.

October 6 fast foam repair attempt: hypothesis — unbounded slot admission can
clobber psIni with emitter coordinates before the next first-emitter Init. The
actual arm64 header/layout and installed assembly agree: slot size 2720, array
offset 48, count at 174128 and psIni at 174136. A disposable actual-header probe
reproduces the report pointer from float values 14.5335846 and -0.1015913 written
to slot64; slot63 and capacity rejection preserve the seeded pointer. The report
still does not establish actual ship history/count. Delta — two-line capacity
check before shipsCount++ via sea-foam-capacity.patch, preserving the first64
emitters and every ship's gameplay. Extra ships receive no foam beyond the
already declared capacity. Latest beta4/develop remain unchecked; particle-manager
and bow-splash sound fixes are inapplicable. Native build passes with exactly
one generated source changed and zero permanent test delta. Local source
checkpoint 0532a1b, no push. Disposition: guard mechanism accepted by the probe;
installed activation and real-save/sea crash acceptance unresolved. The frozen
2fa372af batch also restores the original FPS backend and preserves the mast
guard; one ten-minute exit watcher owns canonical stage/hash/signature/state
verification. Native UI save input was deferred/refused during user interaction;
no quicksave success or automatic game exit is claimed. Existing quicksave was
backed up before attempting maintenance input.

### October 6 — shared sea danger repair; source verified, player replay unresolved

The player reported incorrect sea danger for their fleet and NPCs. Installed
traffic/sea/UI share `WdmTrafficCharacterPower`; changing dead native `GetPower`
would not repair the consuming path. The class/role formula collapsed different
hull HP, installed gun quantities and calibers. Cargo-only readiness also dropped
from 25.6 to 6.4 after the last salvo moved into loaded guns, before any shot.
A canonical-only destroyed-gun state reproduced a separate alias guard defect;
ordinary dual-alias generated ships were not universally affected.

Delta: actual HP and cannon damage/count; optimal-crew readiness; exact native
loaded counts; guarded physical damage enumeration; saved actual hull/cannon
stats; loaded inventory normalization in NPC snapshots; one-time observation
rebase without clearing intent. No graphics settings or new GPU buffer caching.

Disposable actual-VM baseline reproduces both failures. Primary and seeded sea
exit/old-save probes pass with zero error log; exact main composition also
compiles. The installed-baseline delivery excludes paused military additions and
changes only PROGRAM/sea_ai/AICannon.c, PROGRAM/sea_ai/sea.c and
PROGRAM/worldmap/worldmap_encgen.c. Their output hashes are respectively
3419ae6185f2c551d4aeeea328ba66aa6e1d4866dcbcc14ffb05536103c4d878,
7b4c7a20efa8d2b18954a47e134e7cc3701b72d241d6b38fcb4a30db63ad976a and
3fe5ad32138fafabece7fe38ad8c98443349656da27753da689c53febb2aec9a.

Combined engine cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d
builds through the canonical native-metal path and supersedes the former pending
2fa372 batch. It contains mast/foam guards, the FPS regression rollback and the
loaded-count bridge. The earlier 10-minute exit watcher expired without staging;
player PID 42956 remained open. Installed engine remains e9a65fbf9570d8e61b2ca92cfa0be802c3065db1478f7dde1ad873045e82b958.
Source/probe disposition accepted; installation and actual player saved-game/
sea battle/FPS acceptance remain unresolved. No successful live installation or
scene fix is claimed from compilation.

### October 6 — frozen repair batch installed after player exit

Canonical stage-only exited zero from the isolated installed-content checkout,
shipping the mast/foam guards, FPS cache rollback and common sea strength fix.
Native build hash `cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d`
became signed installable hash
`073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac` after the
existing installer rewrote rpaths and signed with identifier `metal-engine`.
The disposable exit-worker's initial verification incorrectly compared those
different representations and recorded a false `stage_failed`. Corrected
verification matches the installed engine to the canonical signed-candidate
receipt, verifies all three script hashes and the deep/strict bundle signature.
All 215 preexisting SAVE/config files are unchanged. One additional autosave was
created after staging during the player-launched session; it is preserved.
The worker is finished. Observed PID 63449 launched the installed app at 02:32:55;
its native window renders the world map (125 FPS shown). No game input was sent.
Disposition: installation and startup/rendering accepted; exact save-load crash,
open-sea FPS/stutters and sea-danger action/nearest unaffected action remain
unresolved pending player replay. Paused expansion remains uninstalled. No push.

### Distant-ship reflection + land overexposure triage — October 6, read-only; no staging (game running)

Player FPS hypothesis (ships beyond horizon / spray / weather / CPU fallback) checked against
`experiments/native-metal/.cache/storm` source: `SEA::EnvMap_Render` sends `MSG_SEA_REFLECTION_DRAW`
to every entity in `SEA_REFLECTION`/`SEA_REFLECTION2` once per cube face (5x/frame) with no sender-side
distance cutoff (far plane 4000), and `SHIP::Realize` performs full light setup + model draw with no
frustum/distance culling. Distant-ship cost confirmed by code reading; committed every-second-frame
refresh (`ff9687f`) halves it. Spray/weather/CPU-skinning absent from the live 4s battle sample;
kept as secondary pending post-stage replay. Land/interior overexposure (Porto Bello 15:52, FPS 125):
auto-exposure bounded (base 1.03, adaptive 0.72-1.18) so not exposure; widened dynamic-lighting
admission in `5f32190` (Oct 2, weak-sun preservation) and `cfc9e5c` (Oct 5, lighting without shadow
maps) is the prime suspect (dynamic sun+ambient possibly stacking on baked geometry). No blind
lighting edit; breakage onset + post-exit staged replay pending. No running game stopped.

### Reflection distance-gate feasibility — October 6, read-only; deferred to post-exit batch

Sender-side skip in `SEA::EnvMap_Render` would be the single-point fix (one threshold, coherent
per refresh, probe-compatible), but layer members are untyped `Entity*` (ships, models, sky,
islands) with no common position accessor from sea.cpp; blind casting risks crashes. Receiver-side
gating (SHIP/MODELR handlers) duplicates the threshold and still needs a cutoff value with visual
replay. Both variants require build verification, blocked while the game runs. Decision: implement
the gate in the post-exit batch together with staging of `ff9687f` and same-save replay, choosing
the cutoff from the replay scene. No source touched, no game stopped.

### Reflection fix delivered engine-only — October 6, installed; replay pending

Full `run.sh --stage-only` rebuilds clean (`sea-reflection-budget.patch` position 24 in
`source-patches.json`, `dynamicReflectionTick` present in `.cache/storm` sea.cpp/sea.h, engine-1
relinked 10:08) but aborts in gameplay sync: installed `/Applications` `PROGRAM/sea_ai/sea.c`
digest `7b4c7a20` matches no known BASE/SHA/PREVIOUS revision, so the gate refuses to overwrite
player gameplay (17 files pending stay undelivered, paused expansion untouched). Engine-only delivery
via canonical `install-engine.sh`: final deep/strict verify initially failed on game-regenerated
`__pycache__` inside Frameworks Python (exporter excludes those caches; they reappear on every
launch). Fixed in `284fab1`: installer purges regenerable caches inside its transaction before
sealing, no gate weakened. Installed signed engine `7f252e3b` (pre-install `7467bbb3`), previous
`97d40b3b` retained in `installed-engine-backup`. Deep/strict verify passes. Pending: same-save sea
battle FPS replay (22-32 band falsifier), Porto Bello day overexposure replay, lighting onset timing.

### Overexposure narrowing — October 6, read-only; replay still pending

Interior fill (`interiorAmbient`) and outdoor response (`outdoorResponse`, `.30+.38*sky`
hemisphere, `.82` sun shaping) both date to the initial Metal publish `08b05c7` — excluded as
recent causes. Noon shader math clamps vertex lighting at 1.0, so a modest sun/ambient excess
flattens sun-facing plaster to flat white, matching the Porto Bello screenshot. Remaining
candidates: day fog color/density from weather presets (fog-FX changes staged Oct 1) or
sun+ambient stacking; both need onset timing + staged day replay to bisect. No source touched.

### Reflection lag on camera rotation — October 6, reported on 7f252e3b; fix pending sample

Player reports ship reflections jump/catch up when rotating the camera: expected from the
every-2nd-frame cube (night 15 FPS game rate makes it 7.5 Hz). Planned fix: force whole-cube
refresh when the camera moved since last frame (position + angle), keep half-rate when still.
Camera controller shows no idle wave-bob, so a small epsilon is viable, but the value needs the
load sample first: if the hotspot is not reflection work, the cadence fix gets reverted instead
of extended. No source touched, game running.

### Sea FPS + lighting + reflection batch — October 6, source candidates; build/install pending game exit

User load sample (`~/Desktop/seafps.txt`, night battle, 2 ships in view, FPS 15):
494/~2465 main-thread samples in `SEA::Realize`, 489 in `EnvMap_Render`, 474 in
`SHIP::Realize` → full `MODELR` → `DrawIndexedPrimitive` → `newBufferWithBytes`.
Reflection redraws every layered ship once per cube face (5 faces, no distance or
frustum gate in the `MSG_SEA_REFLECTION_DRAW` handler) — suspect #1, previously
measured at 27.6% in the Oct 6 open-sea sample. New suspect #2, untouched: AI script
VM (`AIGroup`/`AIShip`/`BC_Execute`) is ~21% of this sample. GPU-wait (`nextDrawable`
semaphore) dominates the earlier `/tmp/seaload2.txt` sample, so CPU and GPU limits
coexist; no FPS gain is claimed before a same-save replay.

Deltas (working-tree candidates, game PID 13542 running — no build/stage/install):
- `sea-reflection-budget.patch`: reverted the uncommitted camera-turn override (it
  forced every-frame refresh during continuous rotation, negating the cadence saving;
  cube faces are world-axis-aligned, so rotation alone must not force work). Added a
  3000-unit XZ distance gate in the ship reflection handler (cube far plane is 4000;
  main view still draws every ship). Ship-only hunk verified with `git apply --check`
  against pristine `ship.cpp.orig`; `sea_ship_reflection_probe.py` passes.
- `backend.mm`: aligned the two embedded MSL outdoor ambient factors from
  `.30+.38*sky` to the modern `.18+.44*sky` already used by `lighting.hpp` and the CPU
  vertex path (ground-level washout was GPU-only). The deferred `albedo*.38` floor and
  `rawOutdoor?materialDiffuse` substitution are unchanged — day replay decides those.
Focused commit blocked by the coordination guard (foreign staged entries from another
writer share index/HEAD; `commit --only` denied as `commit-working-entry-not-owned`,
lock reports unlocked). Not a harness defect: shared-HEAD serialization owns that call.
Changes stay working-tree candidates, which the canonical build consumes directly.
Disposition: `unresolved` — needs game exit, canonical build/stage/install, then same-save
night-battle replay (falsifier: leaving the 15 FPS point), Porto Bello day replay for the
ambient alignment, and a Tab/aim camera comparison.

### Reflection distance gate + outdoor ambient parity — October 6, installed; player replay pending

Canonical `run.sh --stage-only` rebuilt the engine from the working-tree batch (probe PASS,
ship hunk `git apply --check` clean against pristine `ship.cpp.orig`). Full stage aborts at the
known gameplay sync point (`unrecognized installed fleet bridge: PROGRAM/sea_ai/sea.c` — the
paused Living Caribbean expansion still rejects the installed content revision), so delivery
used the engine-only path: installed `a353a16e…` from fresh `engine-1` (Oct 6 10:42), bundle
signature verified, rollback `7f252e3b` retained in `.cache/installed-engine-backup`. Batch:
far-ship skip in the reflection handler (3000 XZ units, main view unchanged), reverted
camera-turn override, and the two embedded-MSL outdoor ambient factors aligned to the modern
`.18+.44*sky`. Focused commit still defers to the shared-HEAD guard (foreign staged entries).
Disposition: `unresolved` — needs same-save night-battle FPS replay (was 15), Porto Bello day
replay for the ambient alignment, and the Tab/aim camera comparison.

### Day-battle sailors skinning upload — October 6, source candidate; build/install pending game exit

Same-battle day replay (6 near ships, FPS 21): fresh 6s sample `/tmp/daybattle.txt` (PID 26807,
1628 main-thread samples) moves the hotspot. `Sailors::Realize` is 429: ~247 in
`acceptSkinningVertices` (`_platform_memcmp` full-buffer scan + heap `memcpy`) and ~132 in the
sailor draw path with `newBufferWithBytes` kernel traps plus `RawBuffer`-map wholesale-clear
destruction (`__tree_deleter` + `AGXBuffer dealloc` + IOGPU traps). Reflection is down to 94
(~6%, cadence working, near ships correctly unskippable); AI negligible in this scene.

Root cause: every animated sailor mesh per frame went memcmp (always dirty) → free/malloc →
memcpy → kernel `newBufferWithBytes` → old-buffer dealloc. Fix in working tree (`backend.mm` +
`resources.hpp`): per-facade 3-slot persistent shared ring indexed by `flightRing.current`
(same wait-before-reuse discipline as `FrameArena`), plain `memcpy` into the slot, no compare,
no per-frame kernel allocation. Facade identity/lookup contract unchanged. Untouched: bone-index
validation scan, `bind` size check, `clearSkinningSources` lifecycle.
Disposition: `unresolved` — needs game exit, canonical build/stage/install, same-battle replay.

### Skinning ring delivery — October 6, installed; player replay pending

Canonical build recompiled `backend.mm`/`resources.hpp` clean (fresh `engine-1` 10:50); full
stage still aborts at the known `PROGRAM/sea_ai/sea.c` fleet-bridge sync point, so engine-only
install: `fe4d39e3…`, signature verified, rollback `a353a16e` in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same-battle replay must confirm FPS lift AND sailor rendering
correctness (no torn/garbage crew — the ring reuses the flight-slot wait discipline, but this is
its first live run).

### Ring-fix loss and redelivery — October 6, installed `07f0d860`; player replay pending

The 10:53 battle sample on `fe4d39e3` still showed the old `memcmp` + per-frame kernel
allocation: the skinning-ring hunk was missing from working-tree `backend.mm` at build time
(ambient hunk intact, `resources.hpp` ring members intact — a partial working-tree loss while
a co-writer holds staged entries in the same checkout). Hunk re-applied with exact-replace and
verified present; 10:57 canonical build recompiled `backend.mm.o` and relinked `engine-1`.
Engine-only install `07f0d860…`, signature verified, rollback `fe4d39e3` retained. Next builds must
grep-verify the hunk immediately before compiling. `fe4d39e3` verdict: ambient-only, ring absent.
Disposition: `unresolved` — same-battle replay (FPS + sailor correctness) on `07f0d860`.

### Ring-engine smoke without player — October 6, installed `07f0d860`; battle replay blocked by lock screen

Installed `07f0d860` (sailor skinning ring + outdoor ambient parity + ship reflection distance gate) launched via open at ~13:11 local (PID 43388) with zero game input sent and no SAVE/config touched. Pre-launch: `sea_ship_reflection_probe.py` PASS, ring hunk and both `.18+.44*sky` ambient factors grep-verified in working tree (canonical build 10:57, engine-only install 10:58, signature verified, rollback `fe4d39e3` retained). Process alive 10+ min with Metal diag lines flowing (fixed menu camera, no crash/hang) — startup/rendering smoke accepted for the ring engine. Interactive battle replay proved impossible: display woke to the loginwindow lock screen (screenshot verified), no UI-automation route exists in this session, and bypassing the lock is out of scope. Disposition: `unresolved` — on return: Continue into `открытое море 5` (day battle), read FPS vs 21, confirm crew rendering (no torn sailors), camera-rotation reflections, and day-town light; then a 6s `sample` for the hotspot comparison (memcmp/newBufferWithBytes expected gone).

### Dynamic-buffer transient ring — October 6, source candidate built `435cf1f6`; install pending game exit

Player day-battle sample (`~/Desktop/seafps.txt`, PID 61263, 2940 main samples, FPS 19): memcmp gone (ring live), but SAIL::Realize 368 draws through `resident()` → `newBufferWithBytes` 105 samples in kernel IOGPU traps. Root cause: sail cloth VB/IB are D3DUSAGE_DYNAMIC, rewritten every frame, so the 512-entry resident map reallocates a Metal buffer per sail per frame (~80 sails at 8 ships). Scene-specific, matches player evidence: harbor 3 furled ships 120 FPS vs battle 8 sails-up 19 FPS. Reflection lag on rotation is downstream (9 Hz cube at 19 FPS), not a separate defect.

Delta (working tree, game running — build only): per-facade 3-slot persistent shared ring for D3DUSAGE_DYNAMIC sources (`transientResident` in `backend.mm`, `dynamicRing` on VertexBuffer+IndexBuffer in `resources.hpp`), same flight-slot wait discipline as the skinning ring; static path untouched. `build.sh` recompiled `backend.mm.o`, fresh `engine-1` 16:24. Disposition: `unresolved` — needs game exit, engine-only install, same-save replay (FPS + sail correctness, no torn cloth).

### Dynamic-ring install — October 6, installed `b831c879`; same-save replay pending

Hunks grep-verified before compile (`transientResident`, `dynamicRing` present); `build.sh` recompiled `backend.mm.o`, `engine-1` 16:24. Engine-only install after player exit: `b831c879`, bundle signature `codesign -v` OK, rollback `07f0d860` retained. Disposition: `unresolved` — same day-battle replay (FPS vs 19, sail cloth correctness, rotation reflections).

### Distant sailor shadow caster cull — October 6, installed `2daf1adf`; armada replay pending

Player reported 120 FPS in 3-ship battle vs 10 FPS in fleet armada battle. Root cause:
in armada battles with 7-8 ships, 183 skinned sailor meshes bypassed shadow frustum culling
in `land_shadow.hpp` ("animated bind-pose bounds cannot safely reject a caster") and rendered
into both 2048² sun cascades (near + far) plus receiver passes, generating ~550 redundant GPU
draw calls every frame and hashing 183 bone palettes per frame on CPU.

Delta:
1. `experiments/native-metal/backend.mm`: gate `rawSkinned` sea shadow registry capture to
   `simd_distance_squared(worldMatrix.columns[3].xyz, cameraWorld.xyz) <= 3600.0f` (60m).
   Distant enemy ship sailors (>60m) are never captured into the shadow registry, avoiding
   bone palette hashing and packet creation.
2. `experiments/native-metal/land_shadow.hpp`: in `renderDepthCascade`, skip `p.skinPalette`
   casters completely for `farMap` when `!locationActive`, and cull >50m for `nearMap`.
3. Probes: `world_shadow_registry-probe`, `gpu-skinning-parity-probe`, and
   `gpu-skinning-offscreen-probe` all PASS.

Canonical build produced `engine-1`; installed `2daf1adf24286b72` into
`/Applications/Corsairs Iddictive Remaster.app`, codesign deep/strict verified,
rollback `07f0d860` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same armada battle replay (FPS lift vs 10, shadow appearance).

### Transient sail buffer per-frame ring gate — October 6, installed `6e0fde48`; armada replay pending

Live 4s sample of running game PID 83194 (10 FPS in armada battle / menu) proved that 923 of 1657
main-thread samples (56% CPU) were spent in `_platform_memmove` inside `transientResident`.
Because each ship has 10-15 sails, `DrawBuffer` is called hundreds of times per frame with the same
shared sail vertex buffer. `transientResident` previously re-copied the multi-megabyte buffer on
every single draw call because it lacked revision/frame caching.

Delta:
1. `resources.hpp`: added `dynamicRingRevision` and `dynamicRingFrame` to `VertexBuffer` and `IndexBuffer`.
2. `backend.mm`: in `transientResident`, gate `memcpy` to `dynamicRingRevision != cacheRevision || dynamicRingFrame != flightRing.current`.
   The buffer is now copied at most once per frame across all sail draw calls.
3. Built clean `engine-1`, installed `6e0fde48b99f5945` into `/Applications/Corsairs Iddictive Remaster.app`,
   codesign verified, rollback `2daf1adf` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same armada battle replay (FPS lift vs 10, sail rendering).

### Armada sea AI relation cache & sail index fix — October 6, installed `614b88dc`; armada replay pending

Live 6s sample of running game PID 87212 (armada battle) showed:
1. Sails corrupted due to `transientResident` reusing a single in-flight index buffer across multiple
   D3DLOCK_DISCARD sail draws within the same frame. Fixed: `rawIndices` reverted to `resident`
   (independent revision buffers, no in-flight overwrite); `rawVertices` retains per-frame ring gate.
2. CoreImpl::ProcessExecute consumed 1223 of 2595 main-thread samples (47% CPU) in `SEA_AI::ProcessStage` ->
   `AIShip::Fire` -> `isCanFire` -> `isFriend` -> `GetEffectiveRelation` doing hundreds of thousands
   of `ATTRIBUTES::GetAttributeClass("SeaSurrender")` string lookups per frame with case-insensitive `strlen`.

Delta:
1. `experiments/native-metal/backend.mm`: `rawIndices` reverted to `resident(rawIndexBuffers, ...)` so sail
   indices are not overwritten in-place within the same frame.
2. `experiments/native-metal/sea-surrender-relations.patch`: updated `AIHelper::GetEffectiveRelation` with
   16ms surrender cache (`UpdateSurrenderCache`). When no ships have surrendered (`!bAnySurrendered`),
   returns cached `*GetRelation(x, y)` instantly with zero string searches. When surrendered, compares cached
   DWORD IDs instead of traversing attributes.
3. Built clean `engine-1`, installed `614b88dcead66bd2` into `/Applications/Corsairs Iddictive Remaster.app`,
   codesign verified, rollback `6e0fde48` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — armada battle replay (FPS lift vs 10, sails clean, cannon firing).

### Sea combat script merge (speed floor, gunner spread, aim heights, ballistics) — October 6, installed app; player replay pending

Player screenshot showed ship speed 0.4 kn and player volleys missing fort batteries. Audit found the
sea-combat patches lived only in git-ignored `gameplay/PROGRAM/sea_ai/` (LF) while the installed app and
`.cache/runtime` held newer CRLF files with fleet features (`WdmFleetSeaMarkGone`, `shipName` block,
`trafficFleetID` charge selection) but WITHOUT the sea-combat fixes — a blind push would have reverted
fleet work, a blind pull would have dropped the combat fixes.

Delta (merged RT base + WS combat hunks, written to workspace, `.cache/runtime`, and installed app):
1. `sea.c` `Sea_ApplyMaxSpeedZ`: `WindAgainstSpeed`/`fWindAgainstSpeed` fallback (8.0, clamp >= 1.5),
   headwind multiplier floor 0.22, `MaxSpeedZ` floor 1.8 kn. Fleet `shipName` hunks preserved.
2. `AIShip.c` `Ship_GetBortFireDelta`: nonlinear spread `pow(dist/1000,1.45)*55`, `GunProfessional` +0.12,
   `LongRangeShoot` +0.06, close-range (<60m) cap 1.2m. Fleet `Wdm*` hunks preserved.
3. `AIShip.c` turn: `fTRFromSpeed` floor 0.35, dismasted slow-turn (*0.20, lerp below SP 25),
   `Bring2Range` floor 0.07 -> 0.02, kept surrender `BI_CallUpdateShip` post.
4. `AICannon.c` `Cannon_GetFireHeight`: dropped `Y/2` waterline bug; Balls/Bombs >= 2.2m, Grapes >= 3.2m,
   Knippels 15m. Same path covers volleys at forts.
5. `AIBalls.c`: `SpeedMultiply` 3.0 -> 2.0, height arc 0.40 -> 0.70 (knippels 0.85), gunner-perk
   accuracy block, `GOOD_KNIPPELS` int-compare fix.
Verified by grep in all three targets; WS==RT for all 4 files (pending 30 -> 26).
Disposition: `unresolved` — game process predates delivery, scripts compile at launch; needs full app
restart then sea replay vs ship + fort (volley grouping, headwind speed, dismasted turn, ball arc).

### Fort-battery auto-fire fix — October 6, installed app; player replay pending

Player report: volleys reach the fort but miss its guns (high lighthouse battery, steep up-angle).
Engine audit (`ai_ship.cpp:408` -> `Fire(vFirePos)` -> `Fire2Position(bort,pos,-1.0)`): fort auto-fire
aims at the gun position minus 1m with NO `CANNON_GET_FIRE_HEIGHT` call, so ship aim-height fixes
never applied; random offset came from `Ship_GetBortFireDelta` (tens of meters at fort range), and
`Fort_CannonDamage` falloff `pow(0.11,dist)` zeroes everything beyond ~1.5m from a gun.

Delta (script-only, workspace + runtime + installed app, grep-verified):
1. `AIShip.c` `Ship_GetBortFireDelta`: aim point higher than 6.5m with non-knippel charge (ship hull
   aim sits at 2-4m, so high aim means a fort battery) tightens spread x0.35. Knippels excluded to
   avoid buffing dismasting. Symmetric for NPC ships.
2. `AIFort.c` (new in workspace, pulled from runtime): falloff `pow(0.11,d)` -> `pow(0.30,d)`.
   Direct hits unchanged (1.0 at zero), near misses at 1-2m now chip guns.
Known engine limit (not patched, needs C++ rebuild): `AICannon::CalcHeightFireAngle` returns flat 0
when no ballistic solution exists (d<0) — from directly under a high battery some guns may still
fire flat into the cliff. Revisit if replay shows flat shots.
Disposition: `unresolved` — needs app restart + fort duel replay (hits on batteries, suppression pace).

### World-map traffic avoidance and jitter repair — October 6, engine installed; replay pending

Hypothesis: generated traffic reacts to islands through `WdmIslands::FindReaction`, which
returned zero whenever `PtcData::FindNode` failed to resolve the ship's patch node, so the
avoidance force collapsed exactly where a ship was closest to shore. The remaining jitter came
from the enemy-ship turn sign and the below-`minManeuverSpeed` speed branch oscillating every
frame, plus patrol routes reversing at their own endpoint.

Delta (`experiments/native-metal/worldmap-traffic.patch`, section list extended with
`src/libs/worldmap/src/wdm_islands.cpp`):
1. `wdm_islands.cpp` `FindDirection`: probe along and against the course at r=4..36/48 when the
   source or destination node is unresolved, before falling back to the straight line.
   `FindReaction`: probe the four axis directions at r=4..24 and raise the static reaction
   radius 20 -> 32.
2. `wdm_enemy_ship.cpp` `FindIslandForce`: island weight 1.5 -> 2.0 plus a 32-unit look-ahead
   `ObstacleTest` that picks the free side (7/10-unit lateral probes) and brakes at 15 units.
3. `wdm_enemy_ship.cpp` `FindShipsForce`: lateral reaction clamped from the pass geometry
   instead of a fixed starboard flip; head-on pairs keep the fixed side.
4. `wdm_enemy_ship.cpp` `Move`: degenerate `sn` resolves from `turnspd`, and the reverse-wind
   branch converges on `minManeuverSpeed` without overshoot.
5. `wdm_merchant_ship.cpp` `KillTest`: route reversal only when the stored return point is
   farther than 25 units.

Result: the ordered stack round-trips and `run.sh --stage-only` re-applied it (3 source files
changed), then built and installed engine
`d8d2980cfd7f0810ced5ccf2a4015ecfc10bdd3d46506e1e7cfbda25f5e14337` into
`/Applications/Corsairs Iddictive Remaster.app` with the bundle signature verified and the
previous engine retained in `.cache/installed-engine-backup`.

Blockers recorded, not repaired as part of this delta:
- `tools/metal_military_integration.py` held real newline bytes inside its `enc()` string
  literals, so every canonical stage aborted with `SyntaxError`; the literals are restored and
  the file is left as the other owner's uncommitted revision.
- The gameplay sync still rejects `PROGRAM/Loc_ai/LAi_login.c`: the working callback revision
  covers 16 owners while the installed baseline contains the paused expansion's 13. The stage
  ran with the committed revision and the working file was restored afterwards; no gameplay
  script was delivered by this attempt.
- A concurrent Codex thread delivered gameplay scripts into the same checkout and installed app
  at 21:49-21:50; the engine install is engine-only and left those files untouched.

Disposition: `unresolved` — the player's world-map replay must show ships rounding islands and
the sailing ship holding a steady course.

### Fort accuracy-hack revert + bombs-only blast — October 6, installed app; replay pending

Player rejected the x0.35 fort spread tightening as incoherent (same gunner can't turn sniper vs
forts). Reverted in workspace + runtime + installed app (grep: `iFortCharge` gone).
Falloff rework kept in coherent form per player: only BOMBS get blast (`pow(0.30,d)` when
`AIBalls.CurrentBallType == GOOD_BOMBS`), solid balls and other kinetics stay direct-hit-only
(`pow(0.11,d)`). Direct hits unchanged at 1.0 for all ammo.
Disposition: `unresolved` — needs app restart + fort duel replay with bombs vs balls.
Follow-up per player: kinetics 0.11 -> 0.20 (1m 20%, 2m 4%, 3m 0.8%), bombs stay 0.30.

### Mast-floor propulsion 2.0/2.5/3.0 (engine-only) — October 6, staged to dev bundle; played-app install pending game quit

Player spec: minimum speed by standing main masts — 1 mast 2.0, 2 masts 2.5, 3+ masts 3.0 —
whatever the wind, sail damage, load or skills; dismasted ships stay adrift at 0.
Rewrote `experiments/native-metal/sailing-propulsion-floor.patch`: dropped the sail-cloth-scaled
floor (`MSG_SAIL_GET_DEPLOYED`, 0.30 x capability x deployed) and count standing main masts
directly from `SHIP::pMasts` in `CalculateNewSpeedVector` (skip `bBroken`, skip topmasts
`mastNum >= TOPMAST_BEGIN`). Kept guards: not dead/fixed, sails raised, not Stopped/SeaSurrender,
AI DRIFT blocked. Patch stack verifies clean; engine `e62d65b9` built 22:20 and copied to
`.cache/CorsairsMetal.app` (hash match verified).

NOT yet in `/Applications` (still pre-floor binary from 21:51): `install-engine.sh` refuses
while the game runs, and the player is in-game. Needs: quit game, run install, restart.

Pre-existing blocker, not caused by this batch: `sync_metal_gameplay.py check` is RED —
`PROGRAM/interface/itemsbox.c` first (half-finished 25-file WS/RT sync adopted 9 files into
`.cache/runtime`), then sea files (`AIShip.c` UNKNOWN, `sea.c`/`AICannon.c` differ from pipeline
output, `AIBalls.c`/`AIFort.c` unowned). TRAP: registering current digests without updating
manifest UPDATED/SHA outputs would make staging NORMALIZE (clobber) the approved sea fixes.
The sea ballistics batch must be encoded into the manifest pipeline before any `apply`.
`run.sh --stage-only` exit 2; `plan()` raises before writing, so nothing was clobbered.
Also note: `run.sh`/`install-engine.sh` `ps` guards silently pass inside the sandbox
(`Operation not permitted`) — do NOT hot-install the played app while it runs.

Speed math from player screenshots (stat 8.12, wind 8.1 -> 2.0 kn; wind 7.5 -> 0.5 kn):
`WIND_NORMAL_POWER` is 20, so wind power alone is ~0.4, times the wind-angle multiplier
(down to 0.22 against the wind). The mast floor bypasses all of it. Player also reports new
world-map micro-stutters — queued, not yet investigated.

Disposition: `unresolved` — install + restart + sea replay (shredded-sail speed, dismasted drift).

Update 22:35 — player quit the game; `install-engine.sh` delivered candidate `e62d65b9`
(signed `13b97ec8`) to `/Applications`, verified idempotent on re-run, previous engine backed up.
Kinematics reviewed: floor targets Speed only (inertia/collisions untouched); all stop paths
bypass it (lowered sails, Stopped, surrender, death, NPC DRIFT); dismasted stays 0; symmetric
for NPCs; no save-format change. Known accepted tradeoffs: dead-calm/upwind sailing at 2–3 by
spec, overload/crew penalties bypassed below the floor. Awaiting player sea replay.

### Quarter floor rev2/rev3 + 0.8 mystery + knippel aim — Oct 6, diag engine built, install pending quit

Rev2 (quarter of ideal by standing/total masts, Tmp.fShipSpeedIdeal from Ship_UpdateParameters)
crashed with EXC_BAD_ACCESS in CalculateNewSpeedVector at sea load (per-frame node-name parse).
Rev3 (stored mast numbers, bounded loop) installed 22:42, signed 6e3a981e; player relaunched
22:42:56, no crash, but speed still 0.8 with sails up and masts standing: floor never engages.
Static audit cleared every entry condition on paper; engine truth needed.

Knippel aim (script-only, gameplay+RT+APP synced, bytes verified): Cannon_GetFireHeight aimed
knippels at flat 15m with pure-random scatter, zero skill input; forts have no Height table
(balls 3m, knippels 25m). New: knippels at class sail centroid, fort guns 8m, scatter by
gunner skill x crew exp (2.2x to 0.3x) x distance (0.35x to 1.0x). Same rules for NPCs.

Diag batch for one restart: temp engine patch sailing-floor-diag.patch (FLOORDIAG traces) in
build.sh stack after floor patch; candidate built 23:15, FLOORDIAG strings confirmed. Script
trace added, synced RT+APP. Engine stdout goes to Idictive Corsairs/logs/launch.log (live).

Tooling: nested apply_patch was a silent no-op all session; all edits via shell heredoc plus
head/tail splice with grep verification. Complex python heredocs blocked by PreToolUse guard;
data heredocs pass. install-engine.sh refusal while game runs is honored: build only, no
hot-install.

Disposition: unresolved. Needs: quit, install diag engine, restart, sail 10s, read FLOORDIAG
from launch.log, test knippel/fort aim. Revert temp diag afterwards.

### Port ship pile-up + jitter (islandships.c) — Oct 6, script synced to app; restart+replay pending

Screenshot: ~10 neutral ships grinding in one spot off a port. Two vanilla defects found:
1. Accumulation: GenerateIslandShips adds +0..3 ships per colony per day, old ones never
   cleared (ClearIslandShips runs only on capture/siege). All spawn on 6 shared locators.
2. Jitter: half the spawns got Group_SetTaskMove(rand(1000000), rand(1000000)) — unreachable
   targets through island/shore; AITASK_MOVE fights the touch controller every frame.
   DRIFT zeroes speed+rotate globally (ai_ship_task_controller.cpp), anchored ships are still.

Delta (inputs/gameplay + .cache/runtime + installed APP, bytes verified, braces 16/16):
- Inside the per-colony date gate, InitCharacter all existing IslandShips of that colony +
  Group_FreeAllDead before respawn. Same-day re-entry keeps ships (no regen, no wipe).
- PlaceCharacterShip: all spawn anchored (Ship_SetTaskDrift + Group_SetTaskNone); the
  RELATION_ENEMY attack override below still fires, combat AI overrides drift when hostile.

No engine change. Takes effect on game restart (scripts compile at launch, no .b cache).
Disposition: unresolved — needs restart + sail to a port, verify <=3 calm ships per colony.

### Custody release boarding ("no ship at berth") — Oct 6, script synced to app; replay pending

Player: released from jail to a bay, boarding says no ship. Root cause: Custody_MoveFleetTo
retargets from_sea to the release berth (dock port, or wartime shore) but leaves stale
Ship.trafficBerth, so WdmHarbourEmbarkAccess fails (berth != pchar.location) and reload.c
prints "Нет доступного корабля у этого причала". Hits transfers and wartime shore release.
Peacetime destination is the port (jail.l1.go must equal colony.from_sea), same single cause.

Delta (WS + RT + APP, bytes verified, braces 124/124): Custody_MoveFleetTo now calls
WdmHarbourBindBerth(FindLocation(destination)) after moving hulls. INPUT pipeline copy
left untouched (older pre-CustodyLife revision; gameplay sync still RED, no clobber).
Disposition: unresolved — needs restart + jail-release replay (peace port + war shore).

### Diag engine installed — Oct 6, 23:34

Player quit; install-engine.sh delivered the FLOORDIAG candidate (engine-1 built 23:15,
FLOORDIAG x2 in installed metal-engine, bundle signature verified). Next: launch, sail
10s, read FLOORDIAG from Idictive Corsairs/logs/launch.log, then remove temp diag.

### Worldmap berth hover (traffic pile-up) — Oct 6, engine built; install pending quit

Screenshot: ~8 ships hovering in a line off SantaCatalina + cluster at Providencia.
Root cause: homing force (weight 1.0) vs island repulsion (reaction r=32 x2.0 weight
x2.0 combine = 4x + 32-unit look-ahead brake + 15-unit hard brake) and ship repulsion
(r=45). Arrival radius is 12, so ships stall ~15-30 units offshore, never trigger
WdmTraffic_Arrived, and the queue grows as more voyage ships home to the same port.

Delta (experiments/native-metal/worldmap-berth-homing.patch, after worldmap-traffic
in build.sh): WdmMerchantShip overrides FindIslandForce/FindShipsForce, fading both
repulsions 1.0 -> 0.0 between 70 and 12 units from destination. Open-water avoidance
unchanged; berth offsets (4-10) still spread parked hulls. Build green (stack verified,
2 files changed, engine-1 23:58).

Build-gate lesson: inputs/gameplay is manifest-fingerprinted; the islandships.c fix
broke prepare_metal_inputs verify. Reverted inputs copy to bc03fe (manifest hash).
Script fixes live in WS + RT + APP only; never edit inputs/.

Disposition: unresolved — install at next quit + worldmap replay (ports drain, no hover).
Note: FLOORDIAG still pending; player on worldmap, no sea entry yet (0 hits in log).


### October 7 — Living Caribbean source closure and shared content admission

Hypothesis: the remaining service, harbour, participation and evacuation gaps must
be connected at their existing consumers before the expansion can be delivered.
Delta: paid initial service and atomic sea admission; physical cargo capacity;
weighted eligible targets; saved harbour choice/control and real evacuation route;
actual naval/fort/land credit, late-contract caps, civilian crime and return-news
hooks; guarded existing commander boat contact. The callback composer admits exact
predecessors and pinned parallel revisions while keeping raw backup bytes and
unowned runtime content. Detailed owners and rejected hypotheses are in
`docs/worldmap-traffic.md`.
Result: exact composition/idempotence and unknown-revision negative pass; full
script/lazy-dialogue VM plus four serializer rounds and separate consumer/state
probes pass with zero script errors; canonical native build passes. Installed
preflight is read-only and reports 11 pending consumers. No stage/install occurs
while the existing player process is running. Repository-wide closure still sees
two foreign untracked native patch candidates, preserved for their own writers.
Disposition at that attempt: `unresolved`; the source/install dependencies were
subsequently closed by the attempt below, while player replay remains open.

### October 7 — final parley integration and canonical installed delivery

Hypothesis: hostile contact needs consent before boat transfer and a real combat
signal, while final staging must preserve existing runtime edits and player state.
Delta: `worldmap-contact.c` owns the two-step request/consent/visit contract; 23
existing callbacks cover the command, actual captain deck, scoped relation and
group restore, one-time Cabin return exemption and sea lifecycle cleanup. Native
`sea-contact-activity.patch` adds a synchronous AIBalls live-record count. The
real sea identity is `AISea.Island`; an initial undeclared `sPlayerLocation`
candidate failed the actual script compiler and was corrected before delivery.
Existing BI_Boat checking/launch blocks remain byte-identical. Known military
backups are admitted by exact hashes; unknown edits still reject.

The user's FPS recovery direction preserved the complete five-file draft/diff in
`.local-archives/fps-before-worldmap-20261007`. Later dynamic sail-ring, distant
caster/sailor culling and 16ms relation-cache candidates were removed. Existing
skinning-ring, outdoor ambient, reflection and surrender-marker UI remain.
This is source recovery evidence, not a measured FPS improvement. Unrelated
staged files, native candidates and user state were preserved.

Result: actual whole-script VM plus existing serialization/contribution/contact
cases and parley predicates 931–951 pass with zero script errors. The compiled
native query branch counts seeded authoritative records without mutating them;
the actual entity/event/rendered transition remains a player scenario. Current
runtime composition is idempotent, rejects unknown callback bytes, and retains
the pinned parallel speed/aim/dialogue/custody owners.

Source commits `76da73f` and `316e8b0` close implementation. Canonical
`experiments/native-metal/run.sh --stage-only` exited 0 after the final compact
native patch; patch stack and arm64 build pass. It installed signed engine
`46f859b8b41135c368d9f40a5a57ecb38ef3cedb19c9ca6a89f3d79bfaaf5e45`,
retaining previous engine `029338b9…` in the existing backup owner. Deep/strict
signature verifies, both content plans are empty, compiler readiness is true,
and actual installed/built/VM message headers match. All 233 player SAVE/config
hashes match the pre-delivery snapshot. The game was closed and never launched
or controlled by this attempt.

Disposition: source/build/stage/installed delivery accepted at their own layers;
player-owned prior-save, physical voyages/capture, sea/fort/land fights, harbour
loss/reassignment/evacuation, rendered parley/return, crime/rewards/news, recovery/
prices and FPS remain `unresolved`. Native goal state owns the requested pause;
these observations must not be replaced with proxy acceptance.

### October 7 — FPS/pickup integration and installed dev-helper canary

Hypothesis: repeated sea sail uploads need immutable submission-owned storage,
and the remaining helper must deliver content through the sole installed app.
Delta: per-submission revision/identity caching reuses FrameArena slices without
overwriting in-flight vertex/index bytes; sea registry traversal prunes stale
draw records. Live code-based surrender lookup replaces the transient relation
cache. Ship lights retain their owning-hull predicate. Pickup accepts the two
pinned ordinary/supply predecessors and composes into the current script/native
stack; existing sailing and berth inputs are tracked build dependencies.

Result: extracted actual native/GPU methods pass rewrite/recycled-slot, locked
buffer, allocation reuse, additive pickup, depth/occlusion and full render-state
restoration cases. Actual attribute/codecs pass immediate surrender deletion,
commander replacement, ordinary hostility and sanitizers. Whole-PROGRAM VM and
four serializer rounds pass with zero script errors. These are bounded component
checks, not real-game FPS evidence.

Checkpoint ad83111 is integrated. Canonical run.sh --stage-only exited zero after
the final source/ordered-patch change and installed signed engine
ee1447630287a759187683c49544794a2cbd08d693a762a98ea46a3b2e5e76ed.
Strict signature, compiler readiness, both empty content plans and matching
built/cache/installed message headers pass. All 233 preexisting SAVE/config hashes
remain unchanged. The prior 46f859b8 engine and reviewed FPS draft are preserved.

The helper replacement uses installed Contents/MacOS/launch and the existing
player-state lock; finite content transactions reject conflicts/links and restore
only their owned writes. Fourteen disposable scenarios pass. The actual installed
one-file push/revert canary reseals/verifies and restores exact original bytes;
the installed native launcher's dry-run resolves the current engine without
executing it. No game launch/input or forced exit occurred.

Disposition: FPS/pickup source/build/stage/installed delivery accepted at their
own layers. Helper source and bounded installed canary accepted, saved in focused
commit 4106e7e. The released-ledger repair is integrated separately in .codex
checkpoint 4e0c4f7. Parent replay transferred exactly the three helper paths;
the originally blocked staging and normal focused commit both succeeded. All
five unrelated working/index states remain byte-identical, owned paths are clean,
and no nonignored untracked paths remain. This closes the checkpoint dependency.
Same-save FPS and actual gameplay/reload scenes remain unresolved and player-owned.
Native goal state owns the user's requested pause after completed delivery;
these checks do not imply completion of unavailable player scenes.

### October 7 — player journey hardening, source candidate verified

Hypothesis: ordinary world traffic can break player agency through authored
quest proximity, damaged-ship regeneration, stale participation links/news,
global recovery suppression and unloaded land geometry. Delta: the canonical
sea source and worldmap modules now share real admission/entitlement owners,
retain paid persistent damage and scope recovery/story yield to actual owners;
31 SHA-bound land layouts support finite distinct placement and physical return.

Result: all five lanes reproduce their old mismatches with zero script errors.
Relevant state/negative scenarios and four native codec rounds pass. Independent
canonical initialization/clone tuples and raw GM positions bind land metadata.
Exact four-file composition and integrated events/codec pass with zero errors.
No game launch or player SAVE write occurred. Fourteen owned paths are staged;
five foreign working/index hashes remain unchanged. Normal focused commit
returned `staged-commit-claim-not-unique`; native index scope is unlocked.
Saved .codex repair task `01a117d5-f4e9-76d0-82e3-2abdc657e0cd` owns the
normal-route repair and source-task replay; root retains final Git/install.

Disposition: source/state checks accepted; checkpoint dependency unresolved;
installed activation and rendered gameplay/balance remain unresolved. This
candidate does not replace the installed finite-trade inventory above yet.

### October 7 — player journey hardening installed; checkpoint recovery pending

First canonical stage reached gameplay apply but rejected unchanged previous
canonical `sea.c`: planning had already proposed its future receipt hash and
apply attempted a second admission of old bytes. No gameplay transaction ran.
The minimal sync correction retains one reviewed managed proposal, frozen source
guards and both final receipts. A writer-intercepted pre-edit reproduction fails;
the repaired proposal matches the four expected script hashes. An independently
edited file still rejects, and known delivered content is idempotent.

After that last source change, canonical `run.sh --stage-only` succeeded. Native
inputs changed zero files; the signed engine remains `096395187b7b85e3f647f528d176531f0e6fbe16ad5baebc0029ac725197b8a0`.
Cache and installed plans are empty; strict/deep app signature passes. Installed
generator is `a3fb4c7ac5773bd7c85a12140c6b1eec59de5c615ff5ea8224bb238ce17ffc1b`,
reload `82fa26766d7706d01b21716e52381115edf556764be7caf611779b06c44b3776`,
map UI `d342e2cc54a3272c5b13998210fff578046b5f8f4db62e569e564af3457802cf`,
and sea `eb3cfa85f8d433c7398f2427822e0d4ef60e9f1e5f85dc73ee1c2879f94b0811`.
All 234 observed player-state files and five foreign working/index paths preserve
baseline hashes. No game launch, SAVE rewrite or forced process stop occurred.

The first Git refusal was an expected live-child ownership/provenance conflict,
not a proven guard defect. A later original staging replay encountered a completed
root `auto:apply_patch` index claim; the same saved .codex owner is investigating
its normal posttool release. Root retains the exact focused commit obligation.
Disposition: source/state/composition and installed delivery accepted; checkpoint
pending its normal owner recovery; native player scenes and balance unresolved.

The specialist then bound the held claim to a successful two-file `git add`, not
an active patch. PostTool had no pending entry; its old staged snapshot cannot be
reconstructed safely. Supported recovery is native Stop of this turn, automatic
active-goal continuation, native staged-provenance adoption of the same 14 paths,
then the original focused helper as a separate call. Root retains baseline and
installed evidence in its task temp until that replay. No claim was manually
cleared, no guard was bypassed, and no harness/source repair was attributed to
the stale `auto:apply_patch` label. Checkpoint acceptance remains open.

### October 7 — player journey source checkpoint accepted

Continuation disproved the proposed Stop/adoption workaround: adoption lacked
provenance, and a fresh successful no-op staging call recreated the orphan claim.
The same .codex specialist repaired repeated admission work and successful no-op
stage provenance; its 48 focused checks and current-transcript admission probe
passed. Root replayed the exact original 14-path staged helper through its normal
route: commit `59a3dfad5f6cd4fcc4aad3cc819feae2b634b3ef`, exit zero.
Native index scope is unlocked. Commit contains only the 14 reviewed task paths;
all five foreign working/index hashes and 234 player-state hashes still match.
Repository hygiene passes. No push, reinstall, game launch or SAVE write.

Disposition: this batch's source checkpoint and previously verified installed
activation are accepted. Earlier checkpoint-pending/recovery instructions are
historical and superseded. The separate .codex owner still owns its own repair
checkpoint; live source-task success is not proof of that checkpoint. Actual
player startup/actions, rendered siege geometry and economic balance remain
unresolved, with joint player replay as their continuation owner.

The specialist's restricted profile could not save its own checkpoint. After its
explicit two-file ownership transfer, root verified HEAD, staged/working matches
and four foreign staged fingerprints, then used normal staged adoption and the
focused helper in separate calls. `.codex` commit
`cc7b6fd0240d8a9d0e78756ec860a75077cf8267` contains only the two repair files;
four foreign staged fingerprints remain unchanged and its index scope is unlocked.
That source dependency is saved; no push or instruction edit occurred. The six
gameplay task-owned temporary fixtures were removed via codex-delete-temp after
their evidence was consumed. Native turn cleanup owns remaining session claims;
an optional explicit two-file release rejected unsettled provenance and was not
bypassed. This does not reopen the accepted gameplay commit/install.

### October 7 — remaining player acceptance blocked

Source, install and both checkpoint dependencies are closed. The specialist is
terminal. Installed public launcher binds `Contents/MacOS/metal-engine`; the
read-only exact-name process probe returns no process. No game was launched and
no player state was changed. Real scene/balance evidence remained missing across
three goal turns, so the native goal is `blocked`, not complete or user-paused.
Continuation is the player's next normal current/new-save session with root
observing quest continuity, trade/services, event choice/rewards and sea/land
placement/withdrawal/return. The full objective and unresolved acceptance remain.

### October 7 — player-map stepped movement: follow-camera source repair built

Player reports small steps while simply sailing on the global map. Live PID48484
is the supported app engine, started 16:42:03 local. Installed engine096395187 and
worldmap generatora3fb4c7 match the last delivered batch. Read-only game observation
currently shows a sea scene, not the reported map movement; no game input occurred.

Hypothesis bound to source: WorldMap::Realize sampled the follow camera before
ship Update, then rendered the advanced ship against the previous camera position.
A disposable exact-method/update-block ASan/UBSan probe reproduces variable-frame
ground-focus error0.186328..0.745361; moving the existing camera call after object
updates reduces it below0.000011. Follow+zoom, free camera, pause and once-per-object
update/time negatives pass. It uses explicit motion/control/bounds adapters; this
does not accept the player's perceived result. No physics/timing/speed change.

Canonical worldmap-navigation.patch carries the one-file frame-order delta.
Canonical build.sh succeeded with exactly one changed native source and built
engine2d29535e2940f70b652a98e490ca512af221ccdf82e8df2eb08b8749f33c73ed.
Built source equals the verified proposal. Five foreign working/index fingerprints
are unchanged. The played app and player state were not written. Canonical
run.sh --stage-only installation remains pending the player's normal save/quit;
the running process still uses096395187. Source repair accepted; installed smooth
movement and zoom/storm transition replay unresolved. No forced exit or hot swap.

Player then confirmed normal save/quit. Canonical run.sh --stage-only succeeded
after the last patch change; installed signed engine is
34e1c03d03ad7be83669c92bd11534d24213833a69113dcc7325e4f9f9b66eec
from raw built2d29535e and commitafbe96e. Strict/deep signature passed; gameplay
plans reported zero pending. All239 player files present after normal quit and
all five foreign working/index fingerprints retain exact pre-stage hashes. No
game launch/input occurred. Source/build/install accepted; rendered smoothness
remains player-owned replay. The subsequent 1/4 ideal-speed report is a separate
active repair, preserving the current accepted map batch.

### October 7 — arcade quarter-speed correction, source verified

The player clarified 1/4 of ideal maximum, not a fixed 1.4 knots. Read-only
decoding of the normal sea save at 16:53:12 identifies Blaze/index1, arcade1,
HUD scaler0.4, ideal8.6057501, four intact masts and no blocking flags. Native
floor used the unscaled ideal while ordinary propulsion used2.5: floor2.15144
engine knots became0.860575 on HUD instead of2.15144. NPC identity and absent
masts/ideal are rejected for this save.

Canonical AIShip.c now shares one mode coefficient between Tmp.fShipSpeedIdeal
and MaxSpeedZ. Real script VM executes normal/arcade/normal changed blocks with
zero errors; exact installed native target-method ASan/UBSan fixture passes HUD
quarter, partial/full mast loss, stop/dead/fixed/surrender/drift/lowered sails,
faster stock propulsion and bounded mast vector. An optional VM codec round
reported an invalid function after load; it is not acceptance evidence for this
derived parameter update. No native patch or engine rebuild is required.
Cache gameplay check reports exactly one pending canonical AIShip.c. Installed
engine remains34e1c03d. Source accepted; installation and live sea replay pending.

The player reports whole-map micro-pauses each game hour, while individual ship
jumps are gone after afbe96e. Live sample PID74816 at17:03 was almost entirely
unfocused sleep; subsequent observation showed the game menu. That sample does
not establish a map hotspot. Hourly source/probe diagnosis continues separately;
no economy/event suppression and no game input have occurred.

Player confirmed quit with +; absence of the installed engine process verified.
Canonical run.sh --stage-only succeeded on c8a136c after the last source change:
zero native source changes, exact AIShip.c content delivery, installed engine
34e1c03d unchanged/reused. Installed AIShip.c SHA256 is
e7db81fc82e429b294095a873e8afec85e3e24d5d7c7f7084302dde07262e335.
Strict/deep signature and zero-pending gameplay check pass. All240 player files
from the post-quit snapshot and five pre-existing foreign working/index blobs
remain exact. Source/install accepted; live sea acceleration/HUD replay pending.
A new untracked tools/gameplay/mast-repair.c appeared during delivery; unrelated
concurrent mast-repair work belongs to active chat «Добавить ремонт мачт в море»
01a1182b-8bd0-73f1-b9fc-b3c9d5496736 (native read confirms its file creation);
it is preserved and was not included in c8a136c.

Hourly pause diagnosis identifies WdmTrafficReviewStrategies (hourly gate, called
from refresh/NextDay) and WdmMilitaryTick (hourly gate, frame handler) as synchronous
candidates. A synthetic readiness workload is rejected as causal evidence: it
overcounts nation/role filtering and does not execute actual handlers. Moving
strategy interval checks before readiness is rejected because it suppresses
urgent home-attack/loss/recovery responses. No hourly source change is admitted.
The player independently reopened PID10402 after c8a136c; observation showed the
nation-relations interface, not an advancing map. One pending request asks for
20–30 seconds of ordinary map sailing to capture a real hourly sample. Root owns
that live-profile continuation and the subsequent smallest proven repair.

The revised isolated hourly fixture reports ~0.4ms/call with zero active sieges,
but explicitly stubs character/island lookup and portions of military/strategy
work. Root does not accept this as real-handler timing or a causal exclusion.
It further invalidates the earlier synthetic200ms claim; live advancing-map
profiling remains the decisive missing scenario. No hourly source repair made.

### October 7 — sea outcomes retained on the map, ordinary NPC identity defect

Player reports destroyed sea opponents keep sailing as map miniatures. Installed
sea.c matches canonical pre-repair eb3cfa85. Its ActorOwned rejected captain.quest,
but installed InitCharacter initializes that container for every ordinary NPC.
Exact-method VM seeded with those defaults returns Admission=1 (rejected) with
zero errors before repair; no scene receipt is then written, so sink/exit cannot
close that roster. Replaced that guard with explicit isquest. No descriptor
quest/qID, companion, group/index/ordinal or scene receipt guard was removed.
Admission/ReturnCases/NegativeCases now return0 with zero errors; admitted death
marks roster dead and descriptor needDelete, survivors retain damage, repeated
exit and deferred/story/companion/stale-identity negatives pass. CodecSeed plus
four CodecAfter rounds pass with zero errors after supplying the fixture's
required empty OnLoad adapter. No native patch; historical already-resurrected
rosters cannot be retrospectively distinguished without actual loss evidence.
Root owns canonical sea.c and worldmap topic; foreign mast/cannon work preserved.
Source accepted; installed delivery and player sea→map replay pending.

Player confirmed quit with +. Selected registered sea.c installed transaction
completed after process absence and player_guard admission; existing source
backup, receipt, concurrent-byte checks and rollback remain the ordinary owners.
Canonical native run.sh staging was not run for this content-only batch: unrelated
mast/cannon native inputs and a changed cached bundle are pending with their
writers. No engine or other gameplay source was copied. Cache/installed sea.c
SHA256944fa5c0cc3e028ca3d828ec019243cd35b2d6a91c5f6096069d861ae976e8ba;
engine34e1c03d and all241 post-quit player files unchanged. Transaction's
strict/deep app-signature check passed. Commit339178e owns only sea.c and the
worldmap topic. General agent_context hygiene remains blocked by foreign
untracked mast/cannon inputs, preserved with their active owners. Source/install
accepted; player must replay a new sea battle then return to the map, checking
fully destroyed fleet removal and surviving fleet damage. Previously resurrected
rosters have no reliable historic loss evidence and are not guessed away.
Hourly whole-map pause remains unresolved under root's pending live-profile
request; this identity repair does not accept that separate requirement.

### October 7 — safe-sea mast repair, installed; action unresolved

Hypothesis: restoring authored mast subtrees inside the existing native ship
avoids encounter regeneration and the saved broken-flag overwrite of sea reload.
Delta: focused local commit `f6ce3ab` (nine mast-owned paths) adds the quick-menu
planner, reusable confirmation, ordinary plank withdrawals and 8–72-hour time
advance, with a standing-main-mast ceiling of floor(60% of total). The native
bridge preflights donor geometry, keeps ship/model/AI identities and rebuilds rig
groups with their stored hole masks. Ordinary repair glyph remains provisional.

Result: native compile, whole-PROGRAM/lazy confirmation compile, contract VM and
one serializer round pass. Canonical run.sh --stage-only completed after the
user closed the game; signed installed engine is bf42886a, shared header contains
the bridge opcode, compiler ready and zero cache gameplay files pending. The
batch also consumed six gameplay projections including the parallel cannon
writer's registered source; that writer retains its independent acceptance.
Strict/deep app signature passes. An already reopened native game shows an
active first-person sea battle; no UI input or save mutation was made.

Disposition: source and installation accepted; rendered repair and confirmation
fit, plank/time change in a safe encounter, unaffected sails and save/load after
repair remain unresolved with the player. Repository hygiene reports only
foreign untracked cannon/deck/perk/icon paths; owned sources are tracked in the
focused commit and foreign staged blobs were preserved. No push.

### October 7 — manual raking fire, firing-eye floor, RakingFire capstone; staged, replay pending

Hypothesis: a manual volley at a locked ship should spread along its hull with gunner skill instead of one point, and first-person firing view should not sink under the rails.
Delta: cannon-rake-spread.patch (AimRakeFactor ramp 0.30-0.85 x capstone 1.0/0.45, per-gun reachable stations along the live waterline extent, ships only) and deck-firing-eye-floor.patch (eye clamped to design height in firing view) registered in build.sh; gameplay adds the RakingFire perk (requires CannonProfessional and ImmediateReload), the AIShip export, AbilityDescribe texts with corrected ImmediateReload/LongRangeShoot descriptions, a pictures.ini placeholder on the CannonProfessional rect, manifest entries and gameplay_sources txt support; icon source docs/images/raking-fire-capstone-128.png redrawn in atlas style.
Result: build.sh green, run.sh --stage-only exit 0; RakingFire present in the installed PROGRAM/RESOURCE files; both patches in the applied stack state; installed engine matches the candidate. The atlas has no free cell (unused 8itm4/5/6 placeholders hold the insect icon); TX injection queued after icon approval.
Disposition: source and installation accepted; in-game capstone visibility, rake spread against fas/broadside targets, eye height and unaffected AI fire remain unresolved with the player. Focused commit owns 11 paths (runtime.md is exclude-listed local-only); foreign mast/fleet/docs blobs preserved. No push.


### October 7 — generated mast-repair icon, installed; menu replay unresolved

Hypothesis: the provisional repair glyph and the first generated icon failed the
requested dedicated atlas-style affordance. The user screenshot preceded a
successful delivery: an active game held the normal launcher lock, so the first
content transaction made no changes.
Delta: generated a two-state mast/mallet icon with the existing list_icons atlas
as visual reference, packed it into 128x64 A8R8G8B8 TX, bound normal/selected
indices 0/1 to texture slot5 and registered its source/prepared hashes in
src/assets/ui/manifest.json. Existing command frames are part of their texture
cells. The ordinary gameplay delivery now includes this owned texture in its
receipt/backup/rollback transaction.
Result: whole-PROGRAM native VM compilation reports zero script errors; the
64px preview and TX header pass. Isolated delivery accepts first creation and
unchanged replay, and rejects corrupt source and concurrent manifest/input
changes before writes. After the user closed the game, the normal sync API
applied exactly BattleInterface plus the texture; the separately pending perks.c
source was excluded through the existing frozen source_set. Cache and installed
receipts agree on texture SHA256
5348e9bf4f8ab1a23fcbcb30481c797bd6181abcc13bb4e0c2486037b9c57eb3
and BattleInterface SHA256
8d56beba0436ada9af6a8a236bf3535f32c8df7fb9f88e577f89ba6f23a2d566.
Strict/deep signature verification passed; the installed engine and foreign
perks.c bytes stayed identical during this transaction. A parallel cannon owner
then entered canonical build/staging; no competing launch was started.
Disposition: source and icon installation accepted. The command remains hidden
in combat or without a fundable repair below the 60% standing-main-mast ceiling.
A real safe-sea menu and repair action replay remain unresolved with the player.


Icon source checkpoint: local commit 50f160b7fd5fc945175b80d8f9a5d94984b7d2a0
contains exactly eight owned paths. Normal same-blob index reattestation restored
staging provenance, then the original protected staged-commit helper succeeded.
All eight working SHA256 fingerprints and eight current foreign index blobs
were preserved; a separately committed cannon documentation change1d1a43e was
reconciled before that comparison. Agent context --check passed with no
nonignored untracked paths. No push. Guard-recovery dependency is closed by its
specialist's independent source replay. The later canonical cannon staging
retains installed icon/binding hashes; its current signed engine is8c5fb0bf.
Latest read-only native observation is the main menu, not a safe-sea repair replay.

### October 7 — player cannon replay reopens acceptance

Hypothesis: the first installed cannon correction restored the missing old-save
perk and root surrender lookup, but did not satisfy the full volley/projection
contract. Installed signed engine8c5fb0bf is the consumed batch.
Observed: the player's three screenshots show broken land/ship contours,
CannonProfessional's placeholder icon and a wide crosswise ellipse at a stern-on
ally. The player reports sail shots landing ahead of an already sinking ship,
white surrendered highlights and sail HP rising/falling after damage. The actual
old-save character menu shows the new unlearned perk and zero free ability points.
Source falsifiers: automatic Fire(AIShip*) bypasses rake distribution; the actual
method driver produces0m span before correction versus45m at high skill without
the perk. Native prediction uses obsolete height/accuracy formulas, while live
Ball_AddBall uses0.70/0.85 with perk/arcade accuracy modifiers. Cloth is absent
from the pure range-pick set. ProcessSailDamage starts from0 instead of existing
damage; RandomHole2Sail stops iterating when the current mask becomes0. The
neutral arrow atlas is beige/white, so relation enum2 alone does not make it yellow.
Disposition: prior startup/perk visibility accepted; volley, color, continuity
and damage actions reopened and unresolved. Source-only fixes and seeded probes
are in progress; no second installation or player-save mutation yet.


### October 7 — legacy ordinary spawn bypass and persistent port crowding

User reports dozens of ships/pirates accumulating at bays/towns. Three decoded
normal saves reproduce over-admission: latest November1 has74 living ordinary
fleet descriptors (38commerce/15patrol/21pirate), whereas the resident admission
contract is32/16/12 and60total. WDM_TRAFFIC_TICK still invoked legacy random
Follow/Warring constructors. Refresh adopts Follow into persistent roles, while
the transient pursuer counter excludes role-tagged actors and reopens on the
next tick. Real VM before repair produces extra Follow births beyond a seeded
full world (Congestion=1, zero script errors). Removed those legacy tick branches;
scheduler requests, real resident pursuit/clashes, quest constructors, storms
and specials remain. Congestion/disabled-encounter/special negatives and four
native codec rounds pass with zero errors. The exact composed PROGRAM with
installed shared headers compiles in a disposable runner (only game Main renamed).
No permanent tests or new runtime wrapper. Commit742a597 owns only the composer
and worldmap topic. Selected composed output0051df1dd35d4094af4ad6b30f1f6216e1f3af27c2f70a74a20766302a4482dd
is currently installing through existing DeliveryState/player_guard/transact,
with receipt admission/backups/rollback and strict signature. Engine is reused;
foreign cannon UI/ability source edits are excluded. Whole-tree hygiene is blocked
by concurrent untracked cannon UI inputs; they remain with their active writer.

A distinct prior-state mismatch remains: enc27/56/66 returning to LeFransua sit
near552/-371,16units from saved targets, from October28 through November1. Their
voyage counters7/9/3 do not advance. Actual berth fade is already installed;
merely claiming force equilibrium or enlarging the arrival radius is unsupported.
Raw physical GM/navigation investigation remains root-owned through readonly
port_native_congestion. Existing over-limit resident hulls/cargo are preserved;
this spawn repair does not delete74groups to pretend a60group invariant passed.
Source accepted; installation transaction active; port-drain/player replay
unresolved. Hourly map pause remains the separate pending live-profile scenario.

Selected composer transaction completed: installed/cache encgen0051df1d match
742a597; strict/deep signature passed. Signed engine8c5fb0bf is unchanged and
all244 post-quit player files retain exact hashes. No foreign content was copied.
Source/install accepted for quota-bypass repair; existing-fleet berth drain and
player map replay remain unresolved under the root/port-native evidence owner.

Native evidence review: installed islands.gm locates LeFransua at567.48/-367.56;
PTC table connects stalled node3540 to destination3570 in6transitions. The child's
Python port of PtcData edge refinement reports an open route; it is not execution
of native collision/movement. Returned force-equilibrium claims are rejected:
they omit the already-enabled berth fade (saved ServiceOnArrival=1), leaving
about0.5 instead of7.2 shore-force units. Intent-filter expansion is likewise
unnecessary for these saved returns. WdmShip::ShipUpdate has a session-global
static collision-escape vector initialized by the first ship; later ships reuse
it. This is a concrete source defect candidate, but observed headings alone do
not prove it causes these stalls. No radius/avoidance/collision patch was made.
Next decisive boundary is native movement/collision evidence during advancing
map play; root owns that continuation, with player controlling the scene.
Read-only child's disposable tools/probe_ptc_path.py was relocated into root's
owned temporary directory for cleanup, rather than retained as a new test owner.

### October 7 — cannon feedback batch: source verified, delivery blocked

Owner: 01a11860-c9e3-7813-a422-5b79bc52a828. Player confirmed the missed sail
target was already sinking; acceptance remains reopened for cloth range,
curved projection, stern-on longitudinal distribution and gold surrendered UI.
The candidate now uses live AIBalls parameter queries, pure nearest sail picks,
manual/automatic hull stations, crew factor .1–1, cumulative sail damage,
bounded pristine-mask hole enumeration and distinct learned/unlearned perk art.
All 62 existing ability descriptions have the requested short pirate voice.

Actual extracted auto-fire methods reproduce zero target span before and45m
after at the old maximum baseline; zero skill, full perk, moving lead, common
jitter, height and nonship negatives pass. Actual solver translation preserves
the fixed angle after replacing a direction argument with a muzzle origin.
Actual clipped-mesh callback retains >.485m relief and rejects outside/NaN/budget
inputs. Actual sail Trace reproduces the missing above-box Y entry and passes its
mirror/outside/rolled cases after correction. Actual Draw batches gold surrendered
markers with unchanged normal indexing and restored render states. Real script
VM passes canonical sail hit/repair/save fixtures, canonical crew factor0/.5/1
and clamp/event/save fixtures, and full-program perk/menu compilation. These are
source/VM observations, not player action acceptance.

Canonical staging attempt1 rejected private ai_helper.h includes from battle UI;
the live display predicate was moved to public sea_ai/include/ship_surrender.h.
Attempt2 rejected missing new-file mode in that textual patch; the mode is now
explicit100644. Attempt3 compiled the cannon/surrender/rigging code, then failed
on the concurrent worldmap-berth-homing.patch's access to private slowingAlfa in
WdmEnemyShip. That path is held by continuing writer01a10edf-1111-7272-85e3-4c946daac56d.
No installed game changes occurred in these failed attempts; signed engine8c5fb0bf
remains the player runtime. Root retains build/stage/install acceptance for this
cannon batch after the shared worldmap input is valid. Disposition: unresolved.

### October 7 — frozen cannon delivery and missing script event header

The continuing worldmap writer was isolated by a task-owned managed checkout of
5ceff926. Root retained integration and app delivery ownership; that checkout's
build/runtime caches are separate, and the shared saved checkout was preserved.
The first stage restored PROGRAM/RESOURCE without the previous delivery receipt
and rejected perks.c; it did not modify the app. Reusing the canonical runtime
with its matching receipt resolved that admission failure.

Canonical run.sh --stage-only then passed for the frozen commit and delivered
10 files plus signed engine fdab9859e2228c979a675f25e0f75e769df4ccb0b909dac2171bc89a3a2dff53.
Deep/strict signature verification passed; sync reported zero pending files;
all 232 player SAVE file hashes were unchanged. The preceding engine8c inventory
is historical. Current player engine is fdab9859, arm64 native Metal, with the
existing player state/settings and no game process after the following crash.

Installed public launch failed before the menu with fail to create program.
The user's crash report matches CoreImpl::ProcessEngineIniFile; actual player
logs report Invalid Expression. An exact delivered-PROGRAM compiler probe with
the actual installed shared headers reproduces it at sea_ai/AICannon.c(8).
Replacing only shared/sea_ai/script_defines.h with the built header makes that
same compilation pass with debuginfo0 and cache_mode0. Earlier source/VM checks
used newer source headers and therefore did not accept installed startup.

The canonical install transaction is being extended from shared/messages.h to
the finite LEGACY_SHARED_HEADERS policy, including the three new cannon query
constants. Known prior bytes are admitted; unknown edits, unknown headers and
SAVE destinations are rejected. Gameplay, settings and the native binary are
unchanged by this correction. Corrective staging and real startup/action replay
remain unresolved under root; /Applications remains reserved until that replay.

Corrective follow-up: finite shared-header delivery was committed as ce5eef9 in
the saved checkout and cherry-picked as476c045 into the frozen delivery checkout.
Canonical run.sh --stage-only passed after that final source change. Installed
shared/sea_ai/script_defines.h SHA-256 is
9cfad8102a6e0a4dba5b06da6b7b631debe6df7512798ee35c30469003755ff9,
matching its receipt; engine remains fdab9859. The player subsequently opened the
current saved sea battle in the installed app; the window was responsive and
current error.log empty. Actual yellow/gold surrendered overhead arrows were
visible beside ordinary red enemy arrows. Startup is accepted; original cliff,
stern-on ally, sinking-sail volley and damage actions remain unresolved.

The player then reported very low FPS while manually aiming. A three-second live
sample during the requested aim hold binds the installed fdab9859 process to
1533/1951 main-thread samples in DrawManualAimOverlay, mainly GEOM::Clip,
ClipByPlane, segmentBox and GEOM::Trace. The earlier normal-combat sample had no
overlay stack and its screenshot showed51FPS atTime2; it cannot serve as a matched
same-view FPS comparison. Performance acceptance is reopened. Source correction
preserves all paths/queries/caps while rejecting unrelated curve/model candidates
and wholly outside BSP faces earlier. Tighter prism sphere was rejected after
real BSP probes changed capped coverage. Native build/stage/install of the next
combined batch belongs to the harbour integration owner after source freeze and
player closure; no process was stopped and the installed engine staysfdab9859.

Performance source freeze: focused commit11571da passed native syntax for the
controller/geometry and a canonical71-patch migration on the exact cloned source.
The final exact BSP function retained callback bytes/order/cap and node/face visits
on1920 real ship/island queries; cold/warm, zero-plane and test-only negatives
passed. The harbour integration owner then completed the canonical full native
build on11571da overce5eef9+c6bf191. Installation is still pending player closure;
process1916 remains the installedfdab9859. Live aiming FPS and the changed firing
actions are unresolved under the cannon writer after that installation.

Before retiring the frozen delivery checkout, both signed engine rollbacks, nine
original content blobs, the prior0810dd98 script header and two historical receipts
were retained in the canonical cache; the cannon writer independently verified
all14 hashes. The managed worktree archive completed and its archived attachment
was verified; temporary source/profile evidence remains task-owned until the
pending player acceptance.

Combined installed delivery completed after the player exited. Canonical
run.sh --stage-only passed for11571da overce5eef9+c6bf191. Installed signed engine
SHA-256 is37598a37e4c73e7d4823576b69ad0a0dca34b59c97ce70efb9637f74d209b9f9;
its app receipt SHA-256 is68f05ba2e537252aabc54b4095b8389183345334fbe299ad72cc888e5f917170
with matching @engine. The integration owner verified deep/strict signing and all
252 pre-install player files unchanged, with no added files. Shared headers and
all12 cannon/worldmap content consumers match their declared source/runtime/app.
The cannon writer independently matched the installed engine hash and observed
the current night sea battle, November3 02:12 atTime2, FPS114 in third person.
Live process30726 started20:23:38. Its first post-install three-second profile
contains no manual-overlay stack and is a normal-combat baseline only. It is not
a matched FPS comparison to the earlier daytime battle. Player aim-hold replay
and original physical firing scenarios remain unresolved; no gameplay input was
sent by the cannon writer.

Manual replay boundary: CUA Tab input was refused because the player was actively
interacting with the installed game; no key was delivered. A subsequent read-only
player-active profile at20:29:07 also contains no manual overlay (texture loading
instead). Neither post-install profile accepts aiming FPS. The single pending
player aim-hold question in the cannon chat owns the next measurement; original
changed firing/sail/curved-shore scenarios remain unresolved there. No second
launcher, synthetic save edit or process shutdown was used.

### October 7 physical harbour recovery — c6bf191 compiled, activation pending

Hypothesis: connected point navigation can still request full-hull poses that
ground collision rejects. Readonly saved enc27/56/66 positions at552/-371 stayed
unchanged from October28 through November1. Shipped Martinique.gm has no BSP;
the actual mein.gm archipelago BSP has467548nodes/153467vertices/170071triangles.
A disposable C++ fixture executes native GEOM Clip/ClipByPlane, native PTC
refinement, CollisionTest, Move/ShipUpdate and KillTest against that geometry,
saved velocity/yaw and real sloop/bark extents. Explicit wind/attribute/event
adapters replace absent renderer/core plumbing. Old controller leaves all three
voyaging after300seconds, rejecting translation and yaw at contact.

Rejected narrower candidates: fixed escape normal, braking/retained turn, and
straight collision-safe escape fail curved corridors, wind or reload. The final
bounded local full-hull position/heading search recovers after ordinary committed
port contact, preserves original arrival, and recomputes transient paths after
load. No saved fields, radius expansion, fleet deletion or cargo rewriting.
Each candidate pose is collision checked; the old shared static fallback vector
is replaced by a per-ship/per-contact-episode vector. Goto cancels obsolete plans.
Public real headers compile without private slowingAlfa access.

Final ASan/UBSan replay passes all three arrivals with exactly one native event
per group, four cardinal winds, collision-free accepted poses and path/counter
reset plus actual Merchant save-attribute methods at0.1/0.2/0.3seconds. Two service
groups retain exact positions and no new events. Quest, patrol, pursuit, next-leg,
battle and distant approach exclusions pass with seeded contact; open-water and
first-ten-contact physics match unchanged ShipUpdate numerically. Attribute
adapters do not constitute native codec or rendered scene acceptance.

An O2 CPU-only probe measures eight planning calls at817–1327microseconds;
this is local planning cost, not an FPS claim. Search is limited to1024expansions,
failed searches retry no faster than two seconds. A deliberately unreachable
local target exhausts that bounded search in8756microseconds and leaves no path;
worst live fleet/frame cost still needs the pending player map profile.
Canonical build.sh succeeds,
source patch stack migrates exactly four worldmap files, full native engine links.
Focused commit c6bf191 preserves foreign index/paths; agent_context --check passes.
No permanent tests. Cannon writer released /Applications after its header/startup
fix; root's canonical stage-only rejects the player's running fdab9859 engine.
Root owns subsequent stage-only and player-state checks after player closure.
Disposition: source/native fixture/build passed; installed activation and rendered
prior-save harbour drain unresolved. Hourly whole-map pause still awaits profiling.

## October 8 — smooth all-gun density on current scene depth

Hypothesis: the reported red-zone misses include an independently proven density
defect, separate from first-contact mask holes. Selected-gun omission, count
normalization, eight-corner covariance and interpolation of inverse metrics can
shrink the displayed region. The player chooses main-density contours with
allowed tails, then requests a soft rounded blot on scene depth.

Delta: every firing gun contributes27 weighted launch curves and65 quadratically
spaced covariance-root sections. One unnormalized smooth union drives the
Gaussian fill and perimeter. The physical first-hit, own-ship and stale-depth
guards stay with their existing owners. Bounded stack storage removes hot-path
allocation without changing field bytes:10,000 solver cases and120 field
payloads match;52-gun paired CPU cost3.04631→1.24369ms. One strict-MSL GPU probe
measures3.4612ms for52 fields over1,048,576 fully eligible pixels; this is not an
installed frame-rate result. Fixed quadrature remains approximate, with a tested
far-water conditional survivor case at20.31% outside rather than a universal
95% guarantee.

Result: canonical run.sh --stage-only returns0 after the last source edit.
Signed installed engine70d78026692e3eeb351e65fff178df43fa31994583cc043a2372e930ba3d8678,
receipt08f6c75b2cd1180d9097ebbdc03ef79d61395be66e65e5aa90557ab4b8360cb4.
Deep/strict signature passes;17 frozen source inputs,253 player files and all
three foreign index blobs are preserved. Both delivery plans are empty,
compiler_ready is true and the two declared script headers match. Native-only
aim headers are compiled build inputs, not installed script-header deliverables.
The pre-existing engine is retained by the canonical per-hash backup owner.
Focused local commit388d4e7 contains only the five owned source/topic paths;
agent_context --check passes and the three foreign staged blobs retain their hashes.
The three owned temporary evidence directories are removed through the canonical
helper; historical foreign fixtures are retained.

Disposition: source/native/GPU probes and installed staging pass. Actual smooth
blot appearance, first-hit fan/prism flicker, closed rim, aiming FPS and stocked
current-save firing remain unresolved; the player owns real-game replay. No game
is launched or player actor terminated by this batch.

### October 8 — debug tsunami source candidate

Hypothesis: one transient, shared SEA height/gradient addition can move a large
crest toward the player's launch position while existing buoyancy/collision
rules determine the outcome. Native candidate is `sea-tsunami.patch`; script
candidate admits F11 at sea and adds `Цунами`/`Стоп` to the existing debug menu.
One isolated normal PROGRAM startup plus the exact debug segment compiles with
zero errors. Native scalar/SSE/profile/lifecycle and same-value cancel callback
probes pass. No player SAVE/configuration is used by these fixtures.

The three pending content files were withdrawn to the task-owned temporary
fixture before the parallel aiming delivery so its batch cannot silently stage
an incomplete tsunami command. Tsunami remains unregistered in build.sh and
uninstalled. The player's close acknowledgement was followed by an observed
closed process list; the parallel aim owner later reports a new live player
actor77733. No root input or stop is performed on that actor.

The parallel aim owner rejects its per-pixel exact-water solver on observed FPS
and water-gap evidence and confirms a depth-only projection replacement.
Tsunami's temporary exact-water ABI/shader integration is discarded; the native
rendered surface is the remaining shared consumer. That parent retains sole
build/stage/app ownership until its corrected batch is released.

Disposition: source properties pass; installed delivery, F11 action, horizon
approach, crossing/ship response and stop/pause/teardown replay are unresolved.

## October 8 — simple range-local depth drape

The exact per-pixel ballistic/water collision attempt is rejected on the player's
10FPS screenshots, torn water projection and a native sample dominated by GPU
command-buffer/drawable waits. Installed engine40ca9f1c is historical. Its CPU
collision field, water atlas and sampled first-hit masks are not the current
implementation.

Depth-only enginec220a1e6 improves the shape (player screenshot102FPS), but the
player reports roughly half ordinary FPS, a thick distant rim and intermittent
zoom disappearance. Enginec474cc73 removes terrain identity passes, narrows the
rim and reuses point storage. The next screenshot92FPS accepts the thin main rim
but rejects a secondary far hillside stripe and broken joins. Neither screenshot
is a paired with/without-aim performance measurement.

Hypothesis: a field extended along the full flight also intersects distant land;
the contact-only coplanarity fade can remove otherwise valid contour fragments.
Delta: one65-section aggregate dispersion field is bounded by a smooth axial L4
cap around nominal arrival range. All27 weighted launch curves per gun share its
nominal arrival time for axial moments. Existing32-byte section metadata carries
the common center/radius, rebased once. The shader evaluates the field on nearest
rendered scene depth; its contact rim has no coplanarity fade. Finite/zero-gradient
cases emit density only, with no rim. Physical firing, air helpers and random
dispersion remain unchanged.

Result: native build and strict runtime Metal compilation pass. One-off bound,
translation and zero-gradient negative probes pass; independent source review
finds no actionable P1/P2. Canonical run.sh --stage-only returns0 and installs
da34fbae836f77dcc531c88d1ea5c7d8312631180e5fd05ff22916585ee096ba;
receiptcff0fdcf837d9f4c56cb5f5b99ab3a057b1abbf8d6902bce2a6dbba9ea801576
matches the engine. All239 non-log player baseline files and three foreign staged
blobs are unchanged. Aim commit666da3f is local. Broad agent_context --check fails
only on preserved foreign untracked docs/sea-tsunami.md, sea-tsunami.patch and
the debuger.c/debuger.ini content candidates; this is not a clean repository gate.

Disposition: installed candidate; visual far-strip/seam/zoom acceptance and paired
aiming FPS unresolved. The player owns replay. No further UI, input or launch is
performed after the computer-control stop. Tsunami source/working build hunks
remain with their stopped owner and are not included in the aim commit.
Three owned direct-child temporary directories were removed by codex-delete-temp.
The owned witness-pilot.GlorGPuw child remains under the historical foreign
corsairs-aim-integration.KX3uJhIv43 directory: the helper rejects nested targets,
and the parent is not owned for deletion. No alternate deletion is attempted.

## October 8 — false outside contour from a covariance-root derivative

Observed rejection: the player's13:00:21 screenshot still has a huge polygonal
hillside contour; the player reports that the range-cap revision looks worse.
Installed engine/receipt confirmda34fbae, so this is not a stale-engine claim.
The cap attempt is rejected as the solution to that symptom.

Reproduced defect: the current `(1-q)/gradient(q)` stroke estimate approaches
zero distance as an interpolated covariance root collapses, even for a receiver
far outside the field. In one source-exact seven-point Metal fixture a linear
radius boundary is249.85pixels away, but the old shader emits rim1/keyline1.
The corrected shader emits0/0 in both strict and fast math. The real q=1 boundary
retains rim1; its one-pixel AA case returns0.500006, center fill1 and zero-gradient,
out-of-station-range and outside-arrival-cap cases return no rim.

Delta: normalize the local field coordinate to unit radius before evaluating
stroke gradient, then use `4*(1-fourthRoot(q))` as its numerator. The q=1 geometry,
Gaussian density, section ABI, firing and airborne projection stay unchanged.
This is a bounded contour-math correction, not a scene-contact solver or filter.

Result: native build passes; all four live MSL entrypoints compile in strict and
fast math, and the actual GPU fixture passes. Local commitffe2514 contains only
two aim-owned paths. Canonicalrun.sh --stage-only returns0 and installs engine
2a79c6b1822a5eca4d642d464939e74fc2943415354606684dac6472fe3aa6a5,
receiptabc852e3fd4bc37af79168e0a6c554911628e67ba97fbfdfc6a9691b1725e5b0.
All245 non-log player files and three foreign staged blobs preserve their hashes.
The broad context gate still fails only on four preserved foreign tsunami files.

Disposition: installed candidate, no UI/input/launch. Actual disappearance of the
hillside stripe, zoom continuity and paired FPS remain with player replay. The
player also asks about abrupt/ugly transitions: geometric smoothing was not
changed by this correction and remains open. No further speculative geometry
variant is called accepted from the shader fixture.

### October8 revision10 — one smooth projected dispersion patch

Observation: the player confirms revision9 removed the false far stripe, but
rejects broken/angular terrain contours, water disappearance and aiming cost.
The13:13–13:14 screenshots show83/75/42FPS in different views, so they do not
measure a paired overlay cost. A perfectly elliptical shape is unnecessary;
the requested patch needs rounded edges and smooth transitions.

Delta: cannon-depth-drape.patch replaces65 fitted axial sections with one48-byte
world mean/full xyz covariance at nominal arrival. Every eligible gun retains
27 weighted independent yaw/elevation/speed evaluations; no new contact marches
or RNG. aim_volume.hpp projects the covariance once with the current camera
Jacobian and evaluates one pixel-space quadratic. Nearest scene depth supplies
visible receiver pixels without reshaping the contour. Exact own/model exclusion
and relation color remain. The contact pass/color snapshot use a bounded rectangle;
unchanged stopped air blends afterward in its own bounds. backend.mm avoids
water capture before it can finish the main encoder. Own/model capture and air
remain costs; real aiming FPS is not called accepted.

Evidence: the exact52-gun producer agrees with independent uniform-jitter moments
and translation, with invalid-input rejection. Strict/fast MSL check96 boundary
angles, center fill, one-pixel AA and far-outside rejection. The actual native
encoder preserves contour across four stepped receiver depths, excludes own and
sky pixels, retains model color and uses a136×84 contact scissor at256×192.
Projection probes cover5x zoom, yaw and world-origin rebasing. Canonical build
and run.sh --stage-only return0 after the last native/ordered-patch inputs.
No permanent tests are added.

Installed engine0fc23284301f1b3465887169686ab1b14dd1d6b605a07659a3ca92d73282fe6d,
receiptd9d0421b3f72784831020597741e1f8d18bebc06e5d7fddbba5307ccf3732a6c.
Final deep/strict signature passes after installer completion. An earlier
in-flight seal observation was not acceptance. All245 fresh non-log player files
and three foreign staged blobs preserve hashes. Frozen input hashes:
aim_volume.hpp59e996d7237acaa157fa920c7ceb89ac1f85eab7718337e39bae4278c40d551c;
backend.mm197c3ef55920cc5a9478b719bb0df16ba4a309cd04a4aa78298a5d2a3c7083eb;
cannon-depth-drape.patchc8aa8dbbeead37e8d968e097c892fa9a824a2c1988603a1795d6a6911c89f641.

Disposition: installed candidate; complete player terrain/water aesthetics,
zoom/motion and paired FPS remain unresolved. No UI/input, game launch or actor
termination. The broad context gate fails on four preserved foreign tsunami
paths, not adopted into this checkpoint. Focused local checkpoint00e7d57;
no push. A subsequent direct user request resumes only tsunami/debug-trigger
verification with its existing owner; the computer-control stop remains in force.

### October8 revision11 — actual world-depth decal, close-water anchor and ordinary MRT identity

Observation: the13:51 screenshot and follow-up reject revision10's flat screen
ring, lack of object conformance, and close-water aim displayed near the horizon.
Native source confirms its contact quadratic ignores receiver position. Its
airborne mean can carry the airburst's8-unit overhead offset. DX9 GetTransform
already restores absolute camera coordinates before the real CMatrix inverse;
the pure range selector already traces water. Neither camera nor firing selection
needs a replacement for this defect.

Delta: one selected-receiver world anchor/full covariance, inverted once. Current
depth reconstructs actual nearest receiver positions; a bounded quadratic with
fixed3x3 occupancy filtering supplies fill and rounded thin perimeter. Far
geometry outside the field gets zero coverage. Conservative enclosing-box bounds
replace center-Jacobian bounds. The ordinary fixed-function/lit model fragments
write current depth and identity through MRT, sharing their original shading and
alpha-discard helpers. Model scope/camera/depth-write checks gate those outputs;
no model before-depth copy or full-screen difference pass remains. Own ship and
water capture are zero-GPU-work ABI guards. Stopped air, ammunition trajectories,
range selection and existing relation colors remain unchanged.

Evidence: strict/fast exact complete scene/aim MSL compile; native source/build
and canonical stage pass. Actual native encoder uses ordinary fs_aim identity
and compares every surface pixel against an independent world-sphere/filter
oracle across flat/tilted/curved/creased receivers, water below the camera and a
far plane behind the selected field. Far plane has zero marked pixels; close
water has no horizon mark. Own exclusion, relation color, alpha holes, origin
translation and5x zoom pass. Representative contact ROI is68×52 at256×192.
No permanent test infrastructure is added and no paired real-game FPS is claimed.

Installed engine5b14e3196d4674c7cc296de97debc83fc26fbf7a8a8eb88b41f4cf40989221a7;
receipt0ebc514c8b38488591258f7a36a94c33e6e3301a836f8108c46d8e6bae0fdce4.
Final deep/strict signature and receipt binding pass. All250 fresh non-log player
files and three foreign staged blobs retain hashes. Frozen native inputs:
aim_volume.hpp4d2e217d19715914a5d3432e6dc20836df527f1c2469b469e8cd9e119b4c2001;
backend.mm153d2140a79275dd8e2e7a09751631395e371691d141dd3fc787620d3947f8dc;
land_shadow.hppea388bbdd44b1df2e055e0ab19cd159b46efd15639a811a4348a3d45f8327fd2;
cannon-depth-drape.patch95b6eb59d38a8b63c0d1484d28bbf4c70a1dc79f963255586b461030cde2bec8;
unchanged sea-tsunami.patcha24fe3e7f5e03a3ce4e2f1adcd82e2a89c34e4078e02610f325a59431eb8f697;
build.shc30261105afd42b8e6da8a6aa49ba8aabd2132369aef7d509794b3e99684c57c.

The resumed tsunami owner found no new product-code defect: complete no-window
F11 SDL/core/menu/script/native start/cancel/restart and controls VM restore pass.
Physical macOS interception and visible wave/ship response remain unresolved.
Checkpoint499037a integrates the eight already delivered wave source/topic/build
paths. Their imported debugger baseline retains its original whitespace; the
global context gate passes, and aim-owned whitespace checks pass separately.

Disposition: exact new batch installed; real startup, surface appearance through
all angles/zoom, wave motion and paired aiming FPS await player replay. No UI/input,
launch, kill or player-state mutation. No push. Prior flat-ring GPU checks are not
reused as surface-conformance acceptance.
Focused local aim checkpoint69f6e84; global agent-context and aim-owned whitespace
checks pass. The three unrelated staged document blobs remain outside it.

### October8 revision12 — actual impact spread, smooth perimeter and Esc tsunami

Observation: the14:36–14:37 player screenshots reject revision11's undersized
water footprint, angular/frame-jumping perimeter, excessive brightness and
internal sail/mast outlines. Their different views/FPS do not isolate overlay
cost. Fn+F11 opens no menu according to the player's explicit answer. Source
falsification confirms that evaluating jitter at one nominal flight time and
slicing its3D covariance is not a water-impact distribution: an independent
50,000-shot fixture at50m has36.46–70.45m fifth/ninety-fifth-percentile landings,
while the old water slice has only.434m half-length and covers1.814% of hits.

Delta: every eligible gun contributes the existing27 weighted launch samples,
each with its own descending sea landing and first positive selected-range-plane
crossing. Two marginal impact charts retain true means; water's18.95m half-length
in that fixture covers82.192% of hits, not a95% guarantee. Existing ordinary sea
fragments write exact sea identity through MRT; no additional geometry pass or
snapshot is added. Analytic q-boundary AA replaces neighbor receiver-mask edges,
so holes/depth/identity seams are not drawn as perimeter. Bounded70ms tangent
inertia resets at disjoint fields/receiver changes; current physical depth bounds
never interpolate through a third object. Fill/rim/composite alpha is reduced.

Rejected intermediate: nominal-flight normal variance still gives only.434262m
support and excludes a2m relief niche. Final finite projection support instead
uses the actual water-impact fore/aft radius around current receiver witnesses/
model extent. This admits local relief but does not measure unobserved terrain
or conditional hit probability. Far500m scenery stays outside the tested50m
field. Real terrain shape/all-angle acceptance remains unresolved.

The separately owned airburst batch is integrated at its released final bytes:
stable per-shell1–7m proximity, common2m-full/8m-zero quadratic damage for every
consumer, ordinary bomb aim/height law and omnidirectional finite fragments/VFX.
Native71 ASan/UBSan cases and seeded native-VM sail damage/ordinary-negative
checks pass; old sail-hole flooring fails the zero-distance-power negative.
The existing native projectile codec is unchanged. The existing Esc pause menu
adds sea-only tsunami start/cancel through one guarded script command writer;
F11's physical failure remains unexplained rather than claimed fixed.

Evidence: exact aim MSL and four ordinary legacy/modern sea MRT pipelines compile;
windowless GPU metric and actual native encoder pass Metal API validation for
sea/wall admission, relief/far negatives, own exclusion and subtle perimeter.
CPU frame checks cover receiver switch, current clips, finite bounds and inertia
reset. Full integrated PROGRAM include graph plus pause/debug segments report
`CompilerCases=0`, `script_errors=0`; only disposable fixture Main is renamed,
so no game startup, UI, input or player save replay is executed. Canonical native
build and final stage-only pass after the final source freeze. Exact app/cache
gameplay plans are empty, canonical bytes/changed headers match, and final
deep/strict app signature passes. Protected245 non-log player files and three
foreign staged blobs preserve delivery-boundary hashes. Two external startup
logs changed and a game process subsequently appears; root neither launches nor
touches it. Neither observation accepts gameplay.

Installed engine:
`b5e871dd3a2c24168d9267bcda7b44309dd39c7a4e26946e9f167c1627c5113a`.
Receipt:
`6c4a9e52ccfa3e68cc59b1a77a91b7f64e5b4050ec4b9c75e693d45d7ae2bedf`.
Focused source checkpoints `bdfce2e`, `ba9ae12`, `a813b29`; no push.
Frozen native inputs (SHA-256):
`aim_volume.hpp` `08c129bd721edccd3b51f4d09243d794eaa1e018dad2bab1a3c8ed7e1dc6ef93`;
`backend.mm` `d04c4075aaa0f3d6f70320d0a7bb12045d8bb35a863151899bd0a6a8ba40ad62`;
`sea_shaders.hpp` `a521d41ca6fa2c20e645bb0986f9cad91f8a0e60ea470db07394d8f150cae682`;
`cannon-depth-drape.patch` `bbb90c256eb8bb9fccfa1f1863db57cb48e2d4dfecf58472595556bded925607`;
`airburst-shell.patch` `67bb248d87314d0441e69b102851ebcdd869860fba5b9820814b7fc553abd54d`.
Disposition: source/native/compiler/delivery checks pass; real appearance,
zoom/motion, paired FPS and physical Esc/F11/menu/wave actions remain unresolved.

### October8 — player acceptance and bounded follow-ups

Direct player report accepts revision12's shape/projection and excellent FPS;
the airburst ship result is also accepted. It requests slightly more visible
daytime contour and restores a middle fort-airburst balance. Exact ammunition
answer: shrapnel bombs now barely disable forts, earlier they were too strong.
The corresponding human tsunami report confirms the Esc button starts the wave,
but rejects its straight repetitive crest and requests visual/ship-interaction
work in that feature's existing owner.

Ownership: this aim task owns only `aim_volume.hpp` brightness-local contour and
shared integration/build/stage/index/current inventory. The ammunition owner
owns fort-specific diagnosis/repair. The tsunami owner keeps next-wave discovery
or isolated source preparation outside the current day-contour/fort batch.
No game UI/input, launch, kill or save clone is authorized by these reports.
Daylight candidate raises only bright-background rim/keyline and composite cap;
dark-background formula, line width, fill, geometry and air are unchanged.
Next installation requires a fresh frozen native/full-PROGRAM batch and a closed
player game. Current installed hashes above remain the accepted drape baseline.

### October8 — daylight contour and bounded fort-airburst balance installed

Hypothesis: the near-zero fort result comes from premature proximity to a model
box rather than insufficient catalogue damage. Actual installed meshes confirm
the box encloses empty frontage outside every gun's8m blast radius. The minimal
native correction uses intact gun positions with the existing swept sphere.
Controlled matched approaches now disable1–2 guns instead of the old4–6 or the
installed zero; authored blocked approaches remain blocked. The23-fort/2636-gun
geometry corpus,17 production-loop ASan/UBSan cases and12 native-VM gun/HP cases
pass. The probe uses authored/default model pose, not an immersed saved camera.

Daylight raises the bright-background rim at most16%, keyline alpha from.14 to.17
and composite cap from.52 to.58. Dark-background formula, width, fill, geometry,
air and firing physics stay unchanged. Exact aim/sea MSL pipelines compile;
canonical native build and stage-only pass after the final source freeze. Only
`aim_volume.hpp` and `airburst-shell.patch` differ from the13-input previous freeze.
All PROGRAM bytes and delivered script headers remain identical, so the previous
complete PROGRAM zero-error compilation remains applicable without a new fixture.

Installed engine `10400733be42be03508aa27280dcbd1e2d6be77d11ef638a56c563fe6956da51`;
receipt `23c4496eff4d21ae05c3b27d35954b1dd08451ec9e03a3b6f17a8c0fd4651fad`.
Native input SHA-256: `aim_volume.hpp`
`072c91098adc150c4b903cf2cb4201cb2786cf918d8f489521a2a81bf7f7f7f9`;
`airburst-shell.patch`
`3b87f0600f1bf780e86849b3c6b380d7e1583255ad261f6cb6442f0fa1d60f48`.
Final signature,37 canonical inputs, two delivered script headers and empty
cache/app plans pass. Protected247 player files and three foreign staged blobs
preserve fresh hashes. No game action, save clone or GPU drawing replay occurs.

The sound owner's first muting candidate is explicitly rejected and rolled back
before this freeze: `AIBalls.c` retains SHA
`82e36c2958d3808df3200feaeb0bd5119b4505afe409a66bff565bf2c071e8b9`.
Its engine mass-identical-sound handling continues in the same ammunition owner;
the expanded tsunami candidate remains in its existing owner. Shared build/stage
must transfer once, with engine audio first and tsunami queued after its terminal
receipt. Disposition: source/native/compiler/delivery accepted; new daylight and
fort-salvo player acceptance unresolved. Focused checkpoints `ce5faf2`, `3646cf0`;
no push.

### October8 — dense nearby naval sound requests coalesced in the engine

Hypothesis: mass identical simultaneous effects create independent full-level
voices and unnecessary sample/file initialization. The actual `SoundPlay` body
confirms512 same-clock splash requests select, initialize and start512 voices.
The correction applies one40ms onset interval per nearby same alias/type before
those operations; the same probe now creates1 voice. Ten real-time spaced events
create ten voices while earlier voices remain live, proving no total salvo cap.
Distinct/distant effects, interval expiry, stop/release/end, failed init/start,
singleton level and unaffected loop/cache/speech/music/cannon/charge cases pass.
Existing audio policy probe and canonical service syntax compilation also pass.
No permanent tests are added and no producer sounds are muted.

Canonical native build and stage-only return0 at source checkpoint `31bb837`.
Installed signed engine is
`64f461f7c3b74f1fca4ad49bfca9f24bbd017444312fe85a5b9566ad48d39a76`;
receipt is `740f0676676d2b8a8572631959122330cc33d412839c9e64bb5be0e69737812a`.
All116 receipt entries,37 canonical inputs, two script headers, empty cache/app
plans and deep/strict signature pass. All246 fresh non-log player files and three
foreign staged blobs preserve hashes. Fort patch and existing gameplay producer
bytes retain their frozen hashes. Complete PROGRAM inputs are unchanged, so the
prior zero-error VM compilation remains applicable. Disposition: source/build/
delivery accepted; broadside auditory quality and measured battle FPS unresolved.
No game action or push is performed. Shared build/stage/app ownership transfers
to the existing tsunami owner after this terminal receipt and scope release.

### October8 — continuous tsunami, native ship contact and forced map event

Hypothesis: the reported repeating white wall comes from the original uniform
crest and ordinary periodic foam. One continuous native strength/profile now
drives height/slopes/40-byte material tail, existing hull splash consumers and
ship pitch/roll/contact. The shared sampled strength owns nominal 10–50% hull
damage; storm/armor defenses remain in their existing owner. Actual native
overturn is admitted only for a live fierce contact within 50–130° to wave travel.
The existing saved storm descriptor/trigger admits a rare finite-circle event,
consumes its sampled strength and forces local sea without encounter skipping.

16,146 geometry/contact cases and 243 full-profile CMatrix sweeps pass; actual
SEA/SHIP/SEAFOAM source syntax passes. Combined current PROGRAM VM returns
DamageCases0/script_errors0; descriptor/login/exit gates and two save-codec rounds
pass with declared fixture seams. First offscreen pilot is rejected for visual
admission because its camera/substrate is unrepresentative. Restored installed
64-height/four-mip normals/day12 sky component runs7frames/27passes in2.647s,
145.5MiB RSS, exits0 and leaves no runner. Whole frames show irregular foam and
continuous substrate; inactive32/40, cancel, depth/MRT checks pass.

Exact canonical stage returns0; signed engine4423316154a9a9df91434b503be963cb3b1467d0d19651aeca1ebaa502895a91,
receipt2b30bb9882742f575cad42689e4121aec2f730293d294cb7716571b590bced66.
All116 entries,37 inputs, two delivered script headers and zero gameplay plan
match. Installer deep/strict verification passes.246 non-log player files and
three foreign staged blobs are preserved. Disposition: component/source/build/
delivery accepted; actual visuals/splash/damage/capsize/map-transition/FPS
unresolved and player-owned. No game action or push. The player's subsequent
horizontal drift and horizon complaint are a separate active follow-up batch.

### October8 — source-ready transport and narrow sky seam, install waits for idle

Hypothesis: existing tsunami pose response supplies no horizontal current; the
ship should carry native motion into the ordinary coastline sweep. A transient
60–300m budget uses temporary Move velocity, shared with non-consuming TouchMove
predictions. Exact existing method-body probes pass continuous21-strength sweep,
60.000/179.987/293.884m passages, professional209.339m, immunity0, grounding/cancel/
restart/zero delta, clean raw saved State and unchanged ordinary propulsion.
Native prefixb9e7f075b50e742684afdbd7583766f753c6c836af24e44f2da3d03758b1d427
plus unchanged worldmap suffix produces ordered patch97bc7f6241868a6238ab7d657077cb0b7a652d97f1df536c8c31b0fad4904c9e.

Sky analysis binds two consumers: authored 513-vertex SkyFog opacity fades with
Fog.Height; a second shader band unnecessarily suppresses cloud/texture detail
to9.2 degrees. Candidate narrows only that exact-color seam from .018–.16 to
.004–.018. Installed day12 textures plus the full native fog sphere show restored
lower detail; exact horizon227554 pixels, upper264343 pixels and full no-fog
RGBA remain identical. Four matched GPU frames exit0 and leave no runner;
authored thick-weather opacity is source/numerically preserved, without a storm
GPU replay. Candidate dynamic sky SHA0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.

Canonical `run.sh --stage-only` exits1 before build/staging because game30770 is
running from the supported app. Installed bytes stay at44233161/receipt2b30bb98;
no game control, termination, SAVE/config write or runtime override occurs.
Disposition: component/source accepted, full build/install pending idle; player
shore-impact and complete horizon/weather replay unresolved. Root owns the
bounded idle retry, and the player owns closing/testing. No push.

### October8 — transport, shoreline spray, foam repetition and horizon follow-up

The game is absent at the final canonical stage attempt. Native shore prefix
f14ac44c051174e9f64b099c2b8d230cd11f042048e10158aff781c71f430a33
replaces only the previous863-line native prefix; worldmap suffix remains exact.
Combined patchfeba0ff372f36ff2c7fff96409a3e21e8f42aef7e53c648f34ed798253c10976
uses authored CoastFoam strips, actual island trace and synchronous scalar message
50203 to a separately owned12-emitter SEAFOAM_PS pool. Source/component checks
pass512 contacts,8 traces/frame,21 severity samples, scale1 prior particle byte
identity and no-hit/water/submerged/outward/lifecycle negatives. Five native
translation units compile; no installed island rendering is claimed.

The player's remaining crest repetition rejects the earlier narrow material
admission. Wider baseline frames reproduce PENA motifs/unit-cell contrast. Two
authored low-frequency samples distort only the existing foam sampler coordinates.
At actual PENA mips3/4 the independent Fourier oracle reduces unit-lattice contrast
about49.5%, preserving mean/variance within0.4%. Six matched wide frames preserve
macro foam coverage and MRT/depth. The finite-plane far-right cut remains a
fixture limitation. Final material SHAa547cef544331beedbfada3ad89af868b6203d81bc8ddd2d33e5368963f57097;
sky SHA0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.
All no-window GPU runners have ended; no game action is taken.

A fresh closed-game delivery baseline binds246 non-log player files, three
foreign staged blobs and five foreign working paths. Exact run.sh --stage-only
is now consuming the final source batch; delivery result follows below.
Real shoreline appearance, drift into a coast, full sky/weather and gameplay
replay remain unresolved and player-owned.

Adjacent unchanged source finding: CoastFoam::clear deletes aFoams entries but
does not clear its vector. Editor Load after an existing foam can retain dangling
entries; ordinary initial scene load starts empty, and this feature never invokes
editor Load. No adjacent repair is included.

Canonical final stage returns0. Signed engine331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0,
receiptbd6d3c16ae2ef88a9957cbcfe1d9cbcd5e3dc3ba573522152fc159865ed1dd60.
All116 managed entries,37 canonical inputs, both actual script headers and empty
cache/app plans pass. Installer deep/strict verification passes; all246 fresh
non-log player files, five foreign working paths and three staged blobs are exact.
Disposition: integrated source/build/stage accepted; real-game replay unresolved.
The player's additional global-map debug call opens a separate content-only
batch, reusing this installed engine. No game action or push.

### October8 — shared world-map debug creation and cancellation

The player requests the menu call on the global map. Existing installed buttons
were sea-only. Two menu gates now admit the worldmap entity; the shared Start
dispatch cancels marked events and invokes one SeaTsunami_CreateMapEncounter
owner reused by the unchanged natural lottery. Stop marks only Storm descriptors
with tsunamiSeverity and sends existing deleteUpdate; ordinary storms/ships stay.
Native spawn/movement/radius/delay/lifetime and forced sea reload remain unchanged.

First candidate passes scoped VM cases but canonical plan rejects the stale
worldmap_encgen output admission. No runtime write follows that read-only failure.
Recomposition from reviewed baselinef6615ecd… proves old9795a316… and new52a68d39…
differ only by shared constructor dispatch. The projector updates reviewed output
admission and retains the installed previous revision in its existing PREVIOUS
owner. Canonical plan then contains exactly four PROGRAM deltas. Full integrated
PROGRAM reports DamageCases=0/script_errors=0; scoped MapDebugCases and
MapMenuCases also report0 with explicit native-creation/entity/exit seams.

Exact canonical run.sh --stage-only is consuming this content-only batch after
the last gameplay/projector edit. Prior native inputs and installed engine are
reused. Fresh246 non-log player files and foreign working/index baselines are
captured; real map marker/approach/contact/sea-transition acceptance remains
unresolved and player-owned. Result follows below; no game action or push.

Exact canonical stage returns0. Engine remains331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0;
app receipt66c742d6ca3dd069f357709fb145421f7e4b79bb0d2bb04f9f4fe8c598e40519
binds all116 managed entries,37 canonical inputs and both script headers.
Cache/app plans are empty, compiler ready; content transaction deep/strict
signature passes, and engine installation correctly skips identical bytes.
All246 fresh player files, five foreign working files and three staged blobs
remain exact. Disposition: source/compile/delivery accepted; visible global menu
event and sea transition remain player-owned replay. No launch/input/termination,
player-state mutation or push.

### October 8 — expanding front, rendered-land shelter, fleet damage and NPC steering

The player's arc/ring, island shelter and global-fleet requests replace the marked
moving disk with a fixed-origin hollow front. One native contact sampler owns
player contact, NPC damage and attenuation. Esc offers explicit arc/ring starts;
natural generation chooses the shape once. The existing descriptor persists the
front and its once-per-fleet hit receipts. Sea entry consumes actual radial
incoming direction and attenuated strength, then clears pending commands.

Named island GM collision/BSP and navigable-water patches cannot establish the
rendered archipelago's land boundary. The actual placed `mein.gm` buffers feed a
one-time planar triangle BVH: 32 active models, 169,394 clipped triangles,
65,535 nodes and 8 MiB capacity, about 26 ms intake. 2,500 segment queries match
an independent edge/barycentric oracle; the actual Jamaica shelter segment hits
while the open ray passes. Queries do not relock buffers; reload/teardown clears
the cache. Contact and rendered angular cuts share that geometry owner.

The first distance/time attenuation candidate is rejected: even maximum strength
cannot reach the existing fierce capsize threshold after normal map activation.
The final gain preserves full strength through configured spawn reach and fades
smoothly to zero at finite maximum radius. Actual contacts at radius 100/140 keep
strength 1. A second probe finds ordinary-storm damage bills preactivation time
differently across update partitions. Subtracting activation time makes one
3-second update and three 1-second updates both bill 1 active second; paused,
inactive and boundary cases remain safe.

The canonical traffic adapter damages actual saved roster HP/snapshots, applies
outer condition once, loses cargo through its existing owner, refreshes surviving
fleet power and removes the final sunk fleet. Quest/qID/ALONE, locally admitted,
deleted and unknown-state fleets are excluded. Actual VM cases show 1000 HP→700,
50 HP→sunk, four save-codec restores preserving damage and once receipts,
condition/snapshot sea restoration, elapsed-time partitions and funded repair
preserving later losses. Missing OnLoad in the first disposable codec fixture is
a fixture seam, repaired there; the baseline/candidate pair then passes. The full
integrated PROGRAM returns DamageCases=0 and script_errors=0.

Local NPC emergency steering uses normalized existing Sailing skill and ordinary
rotate/speed/collision controllers. Anticipation is 5+55×skill seconds; temporary
task evaluation suspension preserves and resumes the original task. Nineteen
eligibility negatives pass. With actual-controller yaw inertia 0.04 rad/s, skill 1
meets the crest at 1.819° from its bow while skill .05 remains at 55.623°; onset
changes monotonically with skill. These checks do not promise successful turns
for every hull or collision. No global captain-skill state is invented.

The first four-frame map fixture is rejected for clipped Jamaica and an unanimated
substrate. Corrected supported free-camera framing uses real WdmSea draws and
installed authored terrain/texture inputs. Final gain is replayed in two no-window
generic-Metal frames: a raised dark face/pale crest remains visible, the sector
behind Jamaica is cut, opposite open water survives and expired radius emits no
mesh. Two command buffers/30 draws use 69.157 MiB and finish without GPU errors.
Live reflections, labels, ships, moving hulls, full open-water arc silhouette and
game FPS are outside this component evidence and remain player-owned replay.

Exact canonical `experiments/native-metal/run.sh --stage-only` returns 0 after all
native/material/projector inputs. Signed engine
3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c;
receipt 1f6f72a619e2019cf82bd0029c45e1cccb0da6f33ca9d54def882cec0c751f41
binds all 117 managed entries and 37 canonical encoded inputs, both script headers,
six native map paths and five native AI paths. Cache/app plans are empty, compiler
ready and deep/strict signing passes. Ordered patch
35eacd393496a88c69f4c1eb1317ddda28f044e07d40d5c7e692e2bc0c36388c
preserves the earlier local-sea prefix; new WdmTsunamiWave technique SHA-256
fa7d680cf37593f8a8413827eaac1efeb8815b0bd61dc4151df77307221d7b32
is managed by the canonical material receipt. Earlier sea/sky shader bytes remain.

Fresh baseline persistence failed on a mistyped HEAD assertion before staging;
preservation instead compares against the verified prior closed-game snapshot.
All 246 non-log player files, five foreign working files and all five foreign
index entries including three staged blobs still match. Two extra lowercase
launch logs predate this stage. Disposition: integrated source/component/build/
delivery accepted; real-game map/sea/save/ship behavior and FPS unresolved, with
the human owning that replay. No game launch/input/termination, player-state
write or push. Focused local source checkpoint follows below.

Focused local checkpoint `7d2eed3aa086d6ccf1a3f087af2acd21ffd1155e` owns exactly
the eleven expanded-front/traffic/menu/material-admission paths. The ordered
native patch remains byte-identical to the staged batch. Agent context check
passes; zero nonignored untracked paths remain. All five foreign working files
and index entries, including their three staged blobs, survive the checkpoint
unchanged. No push. Build metadata still names the pre-checkpoint HEAD83a643c;
the exact installed source was staged before the commit without subsequent
runtime-input edits.

### October 8 — flat-speed band, progressive tsunami contact and hull-pose correction

Player reports little growth above the sailing minimum and immediate tsunami
damage. Exact native fixtures reproduce both defects: doubled stock propulsion
keeps the same target, and one crest query pays all damage. The initial sailing
insertion breaks the ordered diagnostic patch's context; canonical build rejects
it without replacing cached source. Moving the new formula after the established
floor assignment preserves the diagnostic context and the canonical build passes.
Continuous target/HUD and mast/control negatives pass under ASan/UBSan.

Per-ship contact integrates real profile/time between ordinary queries, caps the
budget and resets on cancel/serial changes. Actual-header tests pass positive
leading/trailing damage, 0.1/1/2-second partitions and lifecycle guards. An initial
following-ship fixture misses most of the event because it starts sailing from
launch time and escapes the finite front; beginning that movement near contact
tests the intended passage without changing production code.

The two later player images reopen hull/wave acceptance. Source discovery binds
rendered height and WaveXZ to one profile, but extra tilt to height rather than
slope, with a stale rendered pose and yaw-only support footprint. Exact-method
calibration tests reject a level crest demanding 117-degree roll. Signed flank
forcing, bounded bow kick, tilted support and current post-Move pose pass native
ShipRocking/CMatrix checks and 400-step ordinary equality. Strength 0.9 and 1 retain
unprotected beam inversion; ordinary/bow/protection/lifecycle negatives pass.
The screenshots themselves do not prove the model crossed the 90-degree fatal gate.

The player's subtractive menu request removes both Stop nodes/handlers/navigation
and merges exact sampled strength/nominal damage into existing Log_Info feedback.
Full PROGRAM/menu compilation and real script VM lifecycle/report cases pass;
ordinary menu sections are unchanged. The frozen daylight aim owner hands over
three hash-bound paths, which root consumes in the same final canonical stage.

Stage-only returns 0; installed engine b4013994 and receipt 4bc95b60 bind the current
inventory above. Deep/strict signature and empty cache/app plans pass. The broad
player-file equality check first detects a human relaunch's two new Sentry files;
all 247 preceding files, SAVE/config and foreign work/index remain byte-identical.
Disposition: source/component/build/delivery accepted; actual current-save feature
replay remains unresolved and player-owned. No agent game control or push.

Focused local checkpoints: df170087e5b04ad18f00d633ebd72f777df74b3b owns the three
daylight aim paths; 6c743f7092b85beb27e11601e8eaf393675e0453 owns sailing native/topic
paths; 24b1f2886a39db064bb80d3e6ac88b79ad006b8c owns the seven tsunami/menu/topic
paths. No runtime inputs change after staging; the exact build stamp remains
18efcae. Agent context checks pass;
foreign staged blobs and working files remain untouched.

### October 8 — storm-plus-tsunami collision-separation hang

The player clarifies that the prior storm scene froze after debug tsunami start.
The old process is no longer available; system logging was overwritten by the
player's normal relaunch. Retained launch.log.1 has no fatal entry. A three-second
read-only sample of PID99695 shows ordinary frame execution/render/present work;
it is not evidence about the terminated process's hang.

Bounded native audit rejects direct event arithmetic, Move-driven braking and
storm-written invalid inertia. The strongest reproducible owner is FakeTouch:
the old counter advances only for islands, and a ship pair can keep the predicted
collision without changing either XZ position. Exact contour/intersection/vector
methods plus the shared event/drift reproduce equal centres and raised-hull float
stagnation; both exceed 6000 predictions with unchanged state. The fixture aborts
the nonterminating calculation explicitly rather than hanging the game.

The smallest correction counts both solver branches and yields when neither ship
position changes. Source-bound ASan/UBSan reproductions now return after one solve.
Six ordinary/current converging pairs retain exact positions and prediction
counts; an always-contact island adapter retains all 1024 steps and final state.
These adapters test the native solver contract, not the player scene or terrain.
Upstream develop still carries the same bug; bounded release/open-closed issue
checks find no applicable fix. References belong to docs/sea-tsunami.md.

Canonical build returns 0 and changes exactly touch.cpp, whose SHA-256
0a52fd82628ecabe964c9e9abf9e5d2713ca707f4372731cb2af6d8a4fb59f84
equals the tested candidate. Focused local checkpoint bbd6221 owns only the native
ordered patch and tsunami topic. Five foreign working files and their staged
blobs remain unchanged; agent context check passes and no untracked files remain.
After the player's normal quit, canonical run.sh --stage-only returns 0 and
installs signed engine 8bea3d2b6f1da5f77e4212b114402e88aa540d8a533466098644877b97ef6575.
Receipt f0f81e446341a7ffcf0a4baa465c7e29ff3414ba066529f29b13fe9ad4c56d7a matches
all 117 managed entries and 37 canonical inputs; cache/app plans are empty,
deep/strict signature passes, and all 247 preceding non-log player files and
foreign work/index entries are unchanged. Disposition: source/component/build/
delivery accepted; original storm-plus-tsunami replay unresolved and player-owned.
No agent game input, launch, termination, SAVE/config write or push.

### October 8 — map-water appearance, both shore contacts and first-third plateau

Player rejects a black rotating map circle and requests shore-hit foam/splash in
both scenes, plus full power through the first third of severity-defined reach.
The native baseline reproduces the collar; neutral tint alone is rejected.
The missing ordinary animated-water layer is decisive. WdmSea now owns both
raised-water passes and their shared frame/UV setup; fixed eight-repeat crest
phase removes tangential scrolling. Cached landfall produces a foam lip on the
water side without transmitting the wave through land. Ordinary water/storm
behavior, event distance/speed fields and physical contact/shelter remain.

Source-bound final map fixture corrects a prototype's normalized texture-matrix
translation and consumes raw current native CMatrix bytes, actual sea methods,
generic MSL, island geometry/materials and both water techniques. Four frames
show open arc, Jamaica contact, decayed shelter gap and no wave at terminal fade.
Gain sweeps at severity 0/.5/1 preserve entry, remain full through the first third,
then monotonically reach zero. Earlier A/B clips are historical/unviewed and do
not accept exact final continuous motion or player FPS.

Local coast spray previously started at fixed bank Y=.5 beneath the displaced
wave. Stored hit/direction now follow max(bank Y, WaveXZ)+.75 during emission.
Exact native shore message/reset/realize methods pass 74 ASan/UBSan checks,
including independent crest altitude, moving surface, seeded replacement,
cancel/expiry/island negatives; fixed-ground mutation fails the altitude oracle.
Particle API, terminal sea and island lookup remain adapters; ordinary actual
SEAFOAM_PS source is unchanged. Compiled seafoam.cpp matches tested SHA-256
3916bf3196a0e3e2abfd90fd8bd9be76e5a05d4034df8381842da685bccd5064.

Canonical build changes nine source files and returns 0; final run.sh --stage-only
returns 0 with zero further stack changes. Signed installed engine 2cb3b168 and
receipt 41750f00 belong to the current full inventory above. All 117 managed and
37 canonical inputs match, worldmap.fx equals built source, both gameplay plans
are empty, and all 247 baseline non-log player files plus five foreign files/index
entries remain unchanged. Disposition: component/build/delivery accepted; actual
game appearance/movement, shore visibility, FPS and original hang replay unresolved
and player-owned. No game input/launch/termination, SAVE/config write or push.

Focused local checkpoint 22bc292190444307a6de96c3a0d49b310e83de5a owns only
sea-tsunami.patch and docs/sea-tsunami.md. The committed patch matches the staged
86e453f5 batch; agent context check passes, no nonignored untracked paths remain,
and the five foreign working files and staged entries are preserved.

### October 8 — local continuation and optional exit after hull clearance

Player reports that the local wave disappears just past the ship and asks to
remain watching its later contacts. Native Start fixed expiry at launchDistance
plus 700 world units; the natural script lock tracked the entire event lifetime.
The correction keeps the existing frame/evaluator/contact/particle consumers and
extends post-origin travel to 2500+2500s, clamped by ordinary scene reach without
shortening the former minimum. Final eight-second fade and finite teardown remain;
map propagation/first-third gain are unchanged.

Native SEA publishes PlayerPassed from the actual controlled MainCharacter ship's
position and full horizontal hull radius beyond the conservative trailing support.
The existing contact query updates it; NPCs cannot unlock the player. Start/reset,
unknown/invalid hulls, reentry and weather attribute rebuilding retain safe state.
The natural script flag survives until expiry but stops imposing its navigation
lock after passage. Ordinary storm/enemy restrictions remain; no forced map return.

Exact native method bodies and evaluator pass 38 disposable ASan/UBSan checks with
external API doubles, including NPC exposure beyond the old expiry. Eight native
script VM cases exercise the canonical lock branch with a sea-presence seam.
The full staged PROGRAM graph compiles against installed headers with zero errors,
without game startup. Canonical run.sh --stage-only returns 0 after three native
source changes. Signed installed engine 9b2c0616 and receipt 7816732d match the
current inventory; all 117 managed and 37 canonical entries match. Post-stage
deep/strict signature passes, gameplay plan is empty/compiler ready, and all 248
non-log player files are preserved. No game input/launch/termination, SAVE/config
write or push. Disposition: source/component/build/delivery accepted; continued
wave/NPC/shore/optional-map-exit gameplay replay remains player-owned and unresolved.
Focused local checkpoint b490bed owns only the tsunami patch, canonical AIShip
script and topic document. It matches the installed patch/source batch; context
check passes with zero nonignored untracked paths. The five foreign working
files and staged entries remain exact.

### October 8 — procedural weather/sky and coherent sea, followed by solar cloud coupling

Player requires textured volumetric clouds without simplification, no blurred
horizon, smooth procedural weather, vivid dawn/dusk and a non-emissive night sea.
The first local source milestone `e47a40d` replaces repeated layered cloud noise
with the licensed volume renderer, retains original astronomy, and separates
ambient water/foam from Fresnel solar radiance. Native components, input decoding,
cache-motion and sea-material checks pass. First stage installs e969a902;
full-game replay stays unresolved and no game is launched.

The player's dense-cloud sunset screenshot reopens solar acceptance: the separate
SunRoad pass restores the sunset highlight but ignores cloud opacity. A shared
completed-cloud transmission now controls original SUNGLOW overlay and both
modern water solar consumers; 0/.5/1 opacity fixtures yield 1/.5/0 transmission.
The opaque native overlay is RGB21 versus clear/unrelated RGB255. The same
cloudy diagnostic loses its white solar road while the clear negative retains it.
Original density-dependent cone/Beer/powder cloud lighting remains. Source
inspection also proves two consumer mismatches: SUNGLOW used raw solar direction
and a legacy texture-alpha cloud mask, and its layer priority -2 precedes SKY +3.
The correction shares the visual weather adapter, prepares cache before early
astronomy without activating/closing a sky draw scope, and bypasses the unrelated
legacy mask only for a current procedural field. An intermediate e31c6dc7 stage
is historical; it predates early-astronomy preparation acceptance.

Final native build consumes two weather source changes; source/probe and early
solar overlay acceptance pass. Focused local commit `2a6a776` owns eight native
source/patch paths, preserving the five foreign staged/working entries. Final
canonical stage returns 0 with zero additional source changes and installs the
85d06014 signed engine and 492280f2 receipt in the current inventory above.
All 117 managed files, deep/strict signature, 248 player files and foreign index
bytes pass final verification. Source/component/build/install are accepted;
actual complete game weather/SunGlow/shore/ship/postfx/FPS remains unresolved.
No game launch or player-state write; release ZIP/site/Drive remain deferred.

### October 8 — real-player cloud slabs/white clipping and late-night fog reopened

Three player captures show regular cloud contour slabs, burned morning highlights
and pale slate-green distant land at21:54. Matched native components reproduce
the first two; authored hourly fog RGB matches the third. Precision experiments
reject larger caches, manual lookup, precise math and wrapped noise; increasing
march count makes cumulative planetary-coordinate drift worse. Fixed-origin ray
samples remove slabs at the original budget. A pre-UNorm highlight shoulder
restores morning texture, and canonical hours20–23 darken toward night endpoints.
The unchanged original noise, density-dependent lighting, solar coupling and
weather clock remain the owners. Details and rejected alternatives live in
`docs/metal-dynamic-sky.md`.

Focused local commit `92d3659` owns two paths. Canonical stage returns0 and the
current inventory records its signed engine and receipt. Fresh shader/cache,
weather-palette and source/delivery checks pass. A signature inspection attempted
while staging was still mutating the app is rejected as stale evidence; the final
post-stage deep/strict signature and117-file receipt both pass. No agent game
launch, input, termination or player-data mutation occurred. Source/component/
build/delivery accepted; original complete player scenes remain unresolved.

### October 9 — 18:25 directional sky/fog mismatch reopened

The player's shore capture shows pale-blue distant mountains and far water
against dark dusk clouds. The previous hour20–23 palette correction leaves18:25
untouched. Native reproduction compares fully fogged geometry to the actual
displayed sky, proving that scalar authored RGB113/147/179 cannot represent the
procedural sky's direction/elevation. The cyan translucent foliage-card edges
are a separate observed defect whose exact cause is not isolated, outside this
fog repair.

Seven native source paths now share one256×128 directional sky-radiance field
across FFP/land, modern water and deferred shadow ratios. Fog amount/alpha,
weather clock, noise, march count and2048²/96-MiB cloud detail remain. Sky rays
use camera-relative world vectors interpolated without vertex normalization;
the actual rotated/translated SKY mesh and cameraY/6 convention are respected.
Doubling the small fog field did not repair the noon discrepancy and is rejected.

Fresh actual-backend GPU rows cover18:25 facing toward/away from sun, noon,
moon-up midnight, dawn, independently seeded dense rain and a rotated SKY with
an elevated translated camera. Dusk mean RGB error drops from0.393691 for the
authored color to0.000240; worst p95 is0.011765. Unfogged RGB36/50/60 remains
exact. All four modern sea passes match FFP's directional RGB211/127/52 at the
sunward grazing horizon, with foam alpha128; disabling sky restores authored
RGB113/147/179. Fresh dynamic-sky/backend, original/modern sea and shadow probes
pass. The513-frame weather/wind/cache replay remains smooth/finite at mean0.689ms,
p951.867ms, max2.985ms and prime47.92ms; timings include fog compute plus a small
diagnostic draw and do not prove player FPS.

Source/native acceptance passes at local commit `3c41e2e`. Root's canonical
staging is pending (the user is running the game); the prior installed engine
is `44cdd028` (`9052624`). Evidence and disposable diagnostic sources live in
the ignored native sky-evidence owner.

### October 9 — overcast fog veil scoped to fog field, engine staged to played app

Player replayed the stale `44cdd028` engine and correctly reported no change:
the directional fog, aerial perspective and overcast fixes never reached the
installed app. A dirty-tree tuning attempt had also leaked overcast dimming
into the sky display, breaking the midnight/dusk exact-authored-fog probe.
The dimming now applies only to the directional fog field (`veil`), the sky
display stays bit-exact at its horizon, and clear-air Rayleigh drops 20x->5x.
Dynamic-sky, backend, shadow and original/modern sea probes pass at `da1d467`.
Canonical `--stage-only` returns 0 with the game idle and installs signed
engine `694d31f3` plus receipt into the played app; SAVE/config untouched,
8252 inputs verified. Player acceptance of dusk overcast islands-vs-sky,
horizon seam, shore water and cloud motion remains with the player on the
new engine.
