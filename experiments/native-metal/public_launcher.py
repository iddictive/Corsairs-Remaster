#!/usr/bin/env python3
"""Public launcher coordinator for Corsairs Iddictive Remaster.app.

Runs inside the standalone macOS application bundle under the bundled,
isolated Python 3.14 runtime. Manages user-state initialization, symlinks
to application resource roots, graphics settings synchronization, and
execve handover to the native Metal engine binary.
"""

from __future__ import annotations

import argparse
import fcntl
import os
import shutil
import sys
from pathlib import Path


DEFAULT_APP_SUPPORT_SUBDIR = "Iddictive Corsairs"


def resolve_bundle_paths(script_path: Path) -> tuple[Path, Path, Path, Path]:
    """Resolve (resources_dir, contents_dir, bundle_dir, engine_binary)."""
    resources_dir = script_path.resolve().parent
    contents_dir = resources_dir.parent
    bundle_dir = contents_dir.parent
    engine_binary = contents_dir / "MacOS" / "metal-engine"
    return resources_dir, contents_dir, bundle_dir, engine_binary


def resolve_user_dir(custom_path: Path | None = None) -> Path:
    """Resolve the player's persistent state directory."""
    if custom_path is not None:
        return custom_path.resolve()
    env_override = os.environ.get("CORSAIRS_USER_DIR")
    if env_override:
        return Path(env_override).resolve()
    return Path.home() / "Library/Application Support" / DEFAULT_APP_SUPPORT_SUBDIR


def sync_symlink(link_path: Path, target_path: Path) -> None:
    """Ensure link_path is a symlink pointing to target_path, updating if moved."""
    target_resolved = target_path.resolve()
    if link_path.is_symlink():
        try:
            raw_target = os.readlink(link_path)
            target_from_link = Path(raw_target)
            if not target_from_link.is_absolute():
                target_from_link = link_path.parent / target_from_link
            current_target = target_from_link.resolve()
        except OSError:
            current_target = None
        if current_target == target_resolved and current_target.is_dir():
            return
        link_path.unlink()
    elif link_path.exists():
        sys.exit(f"Error: Existing non-symlink path found at {link_path}. Refusing unknown stale owner.")
    os.symlink(str(target_resolved), link_path)


def stage_user_state(resources_dir: Path, user_dir: Path) -> None:
    """Stage and update user state directory with required symlinks and configs."""
    user_dir.mkdir(parents=True, exist_ok=True)
    (user_dir / "SAVE").mkdir(parents=True, exist_ok=True)
    (user_dir / "userdata").mkdir(parents=True, exist_ok=True)
    (user_dir / "logs").mkdir(parents=True, exist_ok=True)

    # Symlink read-only large resource trees (21GB) to app resources
    for resource_name in ("PROGRAM", "RESOURCE"):
        src = resources_dir / resource_name
        dst = user_dir / resource_name
        if not src.is_dir():
            raise RuntimeError(f"Missing bundle resource tree: {src}")
        if dst.exists() and not dst.is_symlink():
            sys.exit(f"Error: Existing non-symlink directory found at {dst}. Refusing unknown stale owner.")
        sync_symlink(dst, src)

    # Copy baseline mutable configs if not present in player state
    for config_name in ("engine.ini", "options", "project.df"):
        src = resources_dir / config_name
        dst = user_dir / config_name
        if not dst.exists() and not dst.is_symlink() and src.is_file():
            shutil.copy2(src, dst)


def build_launch_environment(user_dir: Path, inherited: dict[str, str]) -> dict[str, str]:
    """Compute engine runtime environment variables with graphics options."""
    env = dict(inherited)
    env["STORM_USERDATA"] = str((user_dir / "userdata").resolve())
    env.setdefault("STORM_METAL_MODERN_EFFECTS", "1")
    env.setdefault("STORM_METAL_ISLAND_GEOMETRY", "0")
    env.setdefault("STORM_METAL_PROFILE", "0")
    env.setdefault("STORM_METAL_ROCKK2", "0")
    env.setdefault("STORM_TRACE_DECK_WALK", "0")

    for key in ("DXVK_CONFIG_FILE", "DXVK_WSI_DRIVER", "DXVK_LOG_LEVEL", "MVK_CONFIG_LOG_LEVEL"):
        env.pop(key, None)

    # Import canonical settings adapter
    import metal_graphics_settings as settings

    settings.initialize_record(user_dir)
    settings.apply_engine(user_dir)
    return settings.launch_environment(user_dir, env)


