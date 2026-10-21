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

Engine-linked fixture handoff uses `tools/capture_metal_build.py --repo <built-checkout>
--max-bytes <admitted-copy-budget>
--retain-for <specific-consumer-property> --release-after <observable-condition>
--expires <YYYY-MM-DD>`.
Ordinary checks reuse `.cache/build`; committed source/docs are referenced by
Git revision/path, not copied into dated archives. Use a frozen capture only
when that consumer needs exact binary/ABI state across a later cache rebuild.
There is one persistent slot:
`experiments/native-metal/.cache/diagnostics/retained-native-build`.
An explicit `--output` may select only that slot or a child of the bounded
producer's `CODEX_SCRATCH_DIR` outside the checkout; dated archive/output aliases
are rejected before capture. Resolve the slot's recorded consumer/release condition before replacing
a different build. Temporary comparisons use the existing bounded scratch runner.
It checks
the canonical source/engine receipt and rejects inputs/artifacts modified after
that completed build, then retains
all static libraries, native headers (including `.rh` and third-party headers),
build logs/cache and exact fixture-producer commands with hashes. It also retains
`compile-commands.json` from Ninja's `compdb-targets`: bind a fixture that reads a
native class to that class owner's production translation unit. A linked
benchmark's flags may omit library-private definitions that change class layout.
The default producer is `sea-fleet-benchmark`; capture builds or runs no target.
The destination must be new; repeating a complete identical
capture verifies and reuses it, while a mismatch preserves it and fails.
Before copying to a new name, it checks capture manifests under `.local-archives`,
cache diagnostics and the destination parent. Matching native binary/library,
header and vendor hashes reject the duplicate even for a different target:
reuse those artifacts and retain only distinct command metadata. Existing
captures are preserved; nothing is pruned automatically. New manifests record
the consumer, finite review date and release condition; an already-past date
rejects admission. Expiry requires owner review, never automatic deletion of
unresolved evidence. Captured
native artifacts use lossless filesystem compression at creation; hashes and
normal file reads are unchanged. Archive admission shares the storage owner's
8GiB allocated-byte budget instead of permitting an unlimited sequence of builds.
Portable
dependency contents remain in their manifest-bound input owner. This command
does not stage the application or accept gameplay.

`tools/metal_storage.py` owns producer storage policy. Ordinary builds retain the
single `.cache/build` and run trace-budget maintenance before and after the build;
successful engine installation retains one newest prior-engine hash backup.
The current build has an8GiB logical-size preflight ceiling and remains an active
build consumer without age expiry; exceeding it fails for review without purge or
automatic rebuild.
Only regular `metal-engine.<64 lowercase hex>` backup files are eligible;
unknown directories and resource/source backups remain untouched. Diagnostics
have an8GiB allocated-byte aggregate budget: maintenance removes only oldest generated `.trace`
and `.gputrace` bundles, preserving scripts, logs, receipts, raw geometry,
lifetime-locked runs, the newest trace, exact still-live producer PID identities and cache-relative paths listed in the ignored
`.cache/storage-pins.json` (`{"pins":["diagnostics/..."]}`). If protected evidence
still exceeds the budget, the producer fails with an explicit retained-evidence
error. Output checks require an8GiB free-space reserve on the destination volume.
The sea benchmark additionally owns its process sessions and private per-run
`TMPDIR`, monitors a3GiB run budget without following RESOURCE symlinks, and stops
only its own sessions on a cap/deadline. Its benchmark deadline remains90s,
Metal trace35s and CPU sample55s. Tool scratch is removed after those sessions
exit; measurements and concise failure logs remain evidence. These are sampled
limits, so an individual write can overshoot before the next monitor poll.
SIGTERM, Ctrl+C and ordinary failures pass through the same owned-session cleanup;
SIGKILL cannot run cleanup, so the PID identity receipt protects a surviving child
from later trace pruning until it exits.
Exporter output and packaging scratch share the same nested SIGTERM cleanup
scope. Diagnostic session leaders remain unreaped until their groups stop, which
preserves group identity after leader exit and prevents signaling a reused group.

### Storage convention

These are different owners, not three release installations:

| Path | Retained purpose |
| --- | --- |
| `/Applications/Corsairs Iddictive Remaster.app` | Sole player engine and complete game resources |
| `experiments/native-metal/engine`, adjacent renderer files and `inputs/` | Editable source and manifest-bound rebuild dependencies |
| `experiments/native-metal/.cache/` | One reusable build; disposable staging resources are restored as APFS clones only when needed |
| `.local-archives/` | Remaining unique player state, original captures and irreproducible inputs; source/history lives in Git, with no new build or source-copy catalogue |
| `dist/` | Disposable packaging work; a ZIP exists only for an explicit handoff |

