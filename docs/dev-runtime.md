# Dev runtime and content iteration

## Status

Helper source corrected October 7, 2026. Disposable fixtures exercise installed
launch routing, prior snapshots, conflicts, rollback and the clean upgrade path.
The installed native launcher's dry-run resolves the played app engine and its
bundled state/settings owner. A one-file installed content canary passed push,
reseal/strict verification and exact original restoration through revert; engine
and all 233 preexisting SAVE/config hashes were unchanged. Source/route checks
do not accept scene startup, live reload or timing.

## Contract and owners

The sole target is /Applications/Corsairs Iddictive Remaster.app.
run-dev.sh delegates to tools/dev_runtime.py. Launch uses the installed
Contents/MacOS/launch entry point, which invokes its bundled public_launcher.py.
That launcher owns Application Support/Iddictive Corsairs, SAVE, settings, logs,
PROGRAM/RESOURCE links and the engine environment. The helper does not launch
the development cache or assign another player-state directory.

Editable existing files are limited to PROGRAM .c/.h, RESOURCE/INI .ini and
RESOURCE/techniques .fx. SAVE, userdata, root configuration, shared message
headers, artwork and native binaries are outside content synchronization.

Native changes use `experiments/native-metal/build.sh`; its default is two
compiler workers so an interactive game retains CPU capacity. The existing
CMake/Ninja parallelism owner accepts `CORSAIRS_METAL_BUILD_JOBS` as a positive
integer override (for example1 for the lowest concurrent build load). Invalid
values fail before source preparation or configuration. This controls compilation,
not the game's simulation or renderer; build success does not activate the app.

Canonical staging also admits the seven exact UI texture destinations declared in
`tools/delivery_state.py`: mast repair, fire mode, raking fire, heated shot, goods, command and navigator icons.
The three atlases preserve existing pixels outside the airburst cells. Their sources and prepared
TX bytes are checksum-bound by `src/assets/ui/manifest.json` and use the existing
transaction/receipt path. An existing unrecorded atlas is admitted only by the
manifest's exact `original_sha256`; subsequent changes use the receipt.
Arbitrary texture paths and SAVE remain excluded.

Phonk Mode additionally admits the exact music destination
`RESOURCE/Sounds/MUSIC/phonk-drive.mp3`. Its checksum/download recipe belongs to
`src/assets/audio/manifest.json`; canonical staging prepares the official input
through `tools/prepare_phonk_music.py` before gameplay sync. Existing verified
bytes require no network access. The recording is a player-local input, excluded
from Git; its public redistribution/licensing boundary and feature lifecycle
belong to `sea-phonk.md`.

The Great Recipe's three additive item locator GMs are admitted at the exact
Hold/Deck2/Camp_Deck destinations in `tools/delivery_state.py`. Their manifest
in `src/assets/quests/great-recipe` binds the stock dummy mesh and prepared bytes;
`prepare.py` reconstructs them from the retained template and exact stock locator
matrices in `layout.json`. They add native `item` locators only to cloned quest
rooms through `models.always.greatRecipeItems`; no stock location GM is replaced.

Engine staging delivers the built `shared/messages.h` and
`shared/sea_ai/script_defines.h` to both runtime and installed script consumers.
Their finite bootstrap hashes and delivery paths belong to
`LEGACY_SHARED_HEADERS` in `tools/delivery_state.py`; later upgrades use receipts.
Startup compilation must be checked against the installed headers. A probe using
the source tree's newer headers can pass while the played app rejects new event
constants in `AICannon.c`.

`tools/sync_metal_gameplay.py check` compares both the cache and the installed
app, including receipt integrity. A zero cache delta alone does not establish
installed parity. Its compiler-ready flag checks the engine revision, not script
compilation; startup compatibility requires the complete installed `seadogs.c`
include chain rather than selected function fixtures.

Authored map patrol/sinking and quiet-client native patches participate in the
same compiler receipt. Their changed hashes keep content apply unready until
the canonical engine build/stage consumes them; script compilation alone does
not provide the new native attribute bridge. Ordinary installed startup defaults
to `STORM_QUIET_CLIENT=1` through the public launcher. Explicit diagnostic
launches may set it to0; candidate/headless probes retain their diagnostic owner.
The flag activates existing-player runtime logging/profiling without changing
their persistent engine.ini or other settings. Quiet mode avoids diagnostic file
sinks, trace formatting and script call/event timers, and forces frame profiling/
deck tracing off. Gameplay Log_Info notifications and fatal stderr remain.

gameplay/ remains ignored scratch space. Accepted edits need integration into
their durable source before canonical staging or clean-checkout delivery.

## Codex environment actions

The saved Corsairs project's local `.codex/environments/environment.toml`
exposes the installed-game actions and a read-only diagnostic capture:

