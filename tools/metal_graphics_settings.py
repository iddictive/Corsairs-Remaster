#!/usr/bin/env python3
"""Own the native Metal graphics options record and its runtime application.

The game UI serializes this record as ``metal_graphics`` in the Metal runtime
root.  This adapter deliberately does not read the legacy profile options file:
graphics and device settings must survive profile changes and a normal restart.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import tempfile
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_RUNTIME = PROJECT / "experiments/native-metal/.cache/runtime"
METAL_OPTIONS = "metal_graphics"
RESOLUTIONS = ((1280, 720), (1600, 900), (1920, 1080), (2560, 1440))
CURRENT_RESOLUTION = len(RESOLUTIONS)
DESKTOP_RESOLUTION = CURRENT_RESOLUTION + 1
# Keep the old name as a source/probe compatibility alias.  Index 4 means the
# saved current/custom dimensions; index 5 is the new active-display mode.
CUSTOM_RESOLUTION = CURRENT_RESOLUTION
DEFAULT_RESOLUTION_WIDTH = 1920
DEFAULT_RESOLUTION_HEIGHT = 1080
MIN_RESOLUTION_WIDTH = 320
MIN_RESOLUTION_HEIGHT = 200
MAX_RESOLUTION_WIDTH = 16384
MAX_RESOLUTION_HEIGHT = 16384

DEFAULTS = {
    "dynamic_lighting": True,
    "shadow_quality": 1,
    "modern_water": True,
    "modern_lighting": True,
    "cinematic": True,
    "light_shafts": True,
    "dynamic_sky": True,
    "antialiasing": False,
    "full_screen": True,
    "resolution": 2,
    "resolution_width": DEFAULT_RESOLUTION_WIDTH,
    "resolution_height": DEFAULT_RESOLUTION_HEIGHT,
    "display_width": DEFAULT_RESOLUTION_WIDTH,
    "display_height": DEFAULT_RESOLUTION_HEIGHT,
}

DISPLAY_MODE_CUSTOM = 0
DISPLAY_MODE_DESKTOP = 1

ENV_KEYS = {
    "dynamic_lighting": "STORM_METAL_DYNAMIC_LIGHTING",
    "modern_water": "STORM_METAL_MODERN_WATER",
    "modern_lighting": "STORM_METAL_MODERN_LIGHTING",
    "cinematic": "STORM_METAL_CINEMATIC",
    "light_shafts": "STORM_METAL_LIGHT_SHAFTS",
    "dynamic_sky": "STORM_METAL_DYNAMIC_SKY",
    "antialiasing": "STORM_METAL_ANTIALIASING",
}

INTEGER_ENV_KEYS = {
    "shadow_quality": "STORM_METAL_SHADOW_QUALITY",
}

SHADOW_QUALITY_MIN = 0
SHADOW_QUALITY_MAX = 2

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

def integer_value(values: dict[str, str], key: str, minimum: int, maximum: int) -> int:
    raw = values.get(key)
    if raw is None:
        return int(DEFAULTS[key])
    if not re.fullmatch(r"-?[0-9]+", raw):
        raise ValueError(f"{key}: expected an integer")
    value = int(raw)
    if value < minimum or value > maximum:
        raise ValueError(f"{key}: unsupported value")
    return value

def _dimension_value(values: dict[str, str], key: str) -> int:
    try:
        value = int(values[key])
    except (KeyError, ValueError) as error:
        raise ValueError(f"{key}: expected an integer") from error
    is_width = key.endswith("_width")
    limit = (
        MIN_RESOLUTION_WIDTH if is_width else MIN_RESOLUTION_HEIGHT,
        MAX_RESOLUTION_WIDTH if is_width else MAX_RESOLUTION_HEIGHT,
    )
    if not limit[0] <= value <= limit[1]:
        raise ValueError(f"{key}: unsupported custom dimension")
    return value

def _stored_base_dimensions(values: dict[str, str]) -> tuple[int, int]:
    present = [key in values for key in ("resolution_width", "resolution_height")]
    if any(present) and not all(present):
        raise ValueError("custom resolution requires both resolution_width and resolution_height")
    if not any(present):
        return DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT
    return (
        _dimension_value(values, "resolution_width"),
        _dimension_value(values, "resolution_height"),
    )

def _stored_display_dimensions(
    values: dict[str, str], base_width: int, base_height: int
) -> tuple[int, int]:
    present = [key in values for key in ("display_width", "display_height")]
    if any(present) and not all(present):
        raise ValueError("display geometry requires both display_width and display_height")
    if not any(present):
        return base_width, base_height
    return (
        _dimension_value(values, "display_width"),
        _dimension_value(values, "display_height"),
    )

def _render_dimensions(
    resolution: int,
    base_width: int,
    base_height: int,
    display_width: int,
    display_height: int,
) -> tuple[int, int]:
    if resolution == CURRENT_RESOLUTION:
        return base_width, base_height
    if resolution == DESKTOP_RESOLUTION:
        return display_width, display_height
    width = RESOLUTIONS[resolution][0]
    height = (width * display_height + display_width // 2) // display_width
    if not MIN_RESOLUTION_HEIGHT <= height <= MAX_RESOLUTION_HEIGHT:
        raise ValueError("resolution: derived preset height is unsupported")
    return width, height

def graphics_values(runtime: Path) -> dict[str, int | bool]:
    values = parse_record(runtime / METAL_OPTIONS)
    result: dict[str, int | bool] = {
        key: bool_value(values, key) for key in ENV_KEYS
    }
    result["shadow_quality"] = integer_value(
        values, "shadow_quality", SHADOW_QUALITY_MIN, SHADOW_QUALITY_MAX
    )
    result["full_screen"] = bool_value(values, "full_screen")
    resolution = int(values.get("resolution", DEFAULTS["resolution"]))
    if resolution < 0 or resolution > DESKTOP_RESOLUTION:
        raise ValueError("resolution: unsupported preset or custom choice")
    if resolution in (CURRENT_RESOLUTION, DESKTOP_RESOLUTION) and not {
        "resolution_width",
        "resolution_height",
    }.issubset(values):
        raise ValueError("current/desktop resolution requires both resolution_width and resolution_height")
    base_width, base_height = _stored_base_dimensions(values)
    display_width, display_height = _stored_display_dimensions(values, base_width, base_height)
    if resolution == DESKTOP_RESOLUTION and not result["full_screen"]:
        resolution = CURRENT_RESOLUTION
    result["resolution"] = resolution
    result["resolution_width"] = base_width
    result["resolution_height"] = base_height
    result["display_width"] = display_width
    result["display_height"] = display_height
    result["render_width"], result["render_height"] = _render_dimensions(
        resolution, base_width, base_height, display_width, display_height
    )
    return result

def current_engine(engine_ini: Path) -> tuple[bool, int, int, int]:
    values: dict[str, str] = {}
    if engine_ini.is_file():
        for line in engine_ini.read_text(encoding="utf-8-sig").splitlines():
            if "=" in line and not line.lstrip().startswith(("#", ";")):
                key, value = (part.strip() for part in line.split("=", 1))
                values[key] = value
    fullscreen = values.get("full_screen", "1") in {"1", "true", "True"}
    try:
        display_mode = int(values.get("display_mode", str(DISPLAY_MODE_CUSTOM)))
    except ValueError:
        display_mode = DISPLAY_MODE_CUSTOM
    try:
        width = int(values.get("screen_x", str(DEFAULT_RESOLUTION_WIDTH)))
        height = int(values.get("screen_y", str(DEFAULT_RESOLUTION_HEIGHT)))
        _dimension_value({"resolution_width": str(width)}, "resolution_width")
        _dimension_value({"resolution_height": str(height)}, "resolution_height")
    except ValueError:
        width, height = DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT
    if display_mode == DISPLAY_MODE_DESKTOP and fullscreen:
        resolution = DESKTOP_RESOLUTION
    else:
        resolution = next(
            (index for index, dimensions in enumerate(RESOLUTIONS) if dimensions == (width, height)),
            CURRENT_RESOLUTION,
        )
    return fullscreen, resolution, width, height

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
    # Renderer GetInt(nullptr, ...) reads the unnamed INI section. Appending a
    # missing key after [controls] would silently hide Desktop mode from it.
    section = re.search(r"(?m)^\s*\[", source)
    global_end = section.start() if section else len(source)
    global_values, sections = source[:global_end], source[global_end:]
    fullscreen = "1" if values["full_screen"] else "0"
    width = int(values["render_width"])
    height = int(values["render_height"])
    display_mode = (
        DISPLAY_MODE_DESKTOP
        if values["full_screen"] and int(values["resolution"]) == DESKTOP_RESOLUTION
        else DISPLAY_MODE_CUSTOM
    )
    replacements = {
        "full_screen": fullscreen,
        "screen_x": str(width),
        "screen_y": str(height),
        "display_mode": str(display_mode),
    }
    for key, value in replacements.items():
        global_values, count = re.subn(rf"(?m)^{re.escape(key)}\s*=.*$", f"{key} = {value}", global_values)
        if count == 0 and key == "display_mode":
            suffix = "" if global_values.endswith("\n") else "\n"
            global_values += f"{suffix}{key} = {value}\n"
            count = 1
        if count != 1:
            raise ValueError(f"{engine}: expected one {key} entry")
    atomic_write(engine, (global_values + sections).encode("utf-8"))

def initialize_record(runtime: Path) -> None:
    record = runtime / METAL_OPTIONS
    if record.exists():
        graphics_values(runtime)  # Reject malformed persisted input before launch.
        return
    values = dict(DEFAULTS)
    (
        values["full_screen"],
        values["resolution"],
        values["resolution_width"],
        values["resolution_height"],
    ) = current_engine(runtime / "engine.ini")
    values["display_width"] = values["resolution_width"]
    values["display_height"] = values["resolution_height"]
    atomic_write(record, "".join(f"{key}={int(value)}\n" for key, value in values.items()).encode())

def launch_environment(runtime: Path, inherited: dict[str, str]) -> dict[str, str]:
    result = dict(inherited)
    values = graphics_values(runtime)
    for key, env_name in ENV_KEYS.items():
        # An explicit diagnostic override still wins; ordinary launches use UI.
        result.setdefault(env_name, str(int(values[key])))
    for key, env_name in INTEGER_ENV_KEYS.items():
        # An explicit diagnostic override still wins; ordinary launches use UI.
        result.setdefault(env_name, str(int(values[key])))
    return result

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("initialize", "launch"))
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    initialize_record(args.runtime)
    apply_engine(args.runtime)
    if args.action == "launch":
        if args.binary is None or not args.binary.is_file():
            parser.error("launch requires an existing --binary")
        binary = str(args.binary.resolve())
        os.execve(binary, [binary], launch_environment(args.runtime, dict(os.environ)))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
