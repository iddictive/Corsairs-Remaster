#!/usr/bin/env python3
"""Storage ownership for Metal builds, staging, retained evidence and diagnostics."""
from __future__ import annotations

import argparse
from contextlib import contextmanager
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import threading
import time

PROJECT = Path(__file__).resolve().parents[1]
CACHE = PROJECT / "experiments/native-metal/.cache"
GIB = 1024 ** 3
RESERVE = 8 * GIB
DIAGNOSTICS_LIMIT = 8 * GIB
PROFILE_LIMIT = 3 * GIB
BUILD_LIMIT = 8 * GIB
ARCHIVES_LIMIT = 8 * GIB


_waitid = getattr(os, "waitid", None)
if _waitid is None and sys.platform == "darwin":
    # CPython <3.13 hides waitid on Darwin, but libSystem supports WNOWAIT.
    # Match Darwin's public siginfo_t ABI (sys/signal.h); never reap to poll.
    import ctypes

    class _DarwinSiginfo(ctypes.Structure):
        _fields_ = [("si_signo", ctypes.c_int), ("si_errno", ctypes.c_int),
                    ("si_code", ctypes.c_int), ("si_pid", ctypes.c_int),
                    ("si_uid", ctypes.c_uint), ("si_status", ctypes.c_int),
                    ("si_addr", ctypes.c_void_p), ("si_value", ctypes.c_void_p),
                    ("si_band", ctypes.c_long), ("__pad", ctypes.c_ulong * 7)]

    _libc_waitid = ctypes.CDLL(None, use_errno=True).waitid
    _libc_waitid.argtypes = [ctypes.c_int, ctypes.c_uint,
                            ctypes.POINTER(_DarwinSiginfo), ctypes.c_int]
    _libc_waitid.restype = ctypes.c_int

    def _waitid(idtype, pid, options):
        status = _DarwinSiginfo()
        while _libc_waitid(idtype, pid, ctypes.byref(status), options) != 0:
            error = ctypes.get_errno()
            if error != errno.EINTR:
                raise OSError(error, os.strerror(error))
        return status if status.si_pid else None


def file_digest(path: Path) -> bytes:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 ** 2), b""):
            value.update(block)
    return value.digest()


def allocated_bytes(path: Path) -> int:
    """Conservative physical budget; never follow links into another owner."""
    total = 0
    for directory, _, names in os.walk(path, followlinks=False):
        for name in names:
            info = (Path(directory) / name).lstat()
            if stat.S_ISREG(info.st_mode):
                total += info.st_blocks * 512
    return total


def ensure_evidence_space(required_bytes: int, path: Path) -> None:
    archives = PROJECT / ".local-archives"
    if path.resolve().is_relative_to(archives.resolve()):
        if allocated_bytes(archives) + required_bytes > ARCHIVES_LIMIT:
            raise RuntimeError(f"Retained evidence exceeds the {ARCHIVES_LIMIT // GIB}GiB archive budget; "
                               "compact cold evidence before capturing another build")
    ensure_space(required_bytes, path)


def storage_report() -> dict:
    """Allocated bytes include shared APFS extents; they are not exclusive usage."""
    result = {}
    for name in ("dist", ".local-archives", "experiments/native-metal/.cache",
                 "experiments/native-metal/inputs"):
        root = PROJECT / name
        logical = allocated = files = 0
        for directory, _, names in os.walk(root, followlinks=False):
            for name in names:
                info = (Path(directory) / name).lstat()
                if stat.S_ISREG(info.st_mode):
                    logical += info.st_size
                    allocated += info.st_blocks * 512
                    files += 1
        result[str(root.relative_to(PROJECT))] = {
            "files": files, "logical_bytes": logical, "allocated_bytes": allocated}
    result["volume_free_bytes"] = shutil.disk_usage(PROJECT).free
    return result


def compress_copy(source: Path, target: Path) -> None:
    """Keep paths, metadata and readable bytes; use the native cold-file codec.

    ditto's default APFS clone retains the source's uncompressed extents, even
    with --hfsCompression. --noclone is necessary to actually encode the copy.
    """
    subprocess.run(["/usr/bin/ditto", "--noclone", "--hfsCompression",
                    str(source), str(target)], check=True, timeout=120)


