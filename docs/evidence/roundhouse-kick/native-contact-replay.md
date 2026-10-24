# Native contact integration replay

The rejected candidate's isolated animation/rig/physics checks did not exercise
ordinary combat spacing or the native contact → script event → HP → physics
chain. This replay closes that admission gap without starting the game or a
window. It is disposable evidence, with no permanent CMake/test target or
production test API.

## Inputs and procedure

`native-contact-replay.py.txt` compiles one CPU fixture with the cached native
build's exact compiler/link fields. It reads the cached archives without running
Ninja or changing them. The final phase replaces only `roundhouse_character`
and its layout-dependent `character` object in a scratch copy of liblocation.
One compiler, 512 MiB task scratch, finite 180-second supervisor; no install,
save write or game startup. Set `CODEX_SCRATCH_DIR` to an empty task-owned
scratch directory and run from the repository root:

```sh
python3 docs/evidence/roundhouse-kick/native-contact-replay.py.txt baseline
python3 docs/evidence/roundhouse-kick/native-contact-replay.py.txt final
```

The baseline was executed before the native repair, against installed failed
candidate resources from source checkpoint `3c9e29a`, not the changing producer
cache. It requires those failed cached native libraries and installed assets;
their hashes are retained in `native-contact-baseline-result.json`. Its exact
fixture/schema are `native-contact-baseline.cpp.txt` and
`native-contact-baseline-state.h.txt`. Replacing the installed candidate or
cached libraries changes that reproduction input; a current build cannot stand
in for the failed baseline.

The final reads the same ordinary GM/PTC/animation resources, overlays the 49
frozen producer resources, and checks each against the maintained manifest.
Exact compiler commands, native/consumer/asset/archive hashes and stdout are
retained in the corresponding JSON and raw files. `native-contact-final-first.cpp.txt`
is the exact source used for the initial seven-case final run. Its `spacing`
stdout field is the Supervisor separation before the far relocation: that first
far target was actually 3.5 m away. The requested nearer 2.4 m negative was then
replayed alone with the current source; `native-contact-final-far-result.json`
retains its distinct source hash and reports both distances. The other six
unchanged cases were not repeated.

## Exercised owners and substitutions

Real production methods exercised: Character preparation/start/heel sweep,
eligibility event, physical fall and palette update; Supervisor pair collision,
candidate selection and update; MODELR/NODER/GEOM collision against the installed
DuelField GM; PtcData against its installed patch; Location collision-world
construction/physics stepping; real Jolt body creation/pose extraction; actual
CORE entity-ID event transport into the complete canonical PROGRAM and its
normal `LAi_CharacterAttack`/damage pipeline. No contact, support or damage
algorithm is copied into the fixture. The separate script handler only counts
received roundhouse damage events.

Presentation substitutions are explicit: a memory-only GEOM_SERVICE replaces
GPU buffers/materials/textures, while native GM Trace/Clip remain intact;
fixture entity Init methods bypass renderer startup; actor Move/Calculate and
ordinary camera/GUI housekeeping are omitted. A scratch-only access projection
exposes existing private declarations without adding production APIs. The
canonical script Main is renamed and never executed. `LaunchBlood` alone is a
presentation-only no-op, with its original definition renamed exactly once;
the actual HP/defense/attribution/kill pipeline remains canonical. Every draw
adapter entry fails, no SDL/window API is called, and CORE's window pointer is
asserted null before and after every case. Fixture process exit skips GPU
destructors for its deliberately uninitialized presentation entities.

## Results

The failed baseline misses both targets: fighter separation 1.5 m gives a
minimum heel/head distance 0.800099 m; fighter/neutral separation 1.1 m gives
0.400099 m. Both retain 500 HP and create no ragdoll. Supervisor establishes
these distances itself from its ordinary collision radii.

The repaired run uses the actual man fight guard and neutral idle pose:

| Case | Result |
| --- | --- |
| Fighter, 1.5 m | One damage event; HP 500 → 442; one Jolt ragdoll; physical joint displacement 1.249021 m |
| Neutral, 1.1 m | One damage event; HP 500 → 442; one Jolt ragdoll; physical joint displacement 0.837900 m |
| Wall, 0.35 m ahead of initial actor | No damage/body, even when current torso and heel have crossed the wall during windup |
| Far, 2.4 m | Minimum heel/head distance 0.892926 m; no event, HP remains 500, no body |
| Immortal / same group | No damage/body |
| Injected backend rejection | One damage event, HP 500 → 442, no body, explicit `stage=create-ragdoll` diagnostic |