def rotate_launch_log(log_file: Path) -> None:
    """Keep at most one previous session, so the player log stays bounded.

    The engine writes raw stderr (diagnostics) into this file for the whole
    run; without rotation it grows forever. An empty leftover is left in
    place so a failed relaunch cannot discard the previous session.
    """
    try:
        if log_file.stat().st_size == 0:
            return
        os.replace(log_file, log_file.with_name(log_file.name + ".1"))
    except FileNotFoundError:
        return


def parse_args() -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(
        description="Public launcher coordinator for Corsairs Iddictive Remaster."
    )
    parser.add_argument(
        "--stage-only",
        action="store_true",
        help="Stage and synchronize user state directory and settings without launching engine.",
    )
    parser.add_argument(
        "--user-dir",
        type=Path,
        default=None,
        help="Override user state directory (default: ~/Library/Application Support/Iddictive Corsairs).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Check and display launch environment without executing engine.",
    )
    return parser.parse_known_args()


def main() -> int:
    args, unknown = parse_args()

    script_path = Path(__file__).resolve()
    resources_dir, contents_dir, bundle_dir, engine_binary = resolve_bundle_paths(script_path)

    # Ensure canonical settings module is importable
    if str(resources_dir) not in sys.path:
        sys.path.insert(0, str(resources_dir))

    user_dir = resolve_user_dir(args.user_dir)
    user_dir.mkdir(parents=True, exist_ok=True)

    # Serialize same user's launch / staging with nonblocking flock
    lock_file = user_dir / ".launch.lock"
    lock_fd = os.open(str(lock_file), os.O_CREAT | os.O_RDWR, 0o600)
    try:
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except (BlockingIOError, OSError):
        os.close(lock_fd)
        sys.exit("Error: Another instance of Corsairs Iddictive Remaster is already running or staging.")

    # Keep lock held across os.execve through kernel file description inheritance
    os.set_inheritable(lock_fd, True)

    try:
        stage_user_state(resources_dir, user_dir)
        env = build_launch_environment(user_dir, dict(os.environ))
    except Exception as exc:
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
        except OSError:
            pass
        os.close(lock_fd)
        sys.exit(f"Error during staging: {exc}")

    if args.stage_only:
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
        except OSError:
            pass
        os.close(lock_fd)
        print(f"Staged user state in: {user_dir}")
        return 0

    if args.dry_run:
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
        except OSError:
            pass
        os.close(lock_fd)
        print(f"Dry run launch configuration for: {user_dir}")
        print(f"Engine binary: {engine_binary}")
        for k in sorted(env.keys()):
            if k.startswith("STORM_"):
                print(f"  {k}={env[k]}")
        return 0

    if not engine_binary.is_file() or not os.access(engine_binary, os.X_OK):
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
        except OSError:
            pass
        os.close(lock_fd)
        sys.exit(f"Error: Engine executable not found or not executable: {engine_binary}")

    # Redirect logging to user state logs/launch.log, rotating one previous
    # session to launch.log.1 before the engine starts appending.
    log_file = user_dir / "logs" / "launch.log"
    rotate_launch_log(log_file)
    log_fd = os.open(str(log_file), os.O_CREAT | os.O_WRONLY | os.O_APPEND, 0o644)
    os.dup2(log_fd, 1)
    os.dup2(log_fd, 2)
    os.close(log_fd)

    # Transfer control to metal-engine with lock_fd held
    os.chdir(str(user_dir))
    binary_str = str(engine_binary.resolve())
    try:
        os.execve(binary_str, [binary_str], env)
    except OSError as exc:
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
        except OSError:
            pass
        os.close(lock_fd)
        sys.exit(f"Error executing engine {binary_str}: {exc}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
