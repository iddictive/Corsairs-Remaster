#!/usr/bin/env python3
"""Edit installed Corsairs content without rebuilding its native engine.

Pull before editing gameplay/. Push/watch/revert preserve original installed
bytes and reject unknown destination edits. --build uses canonical stage-only
and refuses active dev edits before any content sync or build.
"""
from __future__ import annotations

import argparse
import difflib
import fcntl
import hashlib
import json
import os
import subprocess
import sys
import time
from contextlib import contextmanager
from pathlib import Path

from runtime_script_patch import atomic_write
from delivery_state import DeliveryState, RECEIPT, safe_path, require_idle
from delivery_state import player_guard as delivery_player_guard, transact as deliver
import gameplay_sources

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / "experiments/native-metal"
BASELINE = METAL / "inputs/gameplay"
WORKSPACE = PROJECT / "gameplay"
STATE_ROOT = METAL / ".cache/dev-runtime"
INSTALLED_APP = Path("/Applications/Corsairs Iddictive Remaster.app")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def editable(relative: Path) -> bool:
    parts = relative.parts
    return (len(parts) > 1 and parts[0] == "PROGRAM" and relative.suffix.lower() in (".c", ".h")) or (
        len(parts) > 2 and parts[:2] == ("RESOURCE", "INI") and relative.suffix.lower() == ".ini"
    ) or (len(parts) > 2 and parts[:2] == ("RESOURCE", "techniques") and relative.suffix.lower() == ".fx")


def content_path(root: Path, relative: Path) -> Path:
    if not editable(relative):
        raise RuntimeError(f"Outside editable PROGRAM/RESOURCE scope: {relative}")
    return safe_path(root, relative)


def resources() -> Path:
    return safe_path(INSTALLED_APP, Path("Contents/Resources"))


def files(root: Path) -> list[Path]:
    result = []
    for category in ("PROGRAM", "RESOURCE/INI", "RESOURCE/techniques"):
        start = safe_path(root, Path(category))
        if not start.exists():
            continue
        for directory, dirs, names in os.walk(start, followlinks=False):
            for name in dirs + names:
                path = Path(directory) / name
                if path.is_symlink():
                    raise RuntimeError(f"Linked workspace/content path: {path}")
            for name in names:
                relative = (Path(directory) / name).relative_to(root)
                if editable(relative):
                    result.append(relative)
    return sorted(result)


@contextmanager
def session():
    safe_path(STATE_ROOT).mkdir(parents=True, exist_ok=True)
    with safe_path(STATE_ROOT, Path(".lock")).open("a+b") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise RuntimeError("Another dev helper is changing this workspace.") from error
        yield


def player_guard():
    return delivery_player_guard(INSTALLED_APP, require_initialized=True)


def read_state() -> dict:
    path = safe_path(STATE_ROOT, Path("state.json"))
    state = json.loads(path.read_text()) if path.exists() else {
        "version": 1, "app": str(INSTALLED_APP), "files": {}
    }
    if state.get("version") != 1 or state.get("app") != str(INSTALLED_APP) or not isinstance(state.get("files"), dict):
        raise RuntimeError("Dev snapshot does not belong to this installed app.")
    for name, row in state["files"].items():
        content_path(resources(), Path(name))
        for field in ("original", "applied"):
            value = row.get(field, "")
            if len(value) != 64 or any(c not in "0123456789abcdef" for c in value):
                raise RuntimeError(f"Invalid snapshot for {name}")
        original(row)
    return state


def original(row: dict) -> bytes:
    data = safe_path(STATE_ROOT, Path("originals") / row["original"]).read_bytes()
    if digest(data) != row["original"]:
        raise RuntimeError("Original dev snapshot changed; refusing recovery from corrupt bytes.")
    return data


def snapshot(data: bytes) -> str:
    key = digest(data)
    path = safe_path(STATE_ROOT, Path("originals") / key)
    if path.exists():
        if path.read_bytes() != data:
            raise RuntimeError("Original dev snapshot changed.")
    else:
        atomic_write(path, data)
    return key


def transact(changes: dict[Path, tuple[bytes | None, bytes]], state: dict,
             app_changed: bool, delivery: DeliveryState | None = None) -> None:
    state_path = safe_path(STATE_ROOT, Path("state.json"))
    before_state = state_path.read_bytes() if state_path.exists() else None
    changes[state_path] = (before_state, (json.dumps(state, indent=2, sort_keys=True) + "\n").encode())
    if delivery is not None:
        changes[delivery.receipt] = delivery.change()
    deliver(changes, INSTALLED_APP if app_changed or delivery is not None else None)


