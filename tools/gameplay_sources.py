"""Complete editable gameplay files and their ordinary delivery projection."""
from __future__ import annotations

import json
from pathlib import Path

from delivery_state import DeliveryState, digest, safe_path

ROOT = Path(__file__).resolve().parents[1] / "src/gameplay"
BACKUPS = ROOT.parents[1] / "experiments/native-metal/.cache/gameplay-source-backups"


def source_bytes(name: str, data: bytes, encoding: str = "utf-8") -> bytes:
    text = data.decode(encoding)
    if Path(name).suffix in (".c", ".h", ".ini", ".txt"):
        text = text.replace("\r\n", "\n")
    return text.encode("utf-8")


def runtime_bytes(name: str, data: bytes, encoding: str = "utf-8") -> bytes:
    text = source_bytes(name, data).decode("utf-8")
    if Path(name).suffix in (".c", ".h", ".ini", ".txt"):
        text = text.replace("\n", "\r\n")
    return text.encode(encoding)


def read(selected=None):
    manifest = safe_path(ROOT, Path("manifest.json"))
    raw = manifest.read_bytes()
    record = json.loads(raw)
    if (not isinstance(record, dict) or set(record) != {"version", "files"}
            or type(record["version"]) is not int or record["version"] != 2
            or not isinstance(record["files"], dict)):
        raise RuntimeError("Invalid gameplay source manifest")
    names = set(record["files"]) if selected is None else set(selected)
    if names - record["files"].keys():
        raise RuntimeError(f"Not a canonical gameplay source: {', '.join(sorted(names - record['files'].keys()))}")
    result, inputs = {}, {manifest: (raw, raw)}
    for name in sorted(names):
        row = record["files"][name]
        if (not isinstance(row, dict) or set(row) != {"encoding", "legacy"}
                or row["encoding"] not in ("utf-8", "cp1251")):
            raise RuntimeError(f"Invalid gameplay source format: {name}")
        encoding, legacy = row["encoding"], row["legacy"]
        relative = Path(name)
        parts = relative.parts
        allowed = (len(parts) > 1 and parts[0] == "PROGRAM" and relative.suffix in (".c", ".h", ".txt")) or (
            len(parts) > 2 and parts[:2] == ("RESOURCE", "INI") and relative.suffix == ".ini") or (
            len(parts) > 3 and parts[:3] == ("RESOURCE", "INI", "texts") and relative.suffix == ".txt")
        if not allowed or relative.as_posix() != name:
            raise RuntimeError(f"Invalid gameplay source path: {name}")
        if not isinstance(legacy, list) or any(not isinstance(sha, str) or len(sha) != 64
                or any(c not in "0123456789abcdef" for c in sha) for sha in legacy):
            raise RuntimeError(f"Invalid legacy source admission: {name}")
        path = safe_path(ROOT, relative)
        data = path.read_bytes()
        result[name] = (runtime_bytes(name, data, encoding), legacy, encoding)
        inputs[path] = (data, data)
    return result, inputs


def backup_change(before: bytes | None, incoming: bytes):
    if before is None or before == incoming:
        return {}
    backup = safe_path(BACKUPS, Path(digest(before)))
    saved = backup.read_bytes() if backup.exists() else None
    if saved is not None and saved != before:
        raise RuntimeError("Changed gameplay source backup")
    return {backup: (saved, before)}


def read_variants():
    manifest = safe_path(ROOT, Path("variants.json"))
    raw = manifest.read_bytes()
    record = json.loads(raw)
    if (not isinstance(record, dict) or set(record) != {"version", "files"}
            or type(record["version"]) is not int or record["version"] != 1
            or not isinstance(record["files"], dict)):
        raise RuntimeError("Invalid gameplay variants")
    return record["files"], {manifest: (raw, raw)}


def variant_source(name: str, current: bytes, state: DeliveryState, declarations):
    """Select an existing product variant once, then retain its source identity."""
    files, inputs = declarations
    row = files.get(name)
    if row is None:
        return None
    if (not isinstance(row, dict) or set(row) != {"encoding", "sources"}
            or row["encoding"] not in ("utf-8", "cp1251")
            or not isinstance(row["sources"], dict) or not row["sources"]):
        raise RuntimeError(f"Invalid gameplay variant: {name}")
    for source, legacy in row["sources"].items():
        DeliveryState.source_name(source)
        if (Path(source).parts[0] != "variants" or Path(source).suffix != Path(name).suffix
                or not isinstance(legacy, list) or any(not isinstance(sha, str) or len(sha) != 64
                or any(c not in "0123456789abcdef" for c in sha) for sha in legacy)):
            raise RuntimeError(f"Invalid gameplay variant source: {source}")
    identity = state.files.get(name, {}).get("source")
    if identity is None:
        hashes = {digest(current), digest(runtime_bytes(name, source_bytes(name, current, row["encoding"]), row["encoding"]))}
        matches = [source for source, legacy in row["sources"].items() if hashes.intersection(legacy)]
        if len(matches) != 1:
            raise RuntimeError(f"Unrecognized gameplay variant: {name}; no files changed")
        identity = matches[0]
    if identity not in row["sources"]:
        raise RuntimeError(f"Unknown delivered variant source: {name}; no files changed")
    path = safe_path(ROOT, Path(identity))
    data = path.read_bytes()
    return runtime_bytes(name, data, row["encoding"]), identity, {**inputs, path: (data, data)}


def prepare_delivery(resources: Path, source_set, state: DeliveryState | None = None,
                     development: bool = False):
    sources, inputs = source_set
    app = resources.parent.parent if resources.name == "Resources" and resources.parent.name == "Contents" else None
    state = state if state is not None else DeliveryState(resources, app)
    if state.resources != resources or state.app != app:
        raise RuntimeError("Gameplay receipt belongs to another runtime.")
    changes = dict(inputs)
    for name in sorted(sources):
        incoming, legacy, encoding = sources[name]
        path = state.path(name)
        before = path.read_bytes() if path.exists() else None
        equivalent = before is not None and source_bytes(name, before, encoding) == source_bytes(name, incoming, encoding)
        state.admit(name, before, incoming, legacy, development=development or equivalent, create=True)
        owner = state.files.get(name, {}).get("owner", "stage") if not development else "stage"
        state.track(name, incoming, owner=owner)
        changes.update(backup_change(before, incoming))
        changes[path] = (before, incoming)
    changes[state.receipt] = state.change()
    return changes
