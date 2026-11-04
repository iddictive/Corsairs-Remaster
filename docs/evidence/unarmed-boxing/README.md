# Unarmed animation and combat evidence

## Pose repair, 2026-11-04

`tools/prepare_unarmed.py` owns the baked poses. The repair restores neutral
clavicles and complete arm subtrees, solves moving hand targets before IK,
keeps the same leg solve at rest and in motion, and closes coarse-rig finger
pads. The cross extends at chin height with hip rotation. The uppercut loads
to the punching side, chambers the fist above the pelvis, then rises in front
of the body; its opposite hand stays in guard. Action boundaries share one
exact guard. Damage, input, event timing and saves are unchanged in this batch.

The [front](rigs-front.png), [quarter](rigs-quarter.png) and
[side](rigs-side.png) sheets show all seven actions on six retained real GM
meshes: Danielle, Jessika, YokoDias, man, man1 and man2. The
[phase sheet](boxing-pose-atlas.png) and
[cross/uppercut/block motion](cross-uppercut-block.gif) show the representative
man mesh. These are depth-tested offline two-weight renders, not game/Metal
captures. The GIF uses the bank's 30 FPS; game combat events retain their own
existing phase remapping. Independent pixel observation covered the three
six-model sheets and every man cross/uppercut baked sample from the side.
This is six-mesh visual coverage, not a visual claim for all 24 rigs.

[Native measurements](pose-metrics.json), produced by
[inspect-poses.py](inspect-poses.py) and
[native-pose-sampler.cpp.inc](native-pose-sampler.cpp.inc), use the real cached
Release `AnimationImp::EvaluatePose`, including `Bone` quaternion compression
and native interpolation. They sample quarter-frame intervals across every
action on all 24 rigs (11,880 action samples per candidate). The baseline is
the installed bank set from source commit `9f4d3d3637dd340ebd09ad56870f9cd3ab8b514b`;
all baseline/candidate bank hashes and cached native object hashes are retained.
The baseline banks can be reconstructed with that commit's generator and its
unchanged captured rigs/shared authoring helpers into task-owned scratch.

The elbow internal-angle range changed from 5.23–178.72° to 18.96–157.05°.
Native maximum ankle drift changed from 0.3854 m to 0.0069 m. Every action's
native boundary joint error is zero. Uppercut loaded wrists are 0.113–0.131 m
above the pelvis and rise 0.348–0.521 m to contact. These geometry measurements
support the rendered review; they do not prove absence of every mesh contact.

The 24 AN banks retain 130 frames, 30 FPS, bone counts and total size
(3,649,136 bytes). All ANI bytes/action ranges remain unchanged. No runtime
code or extra per-frame work was added; only offline preparation changed.
Normal delivery updated the existing development runtime and installed app;
the player remains the owner of in-game visual/gameplay acceptance.

Reproduce with Python 3.10+ and the existing Pillow/NumPy authoring environment:

```sh
python3 tools/prepare_unarmed.py
python3 tools/render_unarmed_atlas.py --rig all --angle 90 --output /task-owned-output/rigs-side.png
python3 tools/render_unarmed_atlas.py --motion --action unarmed_cross --action unarmed_uppercut --action unarmed_block --output /task-owned-output/motion.gif
python3 docs/evidence/unarmed-boxing/inspect-poses.py --baseline /reconstructed-baseline --candidate experiments/native-metal/.cache/unarmed --scratch /task-owned-scratch --output /task-owned-output/pose-metrics.json
```

The native sampler compiles only its disposable adapter against existing
animation objects. It never starts the game, changes player data or modifies
the installed app. The caller owns scratch cleanup. Cache reconstruction and
staging remain the normal project commands; no receipt is edited by hand.

## Earlier CPU combat replay

The 2026-11-01 run tested the installed resources against the cached native
libraries after the engine rebuild. Installed `metal-engine` SHA-256:
`b199490e7739a5c951b9d603172d22a3b0d56bff714cecbfcdd59bce8b0fb770`.
Its Mach-O `__text` section and the staged engine's `__text` section both hash to
`3f06368861537e6015e2fc0d514da820f7bf4ca65cbfb7ed3aea2f58e4be3e85`.
The CPU executable links cached archives, not the installed executable itself;
the section match does not independently prove every linked archive's provenance.

