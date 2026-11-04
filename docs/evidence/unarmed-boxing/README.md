# Unarmed animation and combat evidence

## Recorded boxing and native strike timing, 2026-11-04

The subsequent facing repair keeps every recorded body/arm/root sample intact
and aims the native face at the opponent through the actual neck and head.
The previous guard faced about 57° sideways. [Facing measurements and the
full-body comparison](facing/measurements.json) cover all 24 rigs; the
[comparison image](facing/guard-full-body.png) shows man and Jessika.
Reproduce with `python3 docs/evidence/unarmed-boxing/facing/replay.py /caller-owned-output`.
The body-motion sheets and native measurements below precede this gaze-only
correction; they continue to describe the unchanged body and strike timing.

The earlier procedural candidate in `fb4ae910` was rejected in gameplay for
body rotation, slow attacks and insufficient extension. Its historical
[pose measurements](pose-metrics.json) remain evidence of that candidate,
not acceptance of the current motion.

`tools/prepare_unarmed.py` now consumes the retained
[ACCAD motion sources and attribution](../../../src/assets/unarmed/mocap/ATTRIBUTION.md)
through `tools/boxing_mocap.py`. Jab and cross use recorded left/right strikes;
the uppercut mirrors the recorded left uppercut into the same orthodox stance.
The source recipe owns trims, contact mapping and guard transitions. Native
finger articulation supplies the fist shape because the capture has no fingers.
The [close hand views](hands.png) and [digit-only views](fingers-only.png)
separate folded fingers from the wrist mesh. Detailed thumbs oppose the folded
index through the existing native-rig IK helper; applying a generic finger curl
to the thumb left a hook. Their one-off reproduction inputs are
[the hand inspection](inspect-hand.py.txt) and [digit filter](inspect-digits.py.txt);
replace `${OUTPUT}` with a caller-owned output directory when replaying them.
Idle, block and dodges derive from the same recorded guard; every action has
exact common boundary poses. [Retarget evidence](mocap/README.md) records the
source/native trajectories and why the uppercut contact precedes its high
follow-through. Those intermediate sheets use unmodified bind fingers; the
integrated sheets below include the native hand layer.

The [front](rigs-front.png), [quarter](rigs-quarter.png) and [side](rigs-side.png)
sheets show all seven actions on six real GM meshes: Danielle, Jessika,
YokoDias, man, man1 and man2. The [full sample sheet](all-frames.png) shows every
man cross/uppercut sample with one fixed upper-body camera fitted to the whole
sequence. The [phase sheet](boxing-pose-atlas.png) and
[motion GIF](cross-uppercut-block.gif) show the representative man mesh.
These are offline depth-tested two-weight renders, not game captures. The GIF
uses authored strike durations; static sheets cannot prove played motion.

[Current native measurements](pose-metrics-current.json) use the real Release
`AnimationImp::EvaluatePose`, including Bone compression and quarter-frame
interpolation, over all 24 rigs. The baseline is the installed rejected
`fb4ae910` bank set. The 130-frame layout, bone counts and AN byte count stay
compatible. Recorded weight transfer moves ankles: maximum displacement from
initial guard is 0.244 m; this metric is not an assertion that the feet slide.
Captured ankle displacement, native leg proportions and sole support own the
retarget. No additional retargeting or IK runs in gameplay.

The native combat clock previously used the sword clip duration, and
`Character::Move` restored its default speed every frame. Fists now use the
bank's authored whole-action durations: jab 0.32 s, cross 0.42 s, uppercut
0.56 s, multiplied by the existing character speed modifier. The base action
retains its single damage event and existing contact-phase remapping. Sword
rates reset normally. No new per-frame animation sampling or physics work is
added to boxing; the existing speed assignment selects the appropriate rate.
Full-scene FPS and the player's visual acceptance remain outside CPU evidence.

The current [clock replay](clock-result.json) and [raw output](clock-raw.txt)
exercise real `Character::Move` with candidate banks, twelve existing combat
cases and a sword-rate check after each fist. The retained
[one-off runner](clock-replay.py.txt) and [native fixture](clock-fixture.cpp.txt)
bind the exact checks; old historical replay results below remain unchanged.

Reproduce asset and native pose preparation with Python 3.10+ and the project's
Pillow/NumPy environment:

```sh
python3 tools/prepare_unarmed.py
python3 tools/render_unarmed_atlas.py --rig all --angle 90 --output /task-owned-output/rigs-side.png
python3 tools/render_unarmed_atlas.py --motion --action unarmed_cross --action unarmed_uppercut --action unarmed_block --output /task-owned-output/motion.gif
python3 docs/evidence/unarmed-boxing/inspect-poses.py --baseline /reconstructed-fb4-bank-root --candidate experiments/native-metal/.cache/unarmed --scratch /task-owned-scratch --output /task-owned-output/pose-metrics.json
```

The native sampler compiles only a disposable CPU adapter against the current
cached animation objects. It does not start the game or read player saves.
Normal staging and installation own final runtime delivery; no receipt is
edited by hand.

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