The wall uses actual rotated GM geometry, not a synthetic ray response. Its
assertion proves that a strike-window heel is within 0.22 m of the target,
current torso→head is clear, and the captured pre-step torso→head remains
blocked. Both positives run 140 frames, beyond the two-second clip, and still
dispatch exactly once. Forced physics rejection proves failure cannot suppress
valid contact damage; it does not certify a ragdoll fallback.

The native consumer now reads producer `strike_start_time`/`strike_end_time`
(0.86–1.52 seconds), captures pre-step torso visibility, and preserves actual
heel sweep/radii. Fall admission reuses ordinary valid curPos/currentNode;
recovery checks actual pelvis collision support and after five seconds can
relocate directly to the captured valid anchor without blending through space.

This proves the selected CPU interaction chain for one representative man rig,
not rendered Metal motion, every rig/scene, camera housekeeping, lethal corpse
expiry/loot, complete getup lifecycle, pause/dialog behavior or player acceptance.
Those boundaries are not inferred from this replay. The shared build, installed
delivery and human gameplay check remain with the integration owner.

## Circular movement continuation

The first integration replay placed every target directly ahead. It therefore
missed a separate aiming mismatch: ordinary Player melee calls
`Attack(FindAttackCharacter())`, and Character attack turns toward that selected
target. The kick bypassed selection and pinned its starting yaw for two seconds.

`native-contact-aim-baseline-result.json` and its raw companion reproduce this
using the installed corrected-spacing candidate's cached native archive, the
same physical assets and the actual Player target selector. At 1.5 m that
selector chooses the victim in every case. A stationary target 25 degrees left
still misses (minimum heel/head distance 0.484549 m); both directions of a
40 degrees/second circular target motion miss (0.500709 and 0.758879 m). A
25-degree right offset happens to hit the asymmetric heel arc. The exact
baseline fixture is retained as `native-contact-aim-baseline.cpp.txt`; its hash
matches the consumed source, rather than a later edited fixture.

The correction retains the native selector's target entity ID and follows it
at the existing character `turnSpeed` only during windup. Heading freezes before
the producer's `strike_start_time`, 0.86 seconds. Contact still requires the
actual swept heel to enter the unchanged 0.22 m head volume; there is no range
or damage-radius extension and no homing after commitment. Missing, escaped or
ineligible targets clear the ID without selecting a replacement.

`native-contact-aim-final-result.json` retains fourteen motion/negative cases
and six separate forward timing runs. Its exact fixture is
`native-contact-aim-replay.cpp.txt`. Both static offsets now contact at about
0.176376 m. Clockwise and counterclockwise targets contact at 0.166990 and
0.198735 m; both freeze the attacker's heading at magnitude 0.593412 radians.
Each positive delivers one canonical event, HP 500 → 442, creates a Jolt
ragdoll and advances its physical pose. All previous neutral, crossed-windup
wall, 2.4 m far, immortal, same-group and backend-failure contracts pass.

New boundary cases change state during windup, rather than only before start:
the target moves to 3 m, joins the attacker's group, or is erased through CORE.
Each clears `aimTarget` and holds the last heading without reacquisition.
After strike starts, another case moves the target 45 degrees to the other
side; heading remains fixed and the actual heel misses. Every strike-window
frame asserts that heading stays fixed. The erased-target case uses actual
CORE invalidation and native Supervisor removal without invoking fixture GPU
destructors; it does not simulate a whole engine unload.

That boundary initially exposed CORE's deferred deletion contract:
`GetEntityPointer` can still return an invalidated object until destruction.
`native-contact-aim-boundary-before-result.json` retains the failing clear-ID
assertion. The repaired lookup uses existing `GetEntityPointerSafe`, which also
checks validity. Its native candidate differs only in that lookup from the
failed boundary candidate; the respective native source hashes are retained.

For the added windup work, one scratch run kept baseline and final executables
linked to the same fixture object and the same assets. The new state field is
appended after the old layout; baseline observations read only the unchanged
prefix. Three forward positives per executable retain exactly the same contact,
HP, body count and 1.249021 m pose displacement. CPU time for 140 replay frames
has medians 63.406 ms before and 61.253 ms after (ranges 59.271–64.421 and
59.282–76.725 ms). These small, overlapping samples show no measured regression;
they are neither an FPS measurement nor evidence of a speedup. Source/resource
hashes, compile/link commands and each timing observation remain in the JSON.

Use the same runner with `aim-baseline` or `aim-final` to select this continuation.
The baseline requires the recorded pre-aim cached archive, not an archive later
rebuilt with the correction. No motion assets or ordinary death behavior were
changed by this continuation. Player acceptance of the installed result remains
separate from the no-window CPU proof.
