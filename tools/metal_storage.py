#!/usr/bin/env python3
"""Bounded storage and process ownership for Metal build/export diagnostics."""
from __future__ import annotations

import argparse
from contextlib import contextmanager
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import threading
import time

CACHE = Path(__file__).resolve().parents[1] / "experiments/native-metal/.cache"
GIB = 1024 ** 3
RESERVE = 8 * GIB
DIAGNOSTICS_LIMIT = 8 * GIB
PROFILE_LIMIT = 3 * GIB
BUILD_LIMIT = 8 * GIB


def tree_bytes(path: Path, *, follow_symlinks: bool = False) -> int:
    """Logical copy bound, including 4KiB/node; monitors never follow links.

    Explicit dereferencing matches copytree(symlinks=False), used for the Python
    framework export. Repeated targets count again; ancestor cycles are refused.
    """
    def visit(node, ancestors):
        try:
            info = node.stat() if follow_symlinks else node.lstat()
        except FileNotFoundError:
            return 0
        size = 4096 + (info.st_size if not stat.S_ISDIR(info.st_mode) else 0)
        if stat.S_ISDIR(info.st_mode):
            identity = (info.st_dev, info.st_ino)
            if identity in ancestors:
                raise RuntimeError(f"directory cycle in copy input: {node}")
            with os.scandir(node) as entries:
                for entry in entries:
                    size += visit(Path(entry.path), ancestors | {identity})
        return size
    return visit(Path(path), set())


def ensure_space(required_bytes: int = 0, path: Path | None = None) -> None:
    if required_bytes < 0:
        raise ValueError("required storage must be nonnegative")
    ancestor = Path(path) if path is not None else CACHE
    while not ancestor.exists():
        if ancestor == ancestor.parent:
            raise RuntimeError(f"no existing output ancestor: {path}")
        ancestor = ancestor.parent
    free = shutil.disk_usage(ancestor).free
    if free < required_bytes + RESERVE:
        raise RuntimeError(f"insufficient storage on {ancestor}: need {required_bytes} bytes "
                           f"plus {RESERVE} reserve, have {free}")


def ensure_build_size(*, cache: Path = CACHE, limit: int = BUILD_LIMIT) -> None:
    build = Path(cache) / "build"
    if build.is_symlink():
        raise RuntimeError("linked canonical build is outside storage ownership")
    size = tree_bytes(build)
    if size > limit:
        raise RuntimeError(f"canonical build exceeds {limit} byte limit ({size}); "
                           "current build preserved for explicit review")


@contextmanager
def _termination_scope():
    previous = None
    if threading.current_thread() is threading.main_thread():
        previous = signal.getsignal(signal.SIGTERM)
        def terminate(signum, frame):
            raise SystemExit(128 + signum)
        signal.signal(signal.SIGTERM, terminate)
    try:
        yield
    finally:
        if previous is not None:
            signal.signal(signal.SIGTERM, previous)


def _identity(path):
    info = path.lstat()
    return info.st_dev, info.st_ino


def _remove_owned(path, identity):
    try:
        if _identity(path) != identity:
            raise RuntimeError(f"output replaced by another owner; preserved: {path}")
    except FileNotFoundError:
        return
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink()


