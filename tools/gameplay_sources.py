"""Complete editable gameplay files and their ordinary delivery projection."""
from __future__ import annotations

import json
from pathlib import Path

from delivery_state import DeliveryState, digest, safe_path

ROOT = Path(__file__).resolve().parents[1] / "src/gameplay"
BACKUPS = ROOT.parents[1] / "experiments/native-metal/.cache/gameplay-source-backups"


def source_bytes(name: str, data: bytes) -> bytes:
    return data.replace(b"\r\n", b"\n") if Path(name).suffix in (".c", ".h") else data


def runtime_bytes(name: str, data: bytes) -> bytes:
    data = source_bytes(name, data)
    return data.replace(b"\n", b"\r\n") if Path(name).suffix in (".c", ".h") else data


def read(selected=None):
    manifest = safe_path(ROOT, Path("manifest.json"))
    raw = manifest.read_bytes()
    record = json.loads(raw)
    if (not isinstance(record, dict) or set(record) != {"version", "files"}
            or type(record["version"]) is not int or record["version"] != 1
            or not isinstance(record["files"], dict)):
        raise RuntimeError("Invalid gameplay source manifest")
    names = set(record["files"]) if selected is None else set(selected)
    if names - record["files"].keys():
        raise RuntimeError(f"Not a canonical gameplay source: {', '.join(sorted(names - record['files'].keys()))}")
    result, inputs = {}, {manifest: (raw, raw)}
    for name in sorted(names):
        legacy = record["files"][name]
        relative = Path(name)
        parts = relative.parts
        allowed = (len(parts) > 1 and parts[0] == "PROGRAM" and relative.suffix in (".c", ".h", ".txt")) or (
            len(parts) > 2 and parts[:2] == ("RESOURCE", "INI") and relative.suffix == ".ini")
        if not allowed or relative.as_posix() != name:
            raise RuntimeError(f"Invalid gameplay source path: {name}")
        if not isinstance(legacy, list) or any(not isinstance(sha, str) or len(sha) != 64
                or any(c not in "0123456789abcdef" for c in sha) for sha in legacy):
            raise RuntimeError(f"Invalid legacy source admission: {name}")
        path = safe_path(ROOT, relative)
        data = path.read_bytes()
        result[name] = (runtime_bytes(name, data), legacy)
        inputs[path] = (data, data)
    return result, inputs


def prepare_delivery(resources: Path, source_set, state: DeliveryState | None = None,
                     development: bool = False):
    sources, inputs = source_set
    app = resources.parent.parent if resources.name == "Resources" and resources.parent.name == "Contents" else None
    state = state if state is not None else DeliveryState(resources, app)
    if state.resources != resources or state.app != app:
        raise RuntimeError("Gameplay receipt belongs to another runtime.")
    changes = dict(inputs)
    for name in sorted(sources):
        incoming, legacy = sources[name]
        path = state.path(name)
        before = path.read_bytes() if path.exists() else None
        equivalent = before is not None and source_bytes(name, before) == source_bytes(name, incoming)
        state.admit(name, before, incoming, legacy, development=development or equivalent, create=True)
        state.track(name, incoming)
        if before is not None and before != incoming:
            backup = safe_path(BACKUPS, Path(digest(before)))
            saved = backup.read_bytes() if backup.exists() else None
            if saved is not None and saved != before:
                raise RuntimeError(f"Changed gameplay source backup: {name}")
            changes[backup] = (saved, before)
        changes[path] = (before, incoming)
    changes[state.receipt] = state.change()
    return changes