def compact_evidence(excludes: list[Path]) -> dict:
    """Losslessly compact cold evidence, never prune it or touch working trees."""
    roots = (PROJECT / ".local-archives", CACHE / "diagnostics")
    excluded = [path.absolute() for path in excludes]
    # A complete open-file snapshot avoids rewriting currently consumed files.
    opened = subprocess.run(["/usr/sbin/lsof", "-Fn"], capture_output=True,
                            text=True, timeout=30)
    if opened.returncode not in (0, 1):
        raise RuntimeError("Cannot establish open-file consumers; evidence preserved")
    active = {line[1:] for line in opened.stdout.splitlines() if line.startswith("n/")}
    cutoff = time.time() - 24 * 60 * 60
    result = {"compacted_files": 0, "allocated_bytes_removed": 0, "files": []}

    def fingerprint(info):
        return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns,
                info.st_ctime_ns, info.st_mode, info.st_nlink)

    for root in roots:
        if root.is_symlink():
            raise RuntimeError(f"Linked evidence root preserved: {root}")
        for directory, names, files in os.walk(root, followlinks=False):
            parent = Path(directory)
            # A .git file also identifies an archived dirty linked worktree.
            if ((parent / ".git").exists() or parent.name.endswith(".git")
                    or any(parent == path or parent.is_relative_to(path) for path in excluded)
                    or _active(parent, root)):
                names[:] = []
                continue
            names[:] = [name for name in names if not (parent / name).is_symlink()]
            for name in files:
                source = parent / name
                before = source.lstat()
                if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1
                        or before.st_size < 256 * 1024
                        or max(before.st_mtime, before.st_ctime) > cutoff
                        or before.st_flags & stat.UF_COMPRESSED or str(source) in active
                        or any(source == path for path in excluded)):
                    continue
                ensure_space(before.st_size, source.parent)
                descriptor, temporary = tempfile.mkstemp(prefix=".storage-compress-", dir=parent)
                os.close(descriptor)
                target = Path(temporary)
                try:
                    original_hash = file_digest(source)
                    compress_copy(source, target)
                    after = target.stat()
                    saved = (before.st_blocks - after.st_blocks) * 512
                    if saved < 65536 or not after.st_flags & stat.UF_COMPRESSED:
                        continue
                    if (file_digest(target) != original_hash or file_digest(source) != original_hash
                            or fingerprint(source.lstat()) != fingerprint(before)
                            or after.st_mode != before.st_mode
                            or after.st_mtime_ns != before.st_mtime_ns):
                        raise RuntimeError(f"Evidence changed during compression: {source}")
                    os.replace(target, source)
                    result["compacted_files"] += 1
                    result["allocated_bytes_removed"] += saved
                    result["files"].append({"path": source.relative_to(PROJECT).as_posix(),
                                            "sha256": original_hash.hex(),
                                            "allocated_bytes_removed": saved})
                finally:
                    target.unlink(missing_ok=True)
    return result


