# Debug tsunami

## Status

October 8, 2026 installed in the current canonical stage. Signed engine
`5b14e3196d4674c7cc296de97debc83fc26fbf7a8a8eb88b41f4cf40989221a7`;
receipt `0ebc514c8b38488591258f7a36a94c33e6e3301a836f8108c46d8e6bae0fdce4`.
The player reports that F11 opens no menu. The source now adds a sea-only action
to the existing Esc pause menu; delivery of this follow-up remains pending.
The no-window input/script/native-command chain and actual SEA scalar/render
sampler pass; physical-key delivery, visible menu and real-game wave replay
remain unresolved. This is an exaggerated sea experiment, not a
coastal flooding simulation or a new random weather preset.

## Player action

The prepared follow-up uses **Esc → Цунами** in an ordinary local sea scene
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

## Owners and contract

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

Existing ship buoyancy, camera and damage rules remain the consumers. There is
no scripted damage, knockback, reward, random spawn or save migration.

## Acceptance

Compile the scripts and canonical native stack, stage through
`experiments/native-metal/run.sh --stage-only`, then replay the installed app:
open Esc, start, observe horizon approach, crest crossing and ship response,
cancel/restart, pause/resume and sea teardown. Compare ordinary sea after cancel
and verify that aiming follows rendered water. Source/math probes prove
only their properties; they do not accept appearance or gameplay consequences.

The isolated complete PROGRAM startup/debug segment compiles with zero errors.
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
three canonical inputs after newline normalization. The compiler receipt binds
`sea-tsunami.patch` to
`a24fe3e7f5e03a3ce4e2f1adcd82e2a89c34e4078e02610f325a59431eb8f697`.
The earlier combined delivery's player-file preservation and responsive sea
startup do not accept a tsunami action. The exact pending player scenario is
the physical key, visible menu, start, approach, ship response and cancel on the
installed app. The prepared pause entry also requires a real-game menu replay;
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