def push_locked(state: dict, revert: Path | None = None) -> int:
    selected = [revert] if revert is not None else files(WORKSPACE)
    changes, originals = {}, []
    rows = state["files"]
    delivery = DeliveryState(resources(), INSTALLED_APP)
    app_changed = False
    for relative in selected:
        target = content_path(resources(), relative)
        workspace = content_path(WORKSPACE, relative)
        before = target.read_bytes()
        row = rows.get(str(relative))
        if revert is not None:
            if row is None:
                raise RuntimeError(f"No original dev snapshot for {relative}; pull before editing.")
            after = original(row)
        else:
            after = workspace.read_bytes()
        if row is None:
            if before != after:
                raise RuntimeError(f"Unsnapshotted workspace differs from installed {relative}; preserve/reconcile it before pull.")
            originals.append(before)
        elif digest(before) not in (row["applied"], digest(after)):
            raise RuntimeError(f"Unknown installed edit: {relative}; no files changed.")
        delivery.admit(str(relative), before, after, {digest(before)}, development=True)
        if before != after:
            changes[target] = (before, after)
            app_changed = True
        if revert is not None:
            ws_before = workspace.read_bytes() if workspace.exists() else None
            changes[workspace] = (ws_before, after)
        rows[str(relative)] = {"original": row["original"] if row else digest(before), "applied": digest(after)}
        owner = "stage" if rows[str(relative)]["original"] == digest(after) else "development"
        delivery.track(str(relative), after, owner)
    for data in originals:
        snapshot(data)
    transact(changes, state, app_changed, delivery)
    return sum(1 for path in changes if path.is_relative_to(resources()) and path != delivery.receipt)


def cmd_push(quiet: bool = False, revert: Path | None = None) -> int:
    if revert is None and not files(WORKSPACE):
        if not quiet:
            print("No editable workspace files; pull before editing.")
        return 0
    with session(), player_guard():
        count = push_locked(read_state(), revert)
    if not quiet:
        print(f"Installed content: {count} file(s) updated; originals retained.")
    return 0


def cmd_promote(name: str) -> int:
    """Make a snapshotted dev edit the canonical source and stage it once."""
    with session(), player_guard():
        source_set = gameplay_sources.read([name])
        state = read_state()
        row = state["files"].get(name)
        if row is None:
            raise RuntimeError(f"No original dev snapshot for {name}; pull before editing.")
        relative = Path(name)
        workspace = content_path(WORKSPACE, relative)
        edited = workspace.read_bytes()
        sources, inputs = source_set
        current, legacy, encoding = sources[name]
        incoming = gameplay_sources.runtime_bytes(name, gameplay_sources.source_bytes(name, edited, encoding), encoding)
        installed = content_path(resources(), relative).read_bytes()
        if digest(installed) != row["applied"]:
            raise RuntimeError(f"Unknown installed edit: {name}; no files changed.")
        if gameplay_sources.source_bytes(name, current, encoding) not in (
                gameplay_sources.source_bytes(name, original(row), encoding),
                gameplay_sources.source_bytes(name, incoming, encoding)):
            raise RuntimeError(f"Canonical source changed since dev snapshot: {name}")
        source = safe_path(gameplay_sources.ROOT, relative)
        inputs[source] = (inputs[source][0], gameplay_sources.source_bytes(name, incoming, encoding))
        sources[name] = (incoming, legacy, encoding)
        changes = gameplay_sources.prepare_delivery(METAL / ".cache/runtime", source_set)
        changes.update(gameplay_sources.prepare_delivery(resources(), source_set, development=True))
        changes[workspace] = (edited, incoming)
        key = snapshot(incoming)
        state["files"][name] = {"original": key, "applied": key}
        count = sum(before != after for path, (before, after) in changes.items()
                    if path.is_relative_to(resources()) and path.name != RECEIPT)
        transact(changes, state, True)
    print(f"Promoted {name} to canonical source; installed {count} file(s).")
    return 0


def pull_locked(state: dict, all_files: bool = False) -> None:
    selected = set(files(WORKSPACE))
    for relative in files(resources()):
        path = content_path(resources(), relative)
        baseline = content_path(BASELINE, relative)
        if all_files or not baseline.is_file() or path.read_bytes() != baseline.read_bytes():
            selected.add(relative)
    changes, originals = {}, []
    for relative in sorted(selected):
        installed = content_path(resources(), relative).read_bytes()
        workspace = content_path(WORKSPACE, relative)
        before = workspace.read_bytes() if workspace.exists() else None
        row = state["files"].get(str(relative))
        if before is not None and before != installed and (row is None or digest(before) != row["applied"]):
            raise RuntimeError(f"Pending/unknown workspace edit: {relative}; pull preserves it and changes nothing.")
        if row and digest(installed) != row["applied"] and row["original"] != row["applied"]:
            raise RuntimeError(f"Installed upgrade conflicts with active dev edits: {relative}")
        original_hash = row["original"] if row and digest(installed) == row["applied"] else digest(installed)
        originals.append(installed)
        state["files"][str(relative)] = {"original": original_hash, "applied": digest(installed)}
        changes[workspace] = (before, installed)
    for data in originals:
        snapshot(data)
    transact(changes, state, False)
    print(f"Installed snapshot: {len(selected)} editable file(s); workspace edits preserved.")


def cmd_pull(all_files: bool = False) -> int:
    with session():
        pull_locked(read_state(), all_files)
    return 0


