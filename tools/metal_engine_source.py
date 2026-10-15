"""Identity of directly editable native engine and renderer build inputs."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / "experiments/native-metal"
ENGINE = METAL / "engine"
RECEIPT = METAL / ".cache/gameplay-compiler.json"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_files():
    if not (ENGINE / "src/apps/engine/src/main.cpp").is_file():
        raise RuntimeError("Tracked Metal engine sources are missing")
    paths = {p for p in ENGINE.rglob("*") if p.is_file()}
    for p in METAL.iterdir():
        if p.is_file() and (p.name in {"CMakeLists.txt", "build.sh", "script_vm_probe.cmake"}
                            or p.suffix in {".h", ".hpp", ".cpp", ".mm"}
                            and "probe" not in p.stem and "fixture" not in p.stem):
            paths.add(p)
    for directory in (METAL / "audio", METAL.parent / "native-storm/audio"):
        paths.update(p for p in directory.rglob("*") if p.is_file())
    paths.add(Path(__file__).resolve())
    manifest = METAL / "inputs/manifest.json"
    if manifest.is_file():
        paths.add(manifest)
    for path in sorted(paths):
        if path.is_symlink() or not path.is_file():
            raise RuntimeError(f"Missing or linked engine input: {path.relative_to(PROJECT)}")
        yield path


def fingerprint() -> str:
    value = hashlib.sha256()
    for path in source_files():
        value.update(path.relative_to(PROJECT).as_posix().encode() + b"\0")
        value.update(bytes.fromhex(digest(path)))
    return value.hexdigest()


def record(engine: Path, expected: str):
    current = fingerprint()
    if current != expected:
        raise RuntimeError("Engine sources changed during build; rebuild the completed source batch")
    data = {"version": 2, "source_sha256": current, "engine_sha256": digest(engine)}
    RECEIPT.parent.mkdir(parents=True, exist_ok=True)
    pending = RECEIPT.with_suffix(".pending")
    pending.write_text(json.dumps(data, indent=2) + "\n")
    pending.replace(RECEIPT)


def ready(built: Path, staged: Path) -> bool:
    try:
        data = json.loads(RECEIPT.read_text())
        return (data.get("version") == 2 and data.get("source_sha256") == fingerprint()
                and data.get("engine_sha256") == digest(built)
                and data["engine_sha256"] == digest(staged))
    except (OSError, ValueError, RuntimeError, KeyError):
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("fingerprint", "record"))
    parser.add_argument("--engine", type=Path)
    parser.add_argument("--expected")
    args = parser.parse_args()
    if args.command == "fingerprint":
        print(fingerprint())
    elif args.engine is None or args.expected is None:
        parser.error("record needs --engine and --expected")
    else:
        record(args.engine, args.expected)
