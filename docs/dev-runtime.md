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

Canonical staging also admits the two exact UI texture destinations declared in
`tools/delivery_state.py`: mast repair and raking fire. Their sources and prepared
TX bytes are checksum-bound by `src/assets/ui/manifest.json` and use the existing
transaction/receipt path. Arbitrary texture paths and SAVE remain excluded.

gameplay/ remains ignored scratch space. Accepted edits need integration into
their durable source before canonical staging or clean-checkout delivery.

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