From the repository root, with the existing cached build and installed game:

```sh
export CORSAIRS_APP="/path/to/Corsairs.app"
scratch="$(mktemp -d)"
CODEX_SCRATCH_DIR="$scratch" python3 docs/evidence/unarmed-boxing/replay.py
```

Set `CORSAIRS_APP` to the installed `.app` bundle, not its `Contents` folder.
Recorded app paths use `${CORSAIRS_APP}` as a placeholder.

The runner compiles only the fixture into temporary storage, reads
installed resources, copies installed `PROGRAM` into scratch for the VM lane,
and sets `STORM_USERDATA` to scratch. It does not start the game, access player
saves, rebuild the engine or write to the installed bundle. The caller owns
cleanup of the temporary directory. The command replaces the raw result and
input manifest for the original run in this directory.
The exact fixture is [replay.cpp.inc](replay.cpp.inc), combined deterministically
with the retained [roundhouse scene adapter](../roundhouse-kick/native-contact-replay.cpp.txt)
by [replay.py](replay.py). The original runner is retained with its app path
redacted as [replay-prior.py.txt](replay-prior.py.txt); its original and
redacted SHA-256 values are recorded separately. The current runner redacts
local paths in its output. [Raw output](replay-raw.txt) and
[input hashes, commands and outcomes](replay-result.json) capture the run;
the manifest includes the compiler, SDK headers, cached libraries, installed
scripts and relevant animation/geometry inputs. It does not preserve those
installed or cached files: changed hashes require reacquiring the original
inputs to reproduce the exact run.

The fixture exercises real equip messages, forced-mode reset, fight mode,
`Character::Attack`, base animation events and sampled pose overrides. The
ordinary man action table's four break entries are initialized to
`attack_break_1`, as in installed `PROGRAM/characters/characters.c`; actor
loading and the blade entity are presentation adapters. It uses a real PTC/GM
scene and native character/animation code, but omits camera, rendering, actor
movement and the complete game startup.

All 12 cases passed: three unarmed strikes, armed fast strike, and sword/null
weapon-handler block in the simple event adapter; three unarmed strikes, armed
fast strike, unarmed fast strike against a blocking defender, and armed block
in the installed gameplay VM. The game VM uses the installed damage handler;
`LaunchBlood` is the only replaced presentation function. It renames game
`Main` without running it, initializes synthetic actors and records events with
an added observer. Each unblocked strike delivered exactly one event and
reduced HP; a blocked fast fist delivered one blocked event and left HP at 500.
At each fist contact the sampled pose matched the selected contact phase, and
its palette differed from the base animation. The armed strike used no fist
bank. The null handler is intentionally absent only in the simple adapter: the
installed game normally registers `eGetWeaponID`.

These are CPU interaction results for the representative man rig. They do not
prove rendered Metal pixels, a particular player model, all action variants,
or an interactive in-game session. Damage values vary with random selection.

## Additional combat cases

The separate [additional replay](replay-extra.py) and
[fixture](replay-extra.cpp.inc) preserve the original results. Its
[raw output](replay-extra-raw.txt) and [input hashes](replay-extra-result.json)
record the distinct run; its historical runner is retained with its app path
redacted as [replay-extra-prior.py.txt](replay-extra-prior.py.txt). To reproduce it, set up a fresh temporary directory as
above and invoke `python3 docs/evidence/unarmed-boxing/replay-extra.py` with
`CODEX_SCRATCH_DIR` set to that directory. The same cache and installed-resource
requirements apply.

In the gameplay VM, a sword fast attack against a blocking unarmed defender
delivered one blocked event and reduced HP from 500 to 497; a blocking armed
defender retained 500 HP. Left and right dodge selected their respective pose
banks and each opened and closed the native recoil window (frames 9–34 at
16 ms per frame). This observes the window flag, not a timed enemy attack
against a moving character.

The installed dialogue strike handler was invoked at approximately 0.5 seconds
after starting the native `ambush_punch` action on a synthetic player with a
sword. The cross pose was selected, the handler reduced target HP from 500 to
470, and the native ragdoll became active. A direct trigger against a target
still in dialogue was rejected without creating a ragdoll. The fixture invokes
the handler directly: it does not verify dialogue option visibility, the
scheduled event transport, final weapon draw, or rendered gameplay.
