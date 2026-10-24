# Spinning heel kick and physical knockdown

## Requested outcome

The player performs a full-body spinning heel kick that connects with an
opponent's face. The opponent transitions from the current animated pose into
an articulated physical fall, with inertia, gravity, joint limits and collision
against the actual location. An ordinary hit/death animation is insufficient.

Learn the personal `RoundhouseKick` perk, then press K in combat (35 energy).
The controls settings list “Удар пяткой с разворота” and retain a saved remap.
The debugger's all-perks command includes the new perk.

Verification uses the real native CPU animation/physics paths and offline skinned-mesh frames;
those checks do not establish played-scene visual acceptance or full-scene FPS.

## Owners and delivery

- Character/player owns input, wind-up, contact, damage and recovery. Add a
  separate kick action; existing sword attacks keep their meanings.
- The existing perk registry owns a learnable personal `RoundhouseKick` perk;
  the debugger's dynamic perk grant includes it automatically.
- `tools/prepare_roundhouse.py` owns baked companion animation banks, semantic
  rig/body mappings and their finite resource manifest. Reuse rowboat AN/FK/IK
  facilities; resolve each distinct rig's topology instead of copying bone IDs.
- Animation owns per-instance external skeletal poses used by both CPU and GPU
  skinning. No shared bind/animation data may be overwritten by a ragdoll.
- A location owns its lazy ragdoll world and actual collision triangles.
  Character deletion, model replacement and location unload release bodies.
- The established native build and ordinary staging/install owners deliver the
  static engine dependency and generated resources.

## Reuse decision

Existing `attack_round_*` clips are sword attacks. Storm's animation service can
play baked quaternion/root-position banks, and rowboat authoring already parses,
retargets and serializes them. Existing character hit/knockback paths do not
provide articulated physics. `LOCAL_COLLIDE` is unfinished; use the implemented
`COLLIDE::Clip` polygon callback for location geometry.