def reclaim_runtime(state_output: Path, *, app: Path = Path(
        "/Applications/Corsairs Iddictive Remaster.app")) -> dict:
    """Drop a reproduced staging copy only after proving its installed source."""
    from export_metal_app import check_active_game
    from prepare_metal_inputs import verify
    from restore_metal_runtime import BASELINES, INPUTS

    runtime = CACHE / "runtime"
    bundle = plistlib.loads((app / "Contents/Info.plist").read_bytes())
    if bundle.get("CFBundleIdentifier") != "us.iddictive.corsairs":
        raise RuntimeError("Expected the standalone Iddictive Corsairs app")
    resources = app / "Contents/Resources"
    if runtime.is_symlink() or not runtime.is_dir():
        raise RuntimeError("Expected the ordinary local staging runtime")
    output = state_output.absolute()
    if output.is_relative_to(runtime) or output == runtime:
        raise ValueError("Retained state must be outside the reclaimed runtime")
    if output.exists() or output.is_symlink():
        raise FileExistsError("Retained-state output already exists; refusing overwrite")
    check_active_game(PROJECT)
    processes = subprocess.check_output(["ps", "-axo", "args="], text=True)
    commands = ("experiments/native-metal/run.sh", "experiments/native-metal/build.sh",
                "experiments/native-metal/install-engine.sh", "tools/package_metal_release.py",
                "tools/export_metal_app.py", "tools/sync_metal_gameplay.py")
    if any(command in line for line in processes.splitlines() for command in commands):
        raise RuntimeError("Build/staging/export consumer is active; runtime preserved")
    opened = subprocess.run(["/usr/sbin/lsof", "-nP", "+D", str(runtime)],
                            capture_output=True, text=True, timeout=30)
    if opened.returncode != 1 or opened.stdout.strip():
        raise RuntimeError("Staging runtime has an open consumer; preserved")
    # This is the normal restoration preflight, not an assumed rebuild recipe.
    verify(INPUTS)
    for name in BASELINES:
        if not (INPUTS / "runtime-baseline" / name).exists():
            raise RuntimeError(f"Missing reconstruction input: {name}")
    if any(path.is_file() or path.is_symlink() for path in (runtime / "SAVE").rglob("*")):
        raise RuntimeError("Development SAVE is nonempty; runtime preserved")
    snapshots = {}
    retained_assets = []
    content = hashlib.sha256()
    count = logical = 0

    def identity(path):
        info = path.lstat()
        return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns)

    for directory, names, files in os.walk(runtime, followlinks=False):
        parent = Path(directory)
        for path in [parent, *(parent / name for name in names + files)]:
            snapshots[path] = identity(path)
        for name in files + [name for name in names if (parent / name).is_symlink()]:
            source = parent / name
            relative = source.relative_to(runtime)
            if relative.parts[0] not in ("PROGRAM", "RESOURCE") or name == ".DS_Store":
                continue
            installed = resources / relative
            if source.is_symlink():
                if not installed.is_symlink() or os.readlink(source) != os.readlink(installed):
                    retained_assets.append(relative)
                value = os.readlink(source).encode()
            else:
                value = file_digest(source)
                if (not installed.is_file() or installed.is_symlink()
                        or installed.stat().st_size != source.stat().st_size
                        or value != file_digest(installed)):
                    retained_assets.append(relative)
                logical += source.stat().st_size
            content.update(relative.as_posix().encode() + b"\0" + value)
            count += 1
    if not count:
        raise RuntimeError("No staging assets verified; runtime preserved")
    state_size = sum(tree_bytes(path) for path in runtime.iterdir()
                     if path.name not in ("PROGRAM", "RESOURCE"))
    state_size += sum(tree_bytes(runtime / path) for path in retained_assets)
    ensure_evidence_space(state_size, output)
    result = {"verified_files": count, "logical_bytes_reclaimed": logical,
              "assets_sha256": content.hexdigest(), "reclaimed": False,
              "retained_asset_paths": [path.as_posix() for path in retained_assets],
              "restore_command": "python3 tools/restore_metal_runtime.py"}
    with owned_output(output, "directory"):
        retained = [source for source in runtime.iterdir()
                    if source.name not in ("PROGRAM", "RESOURCE")]
        retained.extend(runtime / path for path in retained_assets)
        for source in retained:
            target = output / source.relative_to(runtime)
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.is_symlink():
                target.symlink_to(os.readlink(source))
            elif source.is_dir():
                shutil.copytree(source, target, symlinks=True)
            else:
                shutil.copy2(source, target)
            for path in ([source] if not source.is_dir() else source.rglob("*")):
                backed_up = output / path.relative_to(runtime)
                if path.is_symlink():
                    if not backed_up.is_symlink() or os.readlink(path) != os.readlink(backed_up):
                        raise RuntimeError("Retained staging link failed verification")
                elif path.is_file() and file_digest(path) != file_digest(backed_up):
                    raise RuntimeError("Retained staging state failed verification")
        if any(identity(path) != previous for path, previous in snapshots.items()):
            raise RuntimeError("Staging runtime changed during verification; preserved")
        (output / "reclaim.json").write_text(json.dumps(result, indent=2) + "\n")
    _remove_owned(runtime, _identity(runtime))
    result["reclaimed"] = True
    (output / "reclaim.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


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
        try:
            shutil.rmtree(path)
        except OSError as error:
            # Finder can recreate this file between rmtree's scan and final rmdir.
            if error.errno != errno.ENOTEMPTY or _identity(path) != identity:
                raise
            remaining = list(path.iterdir())
            if (len(remaining) != 1 or remaining[0].name != '.DS_Store'
                    or remaining[0].is_symlink() or not remaining[0].is_file()):
                raise
            remaining[0].unlink()
            path.rmdir()
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
    total = allocated_bytes(diagnostics)
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
        size = allocated_bytes(path)
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
            status = _waitid(os.P_PID, process.pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
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
        if _waitid is None or not hasattr(os, "WNOWAIT"):
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
            status = _waitid(os.P_PID, process.pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
            # Darwin can report a stopped child even with only WEXITED set.
            if status is not None and status.si_code in (os.CLD_EXITED, os.CLD_KILLED,
                                                        os.CLD_DUMPED):
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
    parser.add_argument("--report", action="store_true", help="Read-only storage inventory")
    parser.add_argument("--compact-evidence", action="store_true",
                        help="Losslessly compress cold archives and diagnostics in place")
    parser.add_argument("--exclude", type=Path, action="append", default=[],
                        help="Preserve an active evidence path during compaction")
    parser.add_argument("--reclaim-runtime", action="store_true",
                        help="Verify and reclaim the disposable installed-resource copy")
    parser.add_argument("--state-output", type=Path,
                        help="New retained directory for staging config/receipts/logs")
    args = parser.parse_args()
    if args.report:
        print(json.dumps(storage_report(), indent=2))
    elif args.compact_evidence:
        print(json.dumps(compact_evidence(args.exclude), indent=2))
    elif args.reclaim_runtime:
        if args.state_output is None:
            parser.error("--reclaim-runtime requires --state-output")
        print(json.dumps(reclaim_runtime(args.state_output), indent=2))
    elif args.installed_hash:
        prune_engine_backups(args.installed_hash)
    else:
        ensure_build_size()
        maintain()
        ensure_space()


if __name__ == "__main__":
    main()
