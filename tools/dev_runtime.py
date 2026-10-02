#!/usr/bin/env python3
"""Dev Runtime: Fast iterative development workflow for Corsairs Metal.

Eliminates repetitive patch creation, sha256 recalculation, and engine
recompilation during gameplay scripting, shader tweaking, and content editing.

Commands:
  launch [--build] [--no-sync]  Fast-launch game without engine rebuild (< 0.5s)
  push [--quiet]                Sync editable workspace (gameplay/) to runtime
  pull [--all]                  Pull runtime modifications into gameplay/ workspace
  diff [path]                   Show unified diff against clean baseline
  status                        Show modified files and engine binary status
  watch [--interval SEC]        Hot-sync gameplay/ edits to runtime on save
  revert <path>                 Revert a file back to clean baseline
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Iterator

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / "experiments/native-metal"
CACHE = METAL / ".cache"
RUNTIME = CACHE / "runtime"
INPUTS = METAL / "inputs"
BASELINE = INPUTS / "gameplay"
WORKSPACE = PROJECT / "gameplay"
APP_BUNDLE = CACHE / "CorsairsMetal.app"
ENGINE_BINARY = APP_BUNDLE / "Contents/MacOS/metal-engine"
INSTALLED_APP = Path("/Applications/Corsairs Iddictive Remaster.app")
SETTINGS_LAUNCHER = PROJECT / "tools/metal_graphics_settings.py"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def is_game_running() -> bool:
    try:
        res = subprocess.run(["ps", "-axo", "comm="], capture_output=True, text=True, check=True)
        return any(line.rstrip().endswith(("/metal-engine", "/native-engine", "/engine-1", "engine.exe"))
                   for line in res.stdout.splitlines())
    except Exception:
        return False


def atomic_copy(src: Path, dst: Path) -> bool:
    """Copy src to dst atomically. Return True if destination was changed."""
    dst.parent.mkdir(parents=True, exist_ok=True)
    src_bytes = src.read_bytes()
    if dst.is_file():
        if file_sha256(dst) == sha256(src_bytes):
            return False
    tmp = dst.with_name(f".{dst.name}.tmp.{os.getpid()}")
    tmp.write_bytes(src_bytes)
    tmp.replace(dst)
    return True


def collect_relative_files(root: Path) -> list[Path]:
    if not root.is_dir():
        return []
    return sorted(p.relative_to(root) for p in root.rglob("*") if p.is_file() and not p.name.startswith("."))


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_push(quiet: bool = False) -> int:
    """Sync files from gameplay/ workspace into .cache/runtime/ and /Applications/."""
    if not WORKSPACE.is_dir():
        if not quiet:
            print(f"Workspace directory {WORKSPACE} does not exist yet. Run 'pull' first.")
        return 0

    count = 0
    rel_files = collect_relative_files(WORKSPACE)
    for rel in rel_files:
        src = WORKSPACE / rel
        dst = RUNTIME / rel
        if atomic_copy(src, dst):
            count += 1
            if not quiet:
                print(f"  [sync -> runtime] {rel}")

        # Also sync to installed /Applications app if present
        if INSTALLED_APP.is_dir():
            app_dst = INSTALLED_APP / "Contents/Resources" / rel
            if app_dst.parent.exists():
                if atomic_copy(src, app_dst):
                    if not quiet:
                        print(f"  [sync -> App]     {rel}")

    if not quiet:
        print(f"Dev sync: {count} file(s) updated across runtime targets.")
    return 0


def cmd_pull(all_files: bool = False) -> int:
    """Pull modified runtime files into gameplay/ workspace."""
    if not RUNTIME.is_dir():
        print(f"Error: runtime directory {RUNTIME} does not exist.", file=sys.stderr)
        return 1

    if not BASELINE.is_dir():
        print(f"Error: baseline directory {BASELINE} does not exist.", file=sys.stderr)
        return 1

    pulled = 0
    # Check PROGRAM and RESOURCE subdirectories
    for category in ("PROGRAM", "RESOURCE"):
        cat_runtime = RUNTIME / category
        cat_baseline = BASELINE / category
        if not cat_runtime.is_dir():
            continue

        for p in sorted(cat_runtime.rglob("*")):
            if not p.is_file() or p.name.startswith("."):
                continue
            rel = p.relative_to(RUNTIME)
            base_file = BASELINE / rel

            # Determine if modified compared to baseline
            is_modified = False
            if not base_file.is_file():
                # Only include new files in PROGRAM or INI/techniques in RESOURCE
                if category == "PROGRAM" or "INI" in rel.parts or "techniques" in rel.parts:
                    is_modified = True
            else:
                if file_sha256(p) != file_sha256(base_file):
                    is_modified = True

            if is_modified or all_files:
                target = WORKSPACE / rel
                if atomic_copy(p, target):
                    pulled += 1
                    print(f"  [pulled] {rel}")

    print(f"Dev pull complete: {pulled} file(s) updated in {WORKSPACE.relative_to(PROJECT)}/")
    return 0


def cmd_status() -> int:
    """Show current dev runtime status."""
    print("=== Corsairs Metal Dev Runtime Status ===")
    
    # Engine status
    built_engine = CACHE / "build/bin/engine-1"
    if ENGINE_BINARY.is_file():
        mtime = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(ENGINE_BINARY.stat().st_mtime))
        print(f"Engine binary: READY ({ENGINE_BINARY.name}, modified {mtime})")
    elif built_engine.is_file():
        print("Engine binary: Built in build/bin/engine-1, ready to stage.")
    else:
        print("Engine binary: NOT BUILT (will build on first launch).")

    # Workspace status
    if not WORKSPACE.is_dir():
        print(f"Workspace: {WORKSPACE.relative_to(PROJECT)}/ does not exist. (Run 'pull' to create)")
    else:
        ws_files = collect_relative_files(WORKSPACE)
        print(f"Workspace files ({WORKSPACE.relative_to(PROJECT)}/): {len(ws_files)} file(s)")

    # Runtime modifications vs baseline
    if RUNTIME.is_dir() and BASELINE.is_dir():
        modified_in_runtime = []
        for category in ("PROGRAM", "RESOURCE"):
            cat_runtime = RUNTIME / category
            if not cat_runtime.is_dir():
                continue
            for p in cat_runtime.rglob("*"):
                if not p.is_file() or p.name.startswith("."):
                    continue
                rel = p.relative_to(RUNTIME)
                base = BASELINE / rel
                if base.is_file() and file_sha256(p) != file_sha256(base):
                    modified_in_runtime.append(rel)

        print(f"Runtime modified vs baseline: {len(modified_in_runtime)} file(s)")

        # Un-synced workspace changes
        if WORKSPACE.is_dir():
            unsynced = []
            for rel in collect_relative_files(WORKSPACE):
                src = WORKSPACE / rel
                dst = RUNTIME / rel
                if not dst.is_file() or file_sha256(src) != file_sha256(dst):
                    unsynced.append(rel)
            if unsynced:
                print(f"Pending changes in workspace (not yet pushed): {len(unsynced)} file(s)")
                for item in unsynced[:5]:
                    print(f"  * {item}")
                if len(unsynced) > 5:
                    print(f"  ... and {len(unsynced) - 5} more")

    return 0


def cmd_diff(target_path: str | None = None) -> int:
    """Show unified diff of workspace or runtime against baseline."""
    if not BASELINE.is_dir():
        print(f"Error: baseline {BASELINE} missing.", file=sys.stderr)
        return 1

    source_root = WORKSPACE if WORKSPACE.is_dir() else RUNTIME
    source_label = "workspace" if WORKSPACE.is_dir() else "runtime"

    rel_candidates = collect_relative_files(source_root)
    if target_path:
        rel_candidates = [p for p in rel_candidates if target_path in str(p)]

    diff_found = False
    for rel in rel_candidates:
        src = source_root / rel
        base = BASELINE / rel
        if not base.is_file():
            continue
        src_lines = src.read_text(errors="replace").splitlines(keepends=True)
        base_lines = base.read_text(errors="replace").splitlines(keepends=True)
        if src_lines == base_lines:
            continue

        diff_found = True
        diff = difflib.unified_diff(
            base_lines,
            src_lines,
            fromfile=f"a/{rel} (baseline)",
            tofile=f"b/{rel} ({source_label})",
        )
        sys.stdout.writelines(diff)

    if not diff_found and target_path:
        print(f"No differences found for '{target_path}'")
    return 0


def cmd_watch(interval: float = 0.5) -> int:
    """Watch gameplay/ directory and sync changes live to runtime."""
    if not WORKSPACE.is_dir():
        print(f"Creating {WORKSPACE} before starting watch...")
        cmd_pull()

    print(f"Watching {WORKSPACE.relative_to(PROJECT)}/ for changes (interval={interval}s)...")
    print("Edit your .c, .h, and .ini files directly. Press Ctrl+C to stop.")

    mtimes: dict[Path, float] = {}

    def scan():
        for rel in collect_relative_files(WORKSPACE):
            p = WORKSPACE / rel
            try:
                mt = p.stat().st_mtime
                if rel not in mtimes or mtimes[rel] != mt:
                    mtimes[rel] = mt
                    dst = RUNTIME / rel
                    if atomic_copy(p, dst):
                        ts = time.strftime("%H:%M:%S")
                        print(f"[{ts}] Live-synced: {rel}")
                        if INSTALLED_APP.is_dir():
                            app_dst = INSTALLED_APP / "Contents/Resources" / rel
                            if app_dst.parent.exists():
                                atomic_copy(p, app_dst)
            except OSError:
                pass

    # Initial scan
    for rel in collect_relative_files(WORKSPACE):
        mtimes[rel] = (WORKSPACE / rel).stat().st_mtime

    try:
        while True:
            time.sleep(interval)
            scan()
    except KeyboardInterrupt:
        print("\nWatch stopped.")
    return 0


def cmd_launch(rebuild: bool = False, no_sync: bool = False, bg: bool = False, passthrough: list[str] | None = None) -> int:
    """Launch the game in dev mode (< 0.5s startup)."""
    if is_game_running():
        print("Warning: Game is already running. Close it before launching.", file=sys.stderr)
        return 1

    # 1. Sync workspace if present
    if not no_sync and WORKSPACE.is_dir():
        cmd_push(quiet=True)

    # 2. Engine check / build
    need_build = rebuild or not ENGINE_BINARY.is_file()
    if need_build:
        print("Building Metal engine...")
        res = subprocess.run([str(METAL / "build.sh")])
        if res.returncode != 0:
            print("Engine build failed.", file=sys.stderr)
            return res.returncode
        # Copy newly built engine to bundle
        built_engine = CACHE / "build/bin/engine-1"
        if built_engine.is_file():
            ENGINE_BINARY.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(built_engine, ENGINE_BINARY)
            os.chmod(ENGINE_BINARY, 0o755)

    if not ENGINE_BINARY.is_file():
        print(f"Error: Engine binary {ENGINE_BINARY} not found.", file=sys.stderr)
        return 1

   # 3. Fast launch via metal_graphics_settings.py
    print(f"Starting Metal candidate in dev mode (< 0.5s launch)...\nLog: {CACHE / 'launch.log'}")
   
   # Environment variables
    env = dict(os.environ)
    env["STORM_USERDATA"] = str(CACHE / "userdata")
    env["STORM_METAL_MODERN_EFFECTS"] = os.environ.get("STORM_METAL_MODERN_EFFECTS", "1")
    env["STORM_METAL_ISLAND_GEOMETRY"] = os.environ.get("STORM_METAL_ISLAND_GEOMETRY", "0")
    env["STORM_METAL_PROFILE"] = os.environ.get("STORM_METAL_PROFILE", "1")
    env["STORM_METAL_ROCKK2"] = os.environ.get("STORM_METAL_ROCKK2", "0")
    env["STORM_TRACE_DECK_WALK"] = os.environ.get("STORM_TRACE_DECK_WALK", "1")

    # Dependency root for SDL2
    dep_root = INPUTS / "native"
    if not dep_root.is_dir():
        dep_root = PROJECT / "experiments/native-storm/.cache"
    env["DYLD_LIBRARY_PATH"] = str(dep_root / "d3d9/sdl2-fixed/lib")

    for k in ("DXVK_CONFIG_FILE", "DXVK_WSI_DRIVER", "DXVK_LOG_LEVEL", "MVK_CONFIG_LOG_LEVEL"):
        env.pop(k, None)

    # Launch binary directly in runtime directory
    os.chdir(RUNTIME)
    log_file = CACHE / "launch.log"
    cmd = [
        sys.executable,
        str(SETTINGS_LAUNCHER),
        "launch",
        str(RUNTIME),
        "--binary",
        str(ENGINE_BINARY),
    ]
    if bg:
        with open(log_file, "w") as log_out:
            p = subprocess.Popen(cmd, stdout=log_out, stderr=subprocess.STDOUT, env=env)
            print(f"Metal engine running in background (PID: {p.pid}).")
        return 0
    else:
        with open(log_file, "w") as log_out:
            res = subprocess.run(cmd, stdout=log_out, stderr=subprocess.STDOUT, env=env)
            return res.returncode


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(dest="command", help="Dev runtime commands")

    # launch
    p_launch = subparsers.add_parser("launch", help="Fast-launch game without engine rebuild")
    p_launch.add_argument("--build", "-b", action="store_true", help="Force engine rebuild")
    p_launch.add_argument("--no-sync", action="store_true", help="Skip syncing gameplay/ to runtime")
    p_launch.add_argument("--bg", action="store_true", help="Launch in background")

    # push
    p_push = subparsers.add_parser("push", help="Sync gameplay/ workspace to runtime")
    p_push.add_argument("--quiet", "-q", action="store_true", help="Quiet output")

    # pull
    p_pull = subparsers.add_parser("pull", help="Pull modified runtime files into gameplay/ workspace")
    p_pull.add_argument("--all", "-a", action="store_true", help="Pull all runtime files, not just modified")

    # status
    subparsers.add_parser("status", help="Show dev runtime status and modified files")

    # diff
    p_diff = subparsers.add_parser("diff", help="Show unified diff against baseline")
    p_diff.add_argument("path", nargs="?", help="Specific file path to diff")

    # watch
    p_watch = subparsers.add_parser("watch", help="Watch gameplay/ and live-sync changes")
    p_watch.add_argument("--interval", "-i", type=float, default=0.5, help="Polling interval in seconds")

    # revert
    p_revert = subparsers.add_parser("revert", help="Revert a file back to clean baseline")
    p_revert.add_argument("path", help="Relative file path to revert (e.g. PROGRAM/dialog.c)")

    args, unknown = parser.parse_known_args()

    if not args.command:
        # Default to launch if called without command
        return cmd_launch(rebuild=False)

    if args.command == "launch":
        return cmd_launch(rebuild=args.build, no_sync=args.no_sync, bg=args.bg, passthrough=unknown)
    elif args.command == "push":
        return cmd_push(quiet=args.quiet)
    elif args.command == "pull":
        return cmd_pull(all_files=args.all)
    elif args.command == "status":
        return cmd_status()
    elif args.command == "diff":
        return cmd_diff(args.path)
    elif args.command == "watch":
        return cmd_watch(args.interval)
    elif args.command == "revert":
        rel = Path(args.path)
        base_file = BASELINE / rel
        if not base_file.is_file():
            print(f"Error: baseline file not found: {base_file}", file=sys.stderr)
            return 1
        if WORKSPACE.is_dir() and (WORKSPACE / rel).is_file():
            atomic_copy(base_file, WORKSPACE / rel)
        atomic_copy(base_file, RUNTIME / rel)
        print(f"Reverted {rel} to baseline.")
        return 0

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
