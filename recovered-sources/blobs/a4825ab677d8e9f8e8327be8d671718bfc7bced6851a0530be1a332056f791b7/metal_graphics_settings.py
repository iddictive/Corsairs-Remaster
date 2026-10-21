#!/usr/bin/env python3
"""Own the native Metal graphics options record and its source layer.

The game UI serializes this record as ``metal_graphics`` in the global userdata
root.  This adapter deliberately does not read the legacy profile options file:
graphics and device settings must survive profile changes and a normal restart.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_RUNTIME = PROJECT / "experiments/native-metal/.cache/runtime"
METAL_OPTIONS = "metal_graphics"
RESOLUTIONS = ((1280, 720), (1600, 900), (1920, 1080), (2560, 1440))

DEFAULTS = {
    "dynamic_lighting": True,
    "modern_water": True,
    "modern_lighting": True,
    "cinematic": True,
    "light_shafts": True,
    "dynamic_sky": True,
    "antialiasing": False,
    "full_screen": True,
    "resolution": 2,
}

ENV_KEYS = {
    "dynamic_lighting": "STORM_METAL_DYNAMIC_LIGHTING",
    "modern_water": "STORM_METAL_MODERN_WATER",
    "modern_lighting": "STORM_METAL_MODERN_LIGHTING",
    "cinematic": "STORM_METAL_CINEMATIC",
    "light_shafts": "STORM_METAL_LIGHT_SHAFTS",
    "dynamic_sky": "STORM_METAL_DYNAMIC_SKY",
    "antialiasing": "STORM_METAL_ANTIALIASING",
}


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


# Filled after the first source transform.  The source hashes are pinned so a
# future Storm update cannot silently consume this layer.
FILES = (
    FilePatch(
        "PROGRAM/interface/option_sl.c",
        "e34dcee24757681a00e544fe643fb530ee70e869e211c613e2905d219ab46bf2",
        "",
        (),
    ),
    FilePatch(
        "PROGRAM/interface/option_screen.c",
        "6ac2f588a58ea9cbbbdf4b1e500c8aa46f0cded1a41934b0816de540e57edd12",
        "",
        (),
    ),
    FilePatch(
        "RESOURCE/INI/interfaces/option_screen.ini",
        "28ec5c6417518342f061d67b0aed7fcd7ff66dcb78008f6385eb8a3888e175ad",
        "",
        (),
    ),
    FilePatch(
        "RESOURCE/INI/texts/russian/common.ini",
        "a84822877ba63cdfe6ceb0beb5fef8daa158a8fdbd9da73d0a650ff577260562",
        "",
        (),
    ),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_record(path: Path) -> dict[str, str]:
    """Parse the engine's line-oriented key=value record without evaluation."""
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for line_number, raw in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if "=" not in line:
            raise ValueError(f"{path}: malformed line {line_number}")
        key, value = (part.strip() for part in line.split("=", 1))
        if not re.fullmatch(r"[a-z][a-z0-9_]*", key):
            raise ValueError(f"{path}: unsupported key on line {line_number}")
        if not re.fullmatch(r"(?:0|1|true|false|-?[0-9]+)", value, re.IGNORECASE):
            raise ValueError(f"{path}: unsupported value on line {line_number}")
        values[key] = value.lower()
    return values


def bool_value(values: dict[str, str], key: str) -> bool:
    raw = values.get(key)
    if raw is None:
        return DEFAULTS[key]
    if raw in {"1", "true"}:
        return True
    if raw in {"0", "false"}:
        return False
    raise ValueError(f"{key}: expected a boolean")


def graphics_values(runtime: Path) -> dict[str, int | bool]:
    values = parse_record(runtime / METAL_OPTIONS)
    result: dict[str, int | bool] = {
        key: bool_value(values, key) for key in ENV_KEYS
    }
    result["full_screen"] = bool_value(values, "full_screen")
    resolution = int(values.get("resolution", DEFAULTS["resolution"]))
    if resolution < 0 or resolution >= len(RESOLUTIONS):
        raise ValueError("resolution: unsupported preset")
    result["resolution"] = resolution
    return result


def current_engine(engine_ini: Path) -> tuple[bool, int]:
    values: dict[str, str] = {}
    if engine_ini.is_file():
        for line in engine_ini.read_text(encoding="utf-8-sig").splitlines():
            if "=" in line and not line.lstrip().startswith(("#", ";")):
                key, value = (part.strip() for part in line.split("=", 1))
                values[key] = value
    fullscreen = values.get("full_screen", "1") in {"1", "true", "True"}
    try:
        current = (int(values.get("screen_x", "1920")), int(values.get("screen_y", "1080")))
        resolution = min(range(len(RESOLUTIONS)), key=lambda i: abs(RESOLUTIONS[i][0] - current[0]) + abs(RESOLUTIONS[i][1] - current[1]))
    except ValueError:
        resolution = DEFAULTS["resolution"]
    return fullscreen, resolution


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = path.stat().st_mode if path.exists() else 0o644
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def apply_engine(runtime: Path) -> None:
    record = runtime / METAL_OPTIONS
    if not record.exists():
        return
    values = graphics_values(runtime)
    engine = runtime / "engine.ini"
    source = engine.read_text(encoding="utf-8-sig")
    fullscreen = "1" if values["full_screen"] else "0"
    width, height = RESOLUTIONS[int(values["resolution"])]
    replacements = {
        "full_screen": fullscreen,
        "screen_x": str(width),
        "screen_y": str(height),
    }
    for key, value in replacements.items():
        source, count = re.subn(rf"(?m)^{re.escape(key)}\s*=.*$", f"{key} = {value}", source)
        if count != 1:
            raise ValueError(f"{engine}: expected one {key} entry")
    atomic_write(engine, source.encode("utf-8"))


def emit_env(runtime: Path) -> None:
    values = graphics_values(runtime)
    for key, env_name in ENV_KEYS.items():
        print(f"export {env_name}={'1' if values[key] else '0'}")


def transform(source: bytes, spec: FilePatch) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise ValueError(f"{spec.relative_path}: expected source block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if spec.patched_sha256 and digest(result) != spec.patched_sha256:
        raise ValueError(f"{spec.relative_path}: generated hash is not reviewed")
    return result


def prepare(relative_path: str, source: bytes) -> tuple[bytes, str]:
    spec = next(item for item in FILES if item.relative_path == relative_path)
    current = digest(source)
    if current == spec.patched_sha256 and spec.patched_sha256:
        return source, "patched"
    if current != spec.original_sha256:
        raise ValueError(f"{relative_path}: unsupported source hash {current}")
    return transform(source, spec), "original"


def strip(relative_path: str, source: bytes) -> tuple[bytes, str]:
    spec = next(item for item in FILES if item.relative_path == relative_path)
    current = digest(source)
    if current == spec.original_sha256:
        return source, "original"
    if spec.patched_sha256 and current == spec.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(spec.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: reviewed layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != spec.original_sha256:
            raise ValueError(f"{relative_path}: stripped hash is not canonical")
        return result, "patched"
    raise ValueError(f"{relative_path}: unsupported source hash {current}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("apply", "env"))
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME)
    args = parser.parse_args()
    if args.action == "apply":
        apply_engine(args.runtime)
    else:
        emit_env(args.runtime)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