Do not move source or unique evidence into a new folder to disguise disk usage.
Recovered archive sources and their path indexes are retained in the local Git
branch `codex/recovered-archive-sources-20261020`; inspect them with
`git show codex/recovered-archive-sources-20261020:recovery-index.json`.
`orphan-source-index.json`, `recovery-symlinks.json`,
`container-reconstruction.json` and `remaining-aliases.json` on that branch
resolve older inputs, links, containers and deduplicated evidence paths.
Imported history is anchored under `refs/recovery/private-history/*` and
`refs/recovery/storm-engine/*`. These recovery refs are local and include private
historical context; they are not part of the public remote delivery.
Public source recovery uses branch `codex/recovered-sources` and its
`recovered-sources/index.json`: unique historical code and build inputs, with
machine-specific paths normalized. Its ancestry contains only the public
mainline, never the private recovery history or agent notes.

Use the common owner to inspect storage or losslessly compact cold evidence:

```sh
python3 tools/metal_storage.py --report
python3 tools/metal_storage.py --compact-evidence --exclude <active-evidence-directory>
```

After the final staging/export/probe consumer finishes, reclaim the complete
staging copy through the same owner:

```sh
python3 tools/metal_storage.py --reclaim-runtime \
  --state-output .local-archives/<current-evidence>/runtime-state
```

It rejects active build/stage/export or open-file consumers, nonempty development
SAVE and missing rebuild inputs. Before removal it checks every PROGRAM/RESOURCE
file against the app. Differences and cache-only files are preserved and hashed
in the new state directory with staging config, receipts and logs; the app is
never overwritten with them. Source, build, inputs,
installed app and the played profile stay in place. The next ordinary
`run.sh --stage-only` calls `restore_metal_runtime.py` through its existing
missing-runtime path. It reconstructs the app's canonical content; retained cache
differences remain recovery evidence and are never adopted silently.
Content-only tools and exporters need that staging workspace
restored before use. Prior-state reclaim/restore, divergent-file preservation
and nonempty-SAVE refusal pass in disposable fixtures. No live player launch is needed for cache
cleanup, and cache reclamation does not accept gameplay.

Compaction uses the installed `/usr/bin/ditto --noclone --hfsCompression` and
checks unchanged SHA-256, mode, timestamp and source identity before replacement.
Default APFS cloning alone preserves uncompressed extents, so the codec requires
`--noclone`. The local native probe checks both compressible and incompressible
content. Cold files must be at least256KiB and unchanged for24hours; linked files,
Git administration, dirty archived worktrees, explicit exclusions, open files
and lifetime-locked runs are preserved. Paths and readable content remain intact;
there is no archive extraction prerequisite. This applies only to retained
evidence, never installed game resources, editable source, SAVE or active builds.

Allocated-byte totals conservatively count shared APFS extents more than once;
they are not an exclusive-usage measurement. `volume_free_bytes` records actual
volume free space. Packaging rules and the single local ZIP slot belong to
`metal-remaster-delivery.md`; a completed distribution does not justify keeping
dated local copies of its ZIP or extracted app.

October19 cleanup evidence is retained under
`.local-archives/storage-maintenance-20261019`: before/after inventories,
per-file compaction hashes, the removed ZIP's receipt and retained runtime state.
The first completed reclamation verified44,118 assets, preserved the two distinct
MainHero dialogue candidates and removed22,541,904,306 logical staging bytes.
Across2,834 cold files, filesystem compression removed6,212,820,992 conservatively
counted allocated bytes. The obsolete ZIP was10,873,247,949 bytes. These are
storage results; installed-game behavior is unchanged by this cleanup lane.

Engine changes are ordinary edits in `experiments/native-metal/engine`; Metal
changes remain in adjacent renderer files. The normal build no longer rewrites
cached source or replays patches. Compiler readiness fingerprints actual native
inputs and detects edits after the last build; old patch hashes are historical.

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

Authored map patrol/sinking and quiet-client native sources participate in the
same compiler receipt. Their changed hashes keep content apply unready until
the canonical engine build/stage consumes them; script compilation alone does
not provide the new native attribute bridge. Ordinary installed startup defaults
to `STORM_QUIET_CLIENT=1` through the public launcher. Explicit diagnostic
launches may set it to0; candidate/headless probes retain their diagnostic owner.
The flag disables existing-player runtime logging/profiling without changing
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

The Puerto Rico GM/PTC city projection uses canonical `run.sh --stage-only`,
not the text-only dev push. Its recipe and finite resource manifest live under
`src/assets/islands/puerto-rico`; canonical staging prepares the geometry before
content delivery. It requires the native SeaTown context plus `sea.c` activation.
Fort/locator/seabed hashes are checked, player state is outside the transaction.
Current candidate/replay status belongs to `docs/puerto-rico-city.md`.

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
