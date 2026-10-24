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

Installed-byte verification on 2026-10-23 passed for the eight selected
gameplay/control resources, 49 generated rig resources and perk icon (58 total).
The signed installed engine SHA-256 is
`688efcaff59fb0083250542d2f09f8680eacabbc557988f822ea0bb2c1ff8486`.
The ordinary installer completed signature verification and retained its
previous-engine backup. The selected delivery preserved unrelated changelog
and player configuration bytes. These results establish installation and
offline acceptance; interactive combat and full-scene frame time remain unplayed.