| Action | Effect |
| --- | --- |
| Обновить установленную игру | Runs `experiments/native-metal/run.sh --stage-only`: builds the current engine, prepares reviewed resources and updates the played app in `/Applications`. The game must be closed; this action does not launch it. |
| Открыть установленную игру | Opens `/Applications/Corsairs Iddictive Remaster.app` through `/usr/bin/open`. It does not build or update the game and uses the installed launcher's existing player-state owner. |
| Записать профиль FPS (через 5 секунд) | Runs `sh tools/profile-game.sh`: waits five seconds, then records ten seconds of the installed engine's CPU stacks without game input or settings changes. |
| Проверить проект | Runs the existing `python3 tools/agent_context.py --check` repository consistency check. It does not build or launch the game. |

`sh tools/profile-game.sh` also works from the repository terminal. Each capture
uses a separate timestamped file under the ignored
`experiments/native-metal/.cache/diagnostics` directory. Leave the scene being
measured open for the whole capture. CPU samples identify active work and waits;
they do not measure GPU time or replace the game's visible FPS counter.

Setup checks the existing development tools only; it builds neither engine.
The internal word `stage` names file preparation, not a separate player
environment. `run.sh --launch-installed` launches the development cache despite
its name and is therefore not the player-facing environment action.

The configuration is local-only. Inspect the visible Codex menu after it has
reloaded the environment before claiming its labels are current; disk parsing
and launch-route inspection do not prove menu refresh.

## Ordinary content iteration

Open the installed app once to initialize its player state, then close it.

~~~sh
python3 tools/dev_runtime.py pull
# Edit a pulled file in gameplay/ using a normal editor.
python3 tools/dev_runtime.py diff PROGRAM/path/to/file.c
python3 tools/dev_runtime.py push
./run-dev.sh
~~~

pull reads installed resources. Its default includes existing workspace files
and editable installed files differing from the input baseline; --all includes
all editable installed files. It records their original installed bytes in
experiments/native-metal/.cache/dev-runtime/originals and binds original/applied
hashes to this app in state.json. The input baseline only selects files for pull;
it is never the revert source.

A pre-existing unsnapshotted workspace file that differs from the app is
preserved and refused. Reconcile that exact file before pull; the helper does
not adopt an old cache snapshot as the installed original. Pending workspace
edits also prevent pull from overwriting them.

push, watch and revert share one transaction: validate the complete finite batch,
reject linked paths and unknown installed edits, compare again immediately
before each write, write atomically, then reseal and strictly verify the app.
Failure restores only this transaction's still-matching bytes and state, then
reseals/verifies the restored app. A concurrent foreign edit is preserved and
reported as incomplete rollback. Original snapshot blobs remain available.

Content mutation holds both the dev-helper lock and the installed launcher's
existing player-state .launch.lock. It requires the game to be closed. No engine
build runs for content-only delivery; SAVE/settings remain outside the write set.

~~~sh
python3 tools/dev_runtime.py watch --interval 0.5
python3 tools/dev_runtime.py revert PROGRAM/path/to/file.c
python3 tools/dev_runtime.py status
~~~

watch uses the same push transaction and waits while the game owns the launch
lock. Ctrl+C stops it; no background service is installed. revert restores that
file's original dev snapshot in both workspace and app, preserving unrelated
paths. Copying content does not establish live script/shader reload: restart or
reopen the changed consumer and replay its action.

## Native changes and ordinary upgrades

~~~sh
./run-dev.sh --build
~~~

Before syncing or building, --build rejects pending workspace edits and any
previously applied dev edits whose applied hash differs from their original.
Integrate those edits into the durable source, or revert them explicitly first.
--no-sync does not bypass this gate. Refusal runs no canonical staging or app
content mutation; it never silently regenerates dev-edited files.

For a clean workspace, the helper snapshots the prior installed content, runs
experiments/native-metal/run.sh --stage-only, refreshes the clean workspace from
the installed result, then launches the installed app. A failed stage prevents
launch. This is the existing canonical engine/resource delivery path, including
its signature and unknown-edit checks.

After another canonical delivery, pull refreshes a clean previously snapshotted
workspace to the new installed original. It preserves pending workspace edits
and refuses an installed upgrade conflicting with active dev edits. Prior
original blobs remain retained; the current snapshot references the new original.

~~~sh
./run-dev.sh --no-sync
./run-dev.sh --bg
~~~

--no-sync launches only the installed entry point without content delivery.
--bg returns its launcher PID. No run.sh --dev mode exists.

## Acceptance remaining

Installed signature, bounded content push/revert, engine/state preservation and
native launcher routing are verified. The clean --build ordering and its failed
stage/no-launch negative pass in disposable fixtures; canonical staging installs
the current engine separately. Actual edited-script/resource consumption and
its nearest unaffected action remain player scene replay. Measure startup and
reload behavior before making timing or hot-reload claims.
