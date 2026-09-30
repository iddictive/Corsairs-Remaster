#!/usr/bin/env python3
"""Shared exact-hash patching for the mutable primary KVL script runtime."""

from __future__ import annotations

import hashlib
import os
import subprocess
import tempfile
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator


TARGET_ROOT = Path(
    "/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/"
    "KVL"
)
LIFECYCLE_LOCK = TARGET_ROOT / ".codex-engine-lifecycle.lock"


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform(source: bytes, spec: FilePatch) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise RuntimeError(f"{spec.relative_path}: expected block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if sha256(result) != spec.patched_sha256:
        raise RuntimeError(f"{spec.relative_path}: generated patched hash does not match")
    return result


def classify(path: Path, spec: FilePatch) -> str:
    if not path.is_file():
        return "missing"
    digest = sha256(path.read_bytes())
    if digest == spec.original_sha256:
        return "original"
    if digest == spec.patched_sha256:
        return "patched"
    return "unsupported"


def atomic_write(path: Path, data: bytes) -> None:
    mode = path.stat().st_mode if path.exists() else 0o644
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temp_name, mode)
        os.replace(temp_name, path)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)


def require_idle() -> None:
    engine = TARGET_ROOT / "engine.exe"
    result = subprocess.run(
        ["lsof", "-t", "--", str(engine)], capture_output=True, text=True, check=False
    )
    if result.returncode == 0:
        holders = ", ".join(result.stdout.split())
        raise RuntimeError(f"test engine is running (holder PID(s): {holders})")
    if result.returncode != 1 or result.stdout or result.stderr:
        detail = result.stderr.strip() or f"exit {result.returncode}"
        raise RuntimeError(f"cannot prove engine is idle: {detail}")


@contextmanager
def lifecycle_lock() -> Iterator[None]:
    try:
        fd = os.open(LIFECYCLE_LOCK, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    except FileExistsError as error:
        raise RuntimeError(f"runtime lifecycle is already locked: {LIFECYCLE_LOCK}") from error
    try:
        with os.fdopen(fd, "w", encoding="ascii") as stream:
            stream.write(str(os.getpid()))
            stream.flush()
            os.fsync(stream.fileno())
        yield
    finally:
        try:
            if LIFECYCLE_LOCK.read_text(encoding="ascii") == str(os.getpid()):
                LIFECYCLE_LOCK.unlink()
        except FileNotFoundError:
            pass


class PatchSet:
    def __init__(self, snapshot_root: Path, files: tuple[FilePatch, ...]) -> None:
        self.snapshot_root = snapshot_root
        self.files = files

    def overall_state(self, root: Path) -> tuple[str, list[tuple[FilePatch, str]]]:
        rows = [(spec, classify(root / spec.relative_path, spec)) for spec in self.files]
        states = {state for _, state in rows}
        return (states.pop() if len(states) == 1 else "mixed"), rows

    def print_status(self, root: Path) -> int:
        state, rows = self.overall_state(root)
        print(f"target: {root}")
        print(f"state:  {state}")
        for spec, item_state in rows:
            path = root / spec.relative_path
            digest = sha256(path.read_bytes()) if path.is_file() else "-"
            print(f"{item_state:11} {digest}  {spec.relative_path}")
        return 0 if state in {"original", "patched"} else 1

    def apply(self, root: Path) -> str:
        self._require_target(root)
        state, _ = self.overall_state(root)
        if state == "patched":
            return "already patched"
        if state != "original":
            raise RuntimeError(f"refusing apply: aggregate state is {state}")

        with lifecycle_lock():
            require_idle()
            originals = {
                spec.relative_path: (root / spec.relative_path).read_bytes()
                for spec in self.files
            }
            for spec in self.files:
                snapshot = self.snapshot_root / spec.relative_path
                if snapshot.exists():
                    if sha256(snapshot.read_bytes()) != spec.original_sha256:
                        raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
                else:
                    atomic_write(snapshot, originals[spec.relative_path])
                    os.chmod(snapshot, 0o444)

            written: list[Path] = []
            try:
                for spec in self.files:
                    path = root / spec.relative_path
                    atomic_write(path, transform(originals[spec.relative_path], spec))
                    written.append(path)
                final, _ = self.overall_state(root)
                if final != "patched":
                    raise RuntimeError(f"post-apply state is {final}")
            except Exception:
                for path in reversed(written):
                    atomic_write(path, originals[str(path.relative_to(root))])
                raise
        return "patched"

    def revert(self, root: Path) -> str:
        self._require_target(root)
        state, _ = self.overall_state(root)
        if state == "original":
            return "already original"
        if state != "patched":
            raise RuntimeError(f"refusing revert: aggregate state is {state}")

        with lifecycle_lock():
            require_idle()
            patched = {
                spec.relative_path: (root / spec.relative_path).read_bytes()
                for spec in self.files
            }
            written: list[Path] = []
            try:
                for spec in self.files:
                    snapshot = self.snapshot_root / spec.relative_path
                    if not snapshot.is_file():
                        raise RuntimeError(f"snapshot is missing: {snapshot}")
                    original = snapshot.read_bytes()
                    if sha256(original) != spec.original_sha256:
                        raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
                    path = root / spec.relative_path
                    atomic_write(path, original)
                    written.append(path)
                final, _ = self.overall_state(root)
                if final != "original":
                    raise RuntimeError(f"post-revert state is {final}")
            except Exception:
                for path in reversed(written):
                    atomic_write(path, patched[str(path.relative_to(root))])
                raise
        return "original"

    @staticmethod
    def _require_target(root: Path) -> None:
        if root.resolve() != TARGET_ROOT.resolve():
            raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