def require_clean_build(state: dict) -> None:
    for name, row in state["files"].items():
        if row["original"] != row["applied"]:
            raise RuntimeError(f"--build refuses active dev edits ({name}); integrate or revert them before canonical staging.")
    for relative in files(WORKSPACE):
        installed = content_path(resources(), relative).read_bytes()
        if content_path(WORKSPACE, relative).read_bytes() != installed:
            raise RuntimeError(f"--build refuses pending workspace edits ({relative}); no sync/build ran.")


def cmd_launch(rebuild: bool = False, no_sync: bool = False, bg: bool = False) -> int:
    require_idle()
    launcher = safe_path(INSTALLED_APP, Path("Contents/MacOS/launch"))
    engine = safe_path(INSTALLED_APP, Path("Contents/MacOS/metal-engine"))
    if not launcher.is_file() or not engine.is_file():
        raise RuntimeError("Installed remaster is missing; this helper never launches the cache.")
    with session():
        state = read_state()
        if rebuild:
            require_clean_build(state)
            # Capture the prior installed state before a normal canonical upgrade.
            pull_locked(state)
            subprocess.run([str(METAL / "run.sh"), "--stage-only"], check=True)
            pull_locked(read_state())
        elif not no_sync and files(WORKSPACE):
            with player_guard():
                push_locked(state)
        print(f"Launching {INSTALLED_APP}; public launcher owns player state.")
        if bg:
            process = subprocess.Popen([str(launcher)], start_new_session=True)
            print(f"Installed launcher PID: {process.pid}")
            return 0
        return subprocess.run([str(launcher)]).returncode


def cmd_diff(target_path: str | None = None) -> int:
    state = read_state()
    for relative in files(WORKSPACE):
        if target_path and target_path not in str(relative):
            continue
        row = state["files"].get(str(relative))
        if row is None:
            print(f"Unsnapshotted: {relative}")
            continue
        before = original(row).decode(errors="replace").splitlines(keepends=True)
        after = content_path(WORKSPACE, relative).read_text(errors="replace").splitlines(keepends=True)
        sys.stdout.writelines(difflib.unified_diff(before, after, fromfile=f"original/{relative}", tofile=f"workspace/{relative}"))
    return 0


def cmd_status() -> int:
    state = read_state()
    print(f"Target: {INSTALLED_APP}")
    print("Launch/state: installed Contents/MacOS/launch -> public_launcher.py")
    print(f"Workspace: {len(files(WORKSPACE))} editable file(s); snapshotted: {len(state['files'])}")
    for relative in files(WORKSPACE):
        row = state["files"].get(str(relative))
        workspace = content_path(WORKSPACE, relative).read_bytes()
        installed = content_path(resources(), relative).read_bytes()
        label = "unsnapshotted" if row is None else "installed conflict" if digest(installed) != row["applied"] else "pending" if workspace != installed else "synced"
        if label != "synced":
            print(f"{label}: {relative}")
    return 0


def cmd_watch(interval: float = 0.5) -> int:
    if not 0.25 <= interval <= 60:
        raise RuntimeError("Watch interval must be between 0.25 and 60 seconds.")
    print("Watching workspace; delivery waits for the game to close. Ctrl+C stops.")
    previous, last_error = None, None
    try:
        while True:
            current = [(str(p), digest(content_path(WORKSPACE, p).read_bytes())) for p in files(WORKSPACE)]
            if current != previous:
                try:
                    cmd_push(quiet=True)
                    previous, last_error = current, None
                    print("Installed workspace changes; restart/reopen the changed consumer to replay.")
                except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
                    if str(error) != last_error:
                        print(str(error), file=sys.stderr)
                        last_error = str(error)
            time.sleep(interval)
    except KeyboardInterrupt:
        print("Watch stopped.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command")
    launch = commands.add_parser("launch")
    launch.add_argument("--build", "-b", action="store_true")
    launch.add_argument("--no-sync", action="store_true")
    launch.add_argument("--bg", action="store_true")
    push = commands.add_parser("push")
    push.add_argument("--quiet", "-q", action="store_true")
    pull = commands.add_parser("pull")
    pull.add_argument("--all", "-a", action="store_true")
    commands.add_parser("status")
    diff = commands.add_parser("diff")
    diff.add_argument("path", nargs="?")
    watch = commands.add_parser("watch")
    watch.add_argument("--interval", "-i", type=float, default=0.5)
    revert = commands.add_parser("revert")
    revert.add_argument("path", type=Path)
    promote = commands.add_parser("promote")
    promote.add_argument("path", help="Exact registered canonical source path")
    args = parser.parse_args()
    try:
        if args.command in (None, "launch"):
            return cmd_launch(getattr(args, "build", False), getattr(args, "no_sync", False), getattr(args, "bg", False))
        if args.command == "push":
            return cmd_push(args.quiet)
        if args.command == "pull":
            return cmd_pull(args.all)
        if args.command == "status":
            return cmd_status()
        if args.command == "diff":
            return cmd_diff(args.path)
        if args.command == "watch":
            return cmd_watch(args.interval)
        if args.command == "promote":
            return cmd_promote(args.path)
        return cmd_push(revert=args.path)
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Dev runtime: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