@contextmanager
def owned_output(path: Path, kind: str):
    """Reserve an exclusive root; only failed outputs created here are removed."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if kind == "directory":
        path.mkdir()
    elif kind == "file":
        path.open("xb").close()
    else:
        raise ValueError(f"unknown output kind: {kind}")
    identity = _identity(path)
    descriptor = os.open(path, os.O_RDONLY)
    try:
        with _termination_scope():
            try:
                yield path
            except BaseException:
                _remove_owned(path, identity)
                raise
    finally:
        os.close(descriptor)


@contextmanager
def scratch_output(parent: Path, prefix: str):
    parent = Path(parent)
    parent.mkdir(parents=True, exist_ok=True)
    path = Path(tempfile.mkdtemp(prefix=prefix, dir=parent))
    identity = _identity(path)
    descriptor = os.open(path, os.O_RDONLY)
    try:
        with _termination_scope():
            try:
                yield path
            finally:
                _remove_owned(path, identity)
    finally:
        os.close(descriptor)


def _pins(cache):
    path = cache / "storage-pins.json"
    if not path.exists():
        return []
    if path.is_symlink():
        raise RuntimeError("storage pins must be a regular local file")
    values = json.loads(path.read_text())["pins"]
    result = []
    for value in values:
        relative = Path(value)
        if relative.is_absolute() or ".." in relative.parts or not relative.parts:
            raise RuntimeError(f"invalid storage pin: {value}")
        result.append(cache / relative)
    return result


def _active(path, root):
    for parent in (path, *path.parents):
        if not parent.is_relative_to(root):
            break
        lock = parent / ".storage.lock"
        if lock.is_file() and not lock.is_symlink():
            with lock.open("r+") as handle:
                try:
                    fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
                except BlockingIOError:
                    return True
                finally:
                    fcntl.flock(handle, fcntl.LOCK_UN)
        records = parent / ".storage-processes.json"
        if records.is_file() and not records.is_symlink():
            for record in json.loads(records.read_text()):
                if record["identity"] and _process_identity(record["pid"]) == record["identity"]:
                    return True
    return False


def _process_identity(pid):
    result = subprocess.run(["ps", "-p", str(int(pid)), "-o", "lstart="],
                            capture_output=True, text=True, timeout=5)
    return result.stdout.strip() if result.returncode == 0 else ""


def maintain(*, cache: Path = CACHE, diagnostics_limit: int = DIAGNOSTICS_LIMIT) -> None:
    """Prune only known generated trace bundles, oldest first; retain evidence."""
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    with (cache / ".storage-maintenance.lock").open("a+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        _maintain(cache, diagnostics_limit)


def _maintain(cache, diagnostics_limit):
    diagnostics = cache / "diagnostics"
    if diagnostics.is_symlink():
        raise RuntimeError("linked diagnostics root is outside storage ownership")
    if not diagnostics.exists():
        return
    pins = _pins(cache)
    total = tree_bytes(diagnostics)
    if total <= diagnostics_limit:
        return
    candidates = []
    newest = None
    for directory, names, _ in os.walk(diagnostics, followlinks=False):
        parent = Path(directory)
        for name in list(names):
            path = parent / name
            if path.is_symlink():
                names.remove(name)
            elif name.endswith((".trace", ".gputrace")):
                names.remove(name)
                stamp = path.stat().st_mtime_ns
                if newest is None or stamp > newest[0]:
                    newest = (stamp, path)
                pinned = any(path == pin or path.is_relative_to(pin) or pin.is_relative_to(path)
                             for pin in pins)
                if not pinned and not _active(path, diagnostics):
                    candidates.append((stamp, path, _identity(path)))
    for _, path, identity in sorted(candidates):
        if total <= diagnostics_limit:
            break
        if path == newest[1] or _active(path, diagnostics):
            continue
        size = tree_bytes(path)
        _remove_owned(path, identity)
        total -= size
        print(f"storage: removed generated trace {path}", flush=True)
    if total > diagnostics_limit:
        raise RuntimeError(f"diagnostics exceed {diagnostics_limit} bytes after trace pruning "
                           f"({total} retained); pinned, active and non-trace evidence preserved")


def prune_engine_backups(current_hash: str, *, cache: Path = CACHE) -> None:
    if not re.fullmatch(r"[0-9a-f]{64}", current_hash):
        raise ValueError("invalid installed engine hash")
    root = Path(cache) / "installed-engine-backup"
    if root.is_symlink():
        raise RuntimeError("linked engine backup root is outside storage ownership")
    if not root.exists():
        return
    candidates = [p for p in root.iterdir() if not p.is_symlink() and p.is_file()
                  and re.fullmatch(r"metal-engine\.[0-9a-f]{64}", p.name)]
    prior = [p for p in candidates if p.name != f"metal-engine.{current_hash}"]
    keep = max(prior, key=lambda p: p.stat().st_mtime_ns) if prior else None
    for path in candidates:
        if path != keep:
            _remove_owned(path, _identity(path))


class ProcessBudget:
    """Own only fresh sessions; monitor run outputs and private tool scratch."""
    def __init__(self, root: Path, limit: int = PROFILE_LIMIT):
        self.root, self.limit = Path(root), limit
        self.processes = []
        self.identities = []
        self.stopped = threading.Event()
        self.failure = None
        self.thread = threading.Thread(target=self._monitor, daemon=True)

    def _monitor(self):
        while not self.stopped.wait(.2):
            try:
                if tree_bytes(self.root) > self.limit:
                    raise RuntimeError(f"profile exceeded {self.limit} byte cap: {self.root}")
                ensure_space(path=self.root)
            except Exception as error:
                self.failure = error
                for process in tuple(self.processes):
                    self._signal(process, signal.SIGTERM)
                return

    def _owned_group(self, process):
        # WNOWAIT leaves even an exited session leader unreaped. Its PID/PGID
        # cannot be reused while descendants are being stopped. Never reacquire
        # a reaped PID merely because another group with that number exists.
        if process.returncode is not None:
            return False
        try:
            status = os.waitid(os.P_PID, process.pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
            # Darwin getpgid reports ESRCH for a zombie; WNOWAIT still proves
            # that this exact, unreaped child reserves its original group ID.
            return ((status is not None or os.getpgid(process.pid) == process.pid)
                    and _process_identity(process.pid) == next(
                        row["identity"] for row in self.identities if row["pid"] == process.pid))
        except (ChildProcessError, ProcessLookupError, StopIteration):
            return False

    def _signal(self, process, sig):
        if not self._owned_group(process) or not self._group_running(process.pid):
            return
        try:
            os.killpg(process.pid, sig)
        except ProcessLookupError:
            pass
        except PermissionError:
            # Darwin can return EPERM after the last live member disappeared.
            if self._group_running(process.pid):
                raise

    @staticmethod
    def _group_running(pgid):
        result = subprocess.run(["ps", "-axo", "pgid=,state="], capture_output=True,
                                text=True, check=True, timeout=5)
        return any(int(group) == pgid and not state.startswith("Z")
                   for group, state in (line.split() for line in result.stdout.splitlines()))

    def start(self, args, **kwargs):
        self.check()
        if not hasattr(os, "waitid") or not hasattr(os, "WNOWAIT"):
            raise RuntimeError("safe owned-session cleanup requires waitid/WNOWAIT")
        process = subprocess.Popen(args, start_new_session=True, **kwargs)
        self.processes.append(process)
        self.identities.append({"pid": process.pid, "identity": _process_identity(process.pid)})
        temporary = self.root / ".storage-processes.json.tmp"
        temporary.write_text(json.dumps(self.identities) + "\n")
        temporary.replace(self.root / ".storage-processes.json")
        return process

    def wait(self, process, timeout):
        deadline = time.monotonic() + timeout
        while True:
            status = os.waitid(os.P_PID, process.pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
            if status is not None:
                break
            self.check()
            if time.monotonic() >= deadline:
                raise subprocess.TimeoutExpired(process.args, timeout)
            time.sleep(.1)
        self.check(final=True)
        return status.si_status if status.si_code == os.CLD_EXITED else -status.si_status

    def check(self, *, final=False):
        if self.failure is not None:
            raise self.failure
        if final:
            if tree_bytes(self.root) > self.limit:
                raise RuntimeError(f"profile exceeded {self.limit} byte cap: {self.root}")
            ensure_space(path=self.root)

    def close(self):
        self.stopped.set()
        self.thread.join()
        owned = [process for process in self.processes if self._owned_group(process)]
        for process in owned:
            self._signal(process, signal.SIGTERM)
        deadline = time.monotonic() + 2
        while any(self._group_running(process.pid) for process in owned):
            if time.monotonic() >= deadline:
                break
            time.sleep(.05)
        for process in owned:
            self._signal(process, signal.SIGKILL)
        deadline = time.monotonic() + 5
        while any(self._group_running(process.pid) for process in owned):
            if time.monotonic() >= deadline:
                raise RuntimeError("owned process group still running; leader identities retained")
            time.sleep(.05)
        # Reaping is the final operation: no more signals or group-number probes.
        for process in self.processes:
            process.wait(timeout=5)


@contextmanager
def diagnostic_run(path: Path):
    with _termination_scope(), _diagnostic_run(path) as owned:
        yield owned


@contextmanager
def _diagnostic_run(path: Path):
    """Retain measurements/failure logs, release flock and private tool files."""
    maintain()
    ensure_space(PROFILE_LIMIT, path)
    path.mkdir(parents=True, exist_ok=False)
    failed = False
    try:
        with (path / ".storage.lock").open("x+") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            try:
                temporary = Path(tempfile.mkdtemp(prefix=".tooltmp-", dir=path))
                identity = _identity(temporary)
                budget = ProcessBudget(path)
                budget.thread.start()
                original_error = None
                try:
                    yield temporary, budget
                except BaseException as error:
                    original_error = error
                    raise
                finally:
                    try:
                        budget.close()
                    except BaseException as cleanup_error:
                        if original_error is None:
                            raise
                        print(f"owned process cleanup incomplete; scratch retained at {temporary}: "
                              f"{cleanup_error}", file=sys.stderr)
                    else:
                        _remove_owned(temporary, identity)
            finally:
                fcntl.flock(lock, fcntl.LOCK_UN)
    except BaseException as error:
        failed = True
        (path / "storage-failure.log").write_text(f"{type(error).__name__}: {error}\n")
        raise
    finally:
        try:
            maintain()
        except Exception as error:
            if not failed:
                raise
            print(f"storage maintenance after failed run: {error}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--installed-hash")
    args = parser.parse_args()
    if args.installed_hash:
        prune_engine_backups(args.installed_hash)
    else:
        ensure_build_size()
        maintain()
        ensure_space()


if __name__ == "__main__":
    main()
