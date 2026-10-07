"""Ownership and atomic delivery for Corsairs resources and the installed engine.

Receipts travel with their runtime, not with a checkout's disposable cache.
Legacy hashes admit an existing unrecorded installation once; a recorded
predecessor admits later versions without extending historical hash tables.
"""
from __future__ import annotations

import fcntl
import hashlib
import json
import shutil
import subprocess
from contextlib import contextmanager
from pathlib import Path

from runtime_script_patch import atomic_write

RECEIPT = ".delivery-state.json"
ENGINE = "@engine"
TECHNIQUES = ("ship/Rope.fx", "ship/Vant.fx", "_dev/ship.fx", "weather/SunGlow.fx")
# Frozen admission for installations created before receipts. New deliveries
# record their predecessor automatically; do not extend these lists per release.
LEGACY_TECHNIQUES = {
    "ship/Rope.fx": {"5f0f0bef056f652515f2ef2b4b99b16f936ce30b5dd1f0417a13eec17ddbb9e1", "873feea8d12addfef10b0c6dabf106b93e88fc0574c54fb2b14e544eff887e69"},
    "ship/Vant.fx": {"00a92cad48c211d0465fccc19f2481a29310d94676b0988700357eefe566d65f"},
    "_dev/ship.fx": {"e868be15c45b8b2d91489b939933b0c420669f241ac6ea5cde1c9e04465fd8e9", "092a2b5a087dd44d2e3c0eb779d167819fa672398b80189e1f05a75fffaf9621", "1629e130e608483aa5ab640d8d429d3d882810889306875ad3d9e515f87b46f9"},
    "weather/SunGlow.fx": {"326efdd05bdea7fd257b23126b3ffe64f9afbeb8eca51868f78dcf960e8a0f0c", "11abcfe7065d4f652c0204f32f048da1f1e24adf1cfe4085b3a3d3d76c5662cd", "014bbf13890f74450369f8b74269f5518356457fdbcee4be890374aeeb2b527e"},
}
NATIVE_RESOURCES = tuple("RESOURCE/techniques/" + name for name in TECHNIQUES) + (
    "resource/shared/messages.h", "public_launcher.py",
    "RESOURCE/Textures/battle_interface/mast_repair.tga.tx",
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def safe_path(root: Path, relative: Path = Path()) -> Path:
    if relative.is_absolute() or ".." in relative.parts:
        raise RuntimeError(f"Invalid delivery path: {relative}")
    target = root / relative
    for path in (target, *target.parents):
        if path.is_symlink():
            raise RuntimeError(f"Linked delivery path: {path}")
    return target


class DeliveryState:
    def __init__(self, resources: Path, app: Path | None = None):
        self.resources, self.app = resources, app
        if app is not None and resources != app / "Contents/Resources":
            raise RuntimeError("Delivery resources do not belong to the app.")
        self.receipt = safe_path(resources, Path(RECEIPT))
        self.before = self.receipt.read_bytes() if self.receipt.exists() else None
        record = json.loads(self.before) if self.before is not None else {"version": 1, "files": {}}
        if (not isinstance(record, dict) or set(record) != {"version", "files"}
                or type(record["version"]) is not int or record["version"] != 1
                or not isinstance(record["files"], dict)):
            raise RuntimeError("Unsupported delivery receipt; no files changed.")
        self.files = record["files"]
        for name, row in self.files.items():
            self.path(name)
            if not isinstance(row, dict) or set(row) not in ({"sha256", "owner"}, {"sha256", "owner", "source"}):
                raise RuntimeError(f"Invalid delivery record: {name}")
            if "source" in row:
                self.source_name(row["source"])
            sha = row["sha256"]
            if (not isinstance(sha, str) or len(sha) != 64
                    or any(c not in "0123456789abcdef" for c in sha)
                    or row["owner"] not in ("stage", "development")):
                raise RuntimeError(f"Invalid delivery record: {name}")

    def path(self, name: str) -> Path:
        if name == ENGINE:
            if self.app is None:
                raise RuntimeError("A resource runtime cannot own an engine.")
            return safe_path(self.app, Path("Contents/MacOS/metal-engine"))
        relative = Path(name)
        parts = relative.parts
        content = (len(parts) > 1 and parts[0] == "PROGRAM"
                   and relative.suffix.lower() in (".c", ".h", ".txt")) or (
            len(parts) > 2 and parts[:2] == ("RESOURCE", "INI")) or (
            len(parts) > 2 and parts[:2] == ("RESOURCE", "techniques")
            and relative.suffix.lower() == ".fx")
        if relative.as_posix() != name or (name not in NATIVE_RESOURCES and not content):
            raise RuntimeError(f"Outside managed delivery scope: {name}")
        return safe_path(self.resources, relative)

    def admit(self, name: str, current: bytes | None, incoming: bytes, legacy=(),
              development: bool = False, create: bool = False) -> None:
        self.path(name)
        row = self.files.get(name)
        if current is None:
            if row is not None or not create:
                raise RuntimeError(f"Missing delivered file: {name}; no files changed.")
            return
        if row is not None:
            if digest(current) != row["sha256"]:
                raise RuntimeError(f"Unknown delivered-file edit: {name}; no files changed.")
            if row["owner"] == "development" and current != incoming and not development:
                raise RuntimeError(f"Active development edit: {name}; promote or revert it first.")
        elif current != incoming and digest(current) not in legacy:
            raise RuntimeError(f"Unrecognized legacy delivery: {name}; no files changed.")

    @staticmethod
    def source_name(source: str) -> None:
        if (not isinstance(source, str) or not source or len(source) > 512
                or "\0" in source or Path(source).is_absolute() or ".." in Path(source).parts
                or Path(source).as_posix() != source):
            raise RuntimeError("Invalid delivered source identity")

    def track(self, name: str, incoming: bytes, owner: str = "stage", source: str | None = None) -> None:
        self.path(name)
        if owner not in ("stage", "development"):
            raise RuntimeError(f"Invalid delivery owner: {owner}")
        source = self.files.get(name, {}).get("source") if source is None else source
        self.files[name] = {"sha256": digest(incoming), "owner": owner}
        if source is not None:
            self.source_name(source)
            self.files[name]["source"] = source

    def change(self) -> tuple[bytes | None, bytes]:
        record = {"version": 1, "files": self.files}
        return self.before, (json.dumps(record, indent=2, sort_keys=True) + "\n").encode()


def require_idle() -> None:
    result = subprocess.run(["ps", "-axo", "comm="], capture_output=True, text=True, check=True)
    if any(line.rstrip().endswith(("/metal-engine", "/native-engine", "/engine-1", "engine.exe"))
           for line in result.stdout.splitlines()):
        raise RuntimeError("Close the game before changing installed content.")


@contextmanager
def player_guard(app: Path, require_initialized: bool = False):
    # Keep state resolution and the lock protocol owned by the played launcher.
    path = safe_path(app, Path("Contents/Resources/public_launcher.py"))
    namespace = {"__file__": str(path), "__name__": "corsairs_delivery_player_owner"}
    exec(compile(path.read_bytes(), str(path), "exec"), namespace)
    user_dir = namespace["resolve_user_dir"]()
    if not safe_path(user_dir).is_dir():
        if require_initialized:
            raise RuntimeError("Player state is not initialized; open the installed app first.")
        user_dir.mkdir(parents=True, exist_ok=True)
    with safe_path(user_dir, Path(".launch.lock")).open("a+b") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise RuntimeError("Installed app is running or staging; no files changed.") from error
        require_idle()
        yield


def seal_app(app: Path) -> None:
    # These disposable bundled-Python caches are excluded by the exporter.
    frameworks = app / "Contents/Frameworks"
    if frameworks.is_dir():
        for pycache in sorted(frameworks.rglob("__pycache__")):
            shutil.rmtree(pycache)
        for stale in sorted(frameworks.rglob("*.pyc")):
            stale.unlink()
    subprocess.run(["codesign", "--force", "-s", "-", str(app)], check=True)
    subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True)


