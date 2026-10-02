# Dev runtime and fast iteration

## Status

Candidate helper, October 1, 2026. Help, status, diff and a no-change push were
checked. Installed-app launch, edit consumption, live reload and startup timing
were not replayed. The supported player target is Corsairs Iddictive Remaster.app.

## Contract

Iterate on game content using the installed engine when native inputs are
unchanged, preserving player state and unrelated edits. A new patch wrapper or
native engine rebuild is unnecessary for an ordinary content-only change.

## Scope

### In scope
- Fast-launch game directly in dev mode without CMake, Ninja, engine recompilation, or 40-probe runs.
- Direct editing of game scripts (`PROGRAM/`), configuration (`RESOURCE/INI/`), and shaders (`RESOURCE/techniques/`).
- Local workspace directory (`gameplay/`) for editing and diffing without touching python patch files.
- Two-way synchronization between workspace and runtime (`push`, `pull`).
- A watcher (`watch`) that copies saved files to the cache and installed app.
- Unified diff viewer (`diff`) showing clean git diffs against the clean baseline.

### Out of scope
- Replacing the canonical release staging gate (`run.sh --stage-only`) for production packaging.
- Deprecated Windows or Wine runtimes.

## Owners

- `tools/dev_runtime.py` — Core dev runtime engine and CLI.
- `run-dev.sh` — Top-level fast launch shortcut.
- `experiments/native-metal/run.sh` — Supports `--dev` mode delegating to `dev_runtime.py`.
- The installed app's `Contents/Resources/public_launcher.py` launches its own
  `Contents/MacOS/metal-engine` from Application Support, with PROGRAM/RESOURCE
  linked to the bundle. Its saves/settings are independent of the cache.

## Current delivery limits

`dev_runtime.py launch` uses `.cache/runtime` and the binary inside
`.cache/CorsairsMetal.app`, including after `--build`. It does not launch the
installed remaster or install the rebuilt engine. `push` copies workspace files
to both destinations; that alone proves neither app launch nor live reload.

`gameplay/` is ignored scratch space, not a durable source owner. Pending edits
must be integrated into an owned source before claiming they survive a clean
checkout or formal staging. Formal staging may regenerate the same files.

The helper does not verify unknown destination edits, constrain whole-batch
rollback, or refresh the bundle signature. Those gaps remain unresolved; the
former unconditional development mandate is withdrawn.

## Commands

### 1. Fast Launch
```bash
./run-dev.sh
# or: ./experiments/native-metal/run.sh --dev
```
This is a cache launch command, not the player's app entry point. With an
existing cached binary it skips `build.sh` and first copies workspace files.
No launch-time measurement was made.

To rebuild the cached engine when native source changed:
```bash
./run-dev.sh --build
```
This command alone does not update the installed app. Native delivery still
requires the canonical `experiments/native-metal/run.sh --stage-only` and
verification of the app's installed binary/resources, followed by app replay.

### 2. Workspace Synchronization
```bash
# Pull current runtime modifications into workspace
python3 tools/dev_runtime.py pull

# Push workspace edits into active runtime and played app
python3 tools/dev_runtime.py push

# Check dev status
python3 tools/dev_runtime.py status
```

### 3. Unified Diff
```bash
# View diff for a specific file against clean baseline
python3 tools/dev_runtime.py diff GeneratorUtilite.c

# View all current gameplay differences
python3 tools/dev_runtime.py diff
```

### 4. Live Auto-Sync on Save
```bash
python3 tools/dev_runtime.py watch
```
Copies changed workspace files at the polling interval. Already loaded script
segments and cached renderer resources may retain their previous contents;
restart/reopen behavior must be verified for the changed consumer.

## Acceptance criteria

- Launch the installed app with its real player-state owner, consume an edited
  script/resource, replay the changed action and its nearest unaffected action.
- For content-only changes, verify the installed native engine is unchanged and
  no native build ran. For C++/embedded Metal changes, verify build and app delivery.
- Preserve SAVE/settings and unknown destination edits. Verify the installed
  bundle's signature after resource delivery.
- Establish reload behavior and measured timing before claiming either.