Use [Jolt 5.6.0](https://github.com/jrouwe/JoltPhysics/releases/tag/v5.6.0), pinned
by archive hash through CMake FetchContent, for bodies, limited constraints,
contacts and sleeping. Its [ragdoll API](https://jrouwe.github.io/JoltPhysics/class_ragdoll.html)
supports animated-pose initialization and physical pose output. This avoids a
new project-owned articulated-body solver. Compile statically; the installed
game has no downloaded or separately running physics prerequisite.

For motion repair, retain the established AN/FK authoring path. Adobe's
[Mixamo workflow](https://helpx.adobe.com/creative-cloud/help/mixamo-rigging-animation.html)
provides ready character animations, but this project has no FBX retargeting
intake. The bounded repair changes timing and foot placement in the existing
producer; its combat reach and motion still require their own acceptance.

## Acceptance and nearest negatives

- A supported protagonist has planted pivot, torso turn, leg chamber, heel
  extension at face height, follow-through and return to stance.
- Contact uses the animated heel path and the target head, with a wall/visibility
  check. A miss, a wall or an out-of-range target produces no impact/damage.
- Contact happens once per kick. The player cannot interrupt the strike into
  repeated damage. Ordinary movement, sword attacks, dialogue and pause retain
  their existing behavior outside the kick.
- Ragdolls start from the victim's current full-body pose and receive impulse
  at the head. Bones remain connected and collide with real ground/walls.
- A living victim can recover; dead victims retain their normal death/loot
  lifecycle. Cleanup or leaving the location cannot retain bodies or pose locks.
- Idle locations allocate no physics world and perform no physics updates.
  Fixed-step active physics is bounded; measurements distinguish CPU proxies
  from unmeasured full-scene FPS.

## Runtime and evidence

Current native build: Release, Apple clang 21, C++20, macOS 15 minimum; CMake
4.4.3/Ninja 1.13.2, Python 3.14.7 for asset preparation. Existing build inputs and
toolchain remain authoritative. Reuse the current build slot with two workers.

Baseline, actual rig/mesh phase evidence, physics measurements and exact
reproduction commands belong to the [evidence owner](evidence/roundhouse-kick/README.md). Before/after
animation timing must use the same native CPU invocation and retained pose
quality; it is not a substitute for scene FPS. The task remains open until
implementation, appropriate checks and installed resource/engine parity pass.

The [native animation comparison](evidence/roundhouse-kick/animation-cpu-probe.md)
preserves all eight pose sequences and all 56 timed checksums. The
[pose API probe](evidence/roundhouse-kick/native-pose-api-probe.md) checks shared-rig
instance isolation, override lifetime and return to ordinary animation.
[Real-rig physics results](evidence/roundhouse-kick/backend-real-rigs-raw.txt)
cover all 24 supported skeletons and native collision winding from three towns.
The [script replay](evidence/roundhouse-kick/script-vm-acceptance.py.txt) compiles
the complete current PROGRAM and exercises moving-corpse inventory lookup.

Recovery uses a collision-supported navigation patch beneath the physical body.
If physics carries a living actor off the walkable patch, recovery returns to
its validated pre-fall anchor after five seconds; that fallback relocates rather
than sliding the body through intervening geometry. Actors without a valid
anchor cannot enter this knockdown path. Moving corpses retain inventory lookup
at their current physical position, while normal fade, lights and deletion
continue through the character's common update path.

The first candidate's installed-byte verification on 2026-10-23 passed for the eight selected
gameplay/control resources, 49 generated rig resources and perk icon (58 total).
Its signed installed engine SHA-256 was
`688efcaff59fb0083250542d2f09f8680eacabbc557988f822ea0bb2c1ff8486`.
The ordinary installer completed signature verification and retained its
previous-engine backup. The selected delivery preserved unrelated changelog
and player configuration bytes. These results establish installation and
offline checks; the subsequent gameplay report reopened combat acceptance below.

## Reopened combat acceptance

The first installed candidate plays the kick but has a reported missing damage
and knockdown reaction. Its authored contact was checked at 0.7 m, while native
Supervisor collision keeps ordinary fighters about 1.5 m apart and a fighter
versus neutral about 1.1 m apart. The authored 1.4 m miss therefore did not prove
usable combat reach. Per-key smoothstep also resets every authored channel's
velocity at every key, producing repeated braking before impact.

The [native baseline replay](evidence/roundhouse-kick/native-contact-baseline-result.json)
reproduces both misses after real Supervisor separation: the closest heel/head
distances are 0.800099 m for fighters and 0.400099 m for an idle target. Both
retain 500 HP and create no ragdoll. This binds the reported failure to the
native interaction rather than an isolated animation calculation.

The correction must prove a stepped approach and heel contact at actual 1.1 m
and 1.5 m spacing, a 2.4 m miss and wall rejection, through native character
contact, ordinary damage events and physical fall creation. The authored strike
window is generated metadata consumed by the controller. Damage must not depend
on successful physics allocation. Motion must carry continuous momentum through
intermediate keys and sequence pelvis, thigh and calf into impact. Isolated
palette, mesh and physics checks do not establish this interaction contract.

The corrected clip lasts 2 seconds and steps forward 0.8 m. Its generated
strike window is 0.86–1.52 seconds. Time-aware monotone Hermite interpolation
preserves momentum through intermediate keys; the pelvis, thigh and calf peaks
occur in that order. These measurements describe authored kinematics and do
not establish natural-looking motion in the played scene.

The [corrected native replay](evidence/roundhouse-kick/native-contact-replay.md)
passes real Supervisor spacing, animated heel contact, canonical script damage
and Jolt creation in one CPU-only interaction: fighters at 1.5 m and an idle
target at 1.1 m each receive one hit, lose 58 HP (500 → 442), and advance their
physical pose by 1.249 m and 0.838 m respectively. A wall 0.35 m into the
approach rejects impact even after the animated torso crosses it. Far,
immortal and same-group targets receive no hit or body. Forced physics
creation failure still applies damage once and logs the failed stage.

The fixture uses real geometry, navigation, animation, character, VM and
physics owners; only rendering-dependent presentation is suppressed, including
blood particles. It creates no game window and does not establish interactive
player acceptance. Reproduction source, adapter boundaries, commands and raw
output are retained beside the result.

The corrected candidate was rebuilt through the normal native build and
installed on 2026-10-23. Installed-byte readback matches all 58 selected
resources and the signed engine SHA-256
`7c6106e9755b50e34cee9e0d2e47f30bb69e7f7eb12c4a5e938c4c75649e5ddd`.
Default K is present; engine configuration, saved control options and unrelated
changelog bytes retain their pre-install hashes. The installer retains the
previous signed engine for rollback. Interactive motion acceptance and
full-scene frame time remain unplayed.

## Accuracy while circling

The player reports frequent misses during circular combat movement. Ordinary
melee selects its enemy through `Player::FindAttackCharacter` and turns in
`Character::Attack`; the kick instead captures its initial yaw for the entire
clip. A forward stationary fixture does not cover an offset or moving target.

The bounded correction reuses native enemy selection and turning speed during
windup, then commits heading at the generated strike start. A deleted, friendly
or escaping target must not pull the actor around during impact. Actual heel
contact, reach, walls and one damage event remain authoritative; accuracy must
not be implemented as wider invisible damage. The existing CPU replay owns
offset/orbit reproduction and the same forward-hit timing comparison. Ordinary
death ragdolls are feasibility-only scope for this follow-up.

The pre-change native replay selects the intended target but misses a 25° left
offset and both 40°/s orbit directions at 1.5 m. The corrected replay reaches
both 25° offsets and both orbit directions, once each, through the normal
500 → 442 HP and Jolt fall chain. Heading freezes before 0.86 s; contact volume
remains 0.22 m. The unchanged forward-hit replay retains identical HP/contact/
physics outputs. Three-run CPU medians are 63.406 ms before and 61.253 ms after
for the same 140-frame fixture, with overlapping ranges; this is no FPS claim.
Deletion, escape beyond 2.2 m and changed group eligibility during windup clear
the target ID without retargeting. A target moving 45° after strike commitment
does not pull the actor around and receives no hit. Steering resolves the ID
through the native safe lookup, which rejects pending-deletion entities.

The accuracy correction is installed with signed engine SHA-256
`5e05e76adbf339094bec1dc73f4b529ef2a5428c87c9b8ab03406bbce2f27923`.
Installed readback still matches all 58 existing feature/control resources;
engine configuration, saved controls and unrelated changelog bytes are
unchanged. Interactive circling/animation acceptance remains separate from the
native CPU replay.