def transact(changes: dict[Path, tuple[bytes | None, bytes]], app: Path | None = None,
             verify=None, writer=None) -> int:
    """Preflight the whole batch, then restore only still-owned bytes on failure."""
    def read(path):
        safe_path(path.parent, Path(path.name))
        return path.read_bytes() if path.exists() else None

    for path, (before, _) in changes.items():
        if read(path) != before:
            raise RuntimeError(f"Concurrent delivery change: {path}; no files changed.")
    written = []
    write = atomic_write if writer is None else writer
    try:
        for path, (before, after) in changes.items():
            if read(path) != before:
                raise RuntimeError(f"Concurrent delivery change: {path}")
            if before != after:
                write(path, after)
                written.append(path)
        for path, (_, after) in changes.items():
            if read(path) != after:
                raise RuntimeError(f"Delivery verification failed: {path}")
        if verify is not None:
            verify()
        if app is not None and any(path.is_relative_to(app) for path in written):
            seal_app(app)
    except BaseException as error:
        failures = []
        for path in reversed(written):
            try:
                before, after = changes[path]
                if read(path) != after:
                    raise RuntimeError(f"Concurrent change prevents delivery rollback: {path}")
                if before is None:
                    path.unlink()
                else:
                    write(path, before)
            except (OSError, RuntimeError) as rollback_error:
                failures.append(str(rollback_error))
        if app is not None and any(path.is_relative_to(app) for path in written) and not failures:
            try:
                seal_app(app)
            except (OSError, subprocess.CalledProcessError) as rollback_error:
                failures.append(f"Could not restore bundle signature: {rollback_error}")
        if failures:
            raise RuntimeError(f"{error}; delivery rollback incomplete: {'; '.join(failures)}") from error
        raise
    return len(written)


def record_export(app: Path, content: dict[str, bytes] | None = None, sources=None) -> None:
    """Record fresh produced bytes after nested signing, before the outer seal."""
    state = DeliveryState(app / "Contents/Resources", app)
    if state.before is not None:
        raise RuntimeError("Export destination already has delivery state.")
    for name in (*NATIVE_RESOURCES, ENGINE):
        state.track(name, state.path(name).read_bytes())
    for name, incoming in (content or {}).items():
        current = state.path(name).read_bytes()
        if current != incoming:
            raise RuntimeError(f"Stale exported gameplay source: {name}")
        state.track(name, current, source=(sources or {}).get(name))
    atomic_write(state.receipt, state.change()[1])
