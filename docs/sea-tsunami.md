# Tsunami: local sea and world-map encounter

## Status

October 8, 2026: expanded material/ship/world-map source passes canonical staging
and is installed; the current receipt belongs to `docs/runtime.md`. The player confirms
the installed Esc button starts the wave. Its supplied screenshot rejects the
straight, repeating white crest. Physical F11 delivery remains unresolved.
New waveform, splashes, damage, capsize and natural encounters require player replay.
Horizontal wave transport and the separately reported sky-horizon haze correction
are the next active source batch; this installation does not yet include them.
The current installed inventory and chronological receipts belong to
`docs/runtime.md`; component probes do not prove real-game acceptance.

## Player action

The installed follow-up uses **Esc → Цунами** in an ordinary local sea scene
where the player controls the ship, including sailing beside a pier. **Esc →
Стоп цунами** cancels it. Both actions resume the game through the existing pause
menu exit. The buttons are absent on the world map, on land and during boarding.
They occupy two free cells to the right of the existing pause controls.

The existing F11 route remains available in source (Fn+F11 when macOS uses media keys). The script
debug menu contains `Цунами` and `Стоп` in the two free cells of its lower row.
`Цунами` closes the menu and starts one wave ahead of the ship's bow, travelling
back toward the position where it was called. Reopening the menu and choosing
`Стоп` cancels it. A repeated start replaces the previous wave.

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

Native contact sweeps the crest between ship queries: full crossing pays once,
grazing pays actual peak exposure. The existing hull owner preserves immortality,
reload/dead guards and battle armor. `StormProfessional` («Плавание в штормах»)
multiplies damage by 0.7; `DontHitInStorm` prevents it. Natural tsunami starts
ordinary sea weather, avoiding generic hurricane/tornado damage on top.

Native SHIP adds damped pitch/roll after ordinary buoyancy, before model placement.
Maximum unprotected pitch/roll is 1.25/2.05 radians, scaled by `s²` and actual
local contact; the storm perk reduces motion too. Script `Ship.Impulse.Rotate.z`
is an invalid substitute: `SHIP::Move` integrates yaw, while `ShipRocking` owns
the other axes. The native final pose is the authoritative response owner.

Fatal capsize requires live contact, heading **50–130°** to wave travel on either
side (90° ±40°), and actual model up-vector at or below zero. Strength must also
permit a 90° roll: `2.05s² ≥ π/2`, hence `s ≥ 0.87535256`, near **45.0%** nominal
damage. This eligible tail is about **6.4%** of samples; actual deaths also depend
on heading, exposure, defenses and prior pose. Bow/stern approaches are excluded.
Death uses normal `ShipDead` hooks. A synchronous `Ship.TsunamiCapsizeBeam` marker
chooses the continuing sink direction and is immediately removed. Native
`SetDead` transfers tilt into the existing death pose; no ship save fields added.

## Natural world-map encounter

The existing saved `Storm` descriptor owns a `tsunamiSeverity` marker, including
valid strength zero. It is sampled once and reused on restore/sea transition.
`tools/metal_living_caribbean.py` composes generator/reload in their existing
owners. Importing a full generated script would skip traffic/fleet projectors.

Rate `0.000001` allows at most one marked event alongside the ordinary storm
limit. Existing encounter-off, pause and no-encounter gates remain. The cumulative
lottery makes spawn waiting roughly ten times the rate-0.0001 storm under matched
active-map conditions, not a 100-fold guarantee. Player frequency is unmeasured.
Native movement is 5–8 map units/s, lifetime 60–120 seconds, activation delay two
seconds and existing spawn distance 100–140 units. The live trigger is a
**60-unit circle** plus the player's existing 16-unit action radius. Its blue
circle is visible without global debug; strength changes its color. Ordinary
storms retain six-cloud collision; marked outer proximity cannot admit hurricane
weather outside the actual circle.

`TestInStorm → WorldMap_PlayerInStorm → wdmReloadToSea` forces local sea without
the skippable encounter dialog. Reload consumes the descriptor and copies its
strength into `Login.TsunamiSeverity`. `SeaLogin` consumes this before early exits;
after successful sea creation `Sea_FirstInit` invokes the shared start owner.
`bSeaTsunamiEncounter` extends the existing map/land/SailTo lock only for a natural
wave while native `Sea.Tsunami.Active` is true. Finish clears it; debug does not
impose that lock. Sea teardown/load clears flags and pending commands. Attribute
clears during weather/cabin rebuilding republish actual native active state,
preventing a stale projection from unlocking a surviving event.

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
