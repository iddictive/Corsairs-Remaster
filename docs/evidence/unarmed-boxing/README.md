# Unarmed CPU combat replay

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
