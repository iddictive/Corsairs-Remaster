# Dev runtime and fast iteration

## Status

Active dev workflow for Corsairs Metal (October 2026).

## Contract

Provide a zero-rebuild fast launch path (< 0.5s) and direct script/shader editing
workflow without generating python string-replacement patches or recalculating sha256 hashes.

## Scope

### In scope
- Fast-launch game directly in dev mode without CMake, Ninja, engine recompilation, or 40-probe runs.
- Direct editing of game scripts (`PROGRAM/`), configuration (`RESOURCE/INI/`), and shaders (`RESOURCE/techniques/`).
- Local workspace directory (`gameplay/`) for editing and diffing without touching python patch files.
- Two-way synchronization between workspace and runtime (`push`, `pull`).
- Real-time watcher (`watch`) that auto-syncs saved changes into the active runtime and played app.
- Unified diff viewer (`diff`) showing clean git diffs against the clean baseline.

### Out of scope
- Replacing the canonical release staging gate (`run.sh --stage-only`) for production packaging.
- Deprecated Windows or Wine runtimes.

## Owners

- `tools/dev_runtime.py` — Core dev runtime engine and CLI.
- `run-dev.sh` — Top-level fast launch shortcut.
- `experiments/native-metal/run.sh` — Supports `--dev` mode delegating to `dev_runtime.py`.

## Commands

### 1. Fast Launch
```bash
./run-dev.sh
# or: ./experiments/native-metal/run.sh --dev
```
Launches the Metal engine instantly (< 0.5s). Automatically syncs any edited files
from `gameplay/` into the runtime before starting. Skips engine compilation,
probes, hash checks, and code-signing.

To force an engine rebuild when C++ source was actually modified:
```bash
./run-dev.sh --build
```

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
Monitors `gameplay/` and syncs changes to the running game in milliseconds upon file save.

## Acceptance criteria

- Running `./run-dev.sh` starts the game in under 1 second when the engine binary is present.
- Gameplay scripts (`.c`), INI files, and shaders can be edited directly without touching python patch scripts or computing sha256 checksums.
- The canonical production pipeline (`run.sh --stage-only`) remains intact for release builds.

